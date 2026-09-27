/* CS 433 PA1 -- provided workload. You do not need to modify this file.
 *
 * statuser exit N     -- terminate normally with exit code N
 * statuser signal N   -- terminate by raising signal number N on itself
 *
 * This gives you a child whose ending is completely predictable, so you can
 * check both branches of your status reporting:
 *
 *   ./ptime ./workload/statuser exit 42     -> status : exited 42
 *   ./ptime ./workload/statuser signal 15   -> status : signaled 15 (SIGTERM)
 *
 * Signal numbers 2 (SIGINT), 9 (SIGKILL), 11 (SIGSEGV) and 15 (SIGTERM) are
 * the same on Linux and macOS. Others are not -- do not assume.
 *
 * Build: gcc -Wall -Wextra -pthread -std=c11 -O2 -o statuser statuser.c
 */
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void)
{
    fprintf(stderr, "statuser: usage: statuser exit N | statuser signal N\n");
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        usage();
        return 2;
    }

    long n = strtol(argv[2], NULL, 10);

    if (strcmp(argv[1], "exit") == 0) {
        printf("statuser: exiting with code %ld\n", n);
        fflush(stdout);
        return (int)(n & 0xff);
    }

    if (strcmp(argv[1], "signal") == 0) {
        printf("statuser: raising signal %ld on myself\n", n);
        fflush(stdout);            /* flush before we die, or nothing prints */
        raise((int)n);
        /* Reached only if the signal was ignored or handled. */
        fprintf(stderr, "statuser: signal %ld did not kill me\n", n);
        return 3;
    }

    usage();
    return 2;
}
