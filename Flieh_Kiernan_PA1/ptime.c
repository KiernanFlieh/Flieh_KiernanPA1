/* ===========================================================================
 * CS 433 Operating Systems -- Fall 2026 -- CSU San Marcos
 * PA1: ptime -- run a command as a child process and time it.
 *
 * NAME(S): <Kiernan Flieh, Cyril Tabaranza, Nathan Nguyen, Jared Fabian, Nick Pieratos>
 * DATE:    <9/27/2026>
 * ===========================================================================
 *
 * This file compiles and runs AS GIVEN. Try it first:
 *
 *     make
 *     ./ptime /bin/echo hello
 *
 * It prints the report in the exact format the graders expect -- but every
 * number in it is a lie, because no child process has been created yet. Your
 * job is to make the numbers true. Search for "TODO" below; there are five.
 *
 * What is already written for you (do not rewrite these):
 *   signal_name()   signal number -> name, portable across Linux and macOS
 *   ts_to_sec()     struct timespec -> seconds as a double
 *   tv_to_sec()     struct timeval  -> seconds as a double
 *   print_report()  the exact output format. If you change it, tests fail.
 *
 * Build: gcc -Wall -Wextra -pthread -std=c11 -O2 -o ptime ptime.c
 * Your submission must compile with ZERO warnings under those exact flags.
 */

/* -std=c11 makes the compiler strictly standard, and glibc then hides every
 * POSIX function -- fork, execvp, clock_gettime, all of them. This line asks
 * for the POSIX.1-2008 interfaces back. It must come before any #include. */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>

/* ptime's own failure exit code: usage error, or a failed fork/waitpid/clock_gettime/getrusage.
 * Deliberately different from 126 and 127, which mean "the command you asked
 * for could not be run". */
#define PTIME_FAILURE 2

/* ------------------------------------------------------------------ *
 * Provided helpers
 * ------------------------------------------------------------------ */

/* We do not use strsignal() here: glibc returns "Terminated" for SIGTERM but
 * macOS returns "Terminated: 15", so the output would not be reproducible. */
static const char *signal_name(int sig)
{
    switch (sig) {
    case SIGHUP:  return "SIGHUP";
    case SIGINT:  return "SIGINT";
    case SIGQUIT: return "SIGQUIT";
    case SIGILL:  return "SIGILL";
    case SIGABRT: return "SIGABRT";
    case SIGFPE:  return "SIGFPE";
    case SIGKILL: return "SIGKILL";
    case SIGSEGV: return "SIGSEGV";
    case SIGPIPE: return "SIGPIPE";
    case SIGALRM: return "SIGALRM";
    case SIGTERM: return "SIGTERM";
    case SIGBUS:  return "SIGBUS";
    case SIGUSR1: return "SIGUSR1";
    case SIGUSR2: return "SIGUSR2";
    default:      return "unknown";
    }
}

static double ts_to_sec(const struct timespec *ts)
{
    return (double)ts->tv_sec + (double)ts->tv_nsec / 1e9;
}

static double tv_to_sec(const struct timeval *tv)
{
    return (double)tv->tv_sec + (double)tv->tv_usec / 1e6;
}

/* The exact required output. Seven lines, all on `out` (which must be
 * stderr). Field names are padded to 7 columns so the colons line up.
 * DO NOT CHANGE THIS FUNCTION -- tests/run_tests.sh compares against it. */
static void print_report(FILE *out, char *const cmd[], pid_t pid, int status,
                         double wall, double user, double sys)
{
    fprintf(out, "=== ptime ===\n");

    fprintf(out, "%-7s : ", "command");
    for (int i = 0; cmd[i] != NULL; i++)
        fprintf(out, "%s%s", (i == 0) ? "" : " ", cmd[i]);
    fputc('\n', out);

    fprintf(out, "%-7s : %ld\n", "pid", (long)pid);

    if (WIFEXITED(status)) {
        fprintf(out, "%-7s : exited %d\n", "status", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        int sig = WTERMSIG(status);
        fprintf(out, "%-7s : signaled %d (%s)\n", "status", sig,
                signal_name(sig));
    } else {
        fprintf(out, "%-7s : unknown 0x%x\n", "status", (unsigned)status);
    }

    fprintf(out, "%-7s : %.3f s\n", "wall", wall);
    fprintf(out, "%-7s : %.3f s\n", "user", user);
    fprintf(out, "%-7s : %.3f s\n", "sys",  sys);
}

/* ------------------------------------------------------------------ *
 * main -- this is the part you write
 * ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "ptime: usage: ptime COMMAND [ARG]...\n");
        return PTIME_FAILURE;
    }

    /* argv is always NULL-terminated by the C standard, so &argv[1] is
     * already a valid argument vector for execvp(). No copying needed. */
    char **cmd = &argv[1];

    struct timespec t0 = {0, 0}, t1 = {0, 0};

    if(clock_gettime(CLOCK_MONOTONIC, &t0) == -1) {
        return PTIME_FAILURE;
    }

    fflush(NULL);

    pid_t pid = fork();

    if (pid == -1) {
        return PTIME_FAILURE;
    }

    if (pid == 0){
        if(execvp(cmd[0], cmd) == -1) {
            int save_errno = errno;
            fprintf(stderr, "ptime: cannot run '%s': %s\n", cmd[0], strerror(save_errno));
            if (save_errno == ENOENT) {
                _exit(127);
            } else {
                _exit(126);
            }
        }
    }

    int status = 0; 

    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            return PTIME_FAILURE;
        }
    }

     if (clock_gettime(CLOCK_MONOTONIC, &t1) == -1) {
         return PTIME_FAILURE;
    }

    struct rusage ru;

    if (getrusage(RUSAGE_CHILDREN, &ru) == -1) {
        return PTIME_FAILURE;
    }
    
    double wall = ts_to_sec(&t1) - ts_to_sec(&t0);
    double user = tv_to_sec(&ru.ru_utime);
    double sys = tv_to_sec(&ru.ru_stime);
    
    print_report(stderr, cmd, pid, status, wall, user, sys);

    if(WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return PTIME_FAILURE;
}
