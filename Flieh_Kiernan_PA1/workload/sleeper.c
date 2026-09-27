/* CS 433 PA1 -- provided workload. You do not need to modify this file.
 *
 * sleeper MS -- sleep about MS milliseconds. A sleeping process is off the
 * CPU entirely, so its wall time is large and its CPU time is ~0. Use it to
 * check that you are measuring wall time and CPU time separately.
 *
 *   ./ptime ./workload/sleeper 800  -> wall ~= 0.8 s, user ~= 0.000 s
 *
 * Build: gcc -Wall -Wextra -pthread -std=c11 -O2 -o sleeper sleeper.c
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    long ms = 500;
    if (argc > 1)
        ms = strtol(argv[1], NULL, 10);
    if (ms < 0)
        ms = 0;

    struct timespec req;
    req.tv_sec  = ms / 1000;
    req.tv_nsec = (ms % 1000) * 1000000L;

    /* nanosleep can return early if a signal arrives; finish the remainder. */
    struct timespec rem;
    while (nanosleep(&req, &rem) != 0) {
        if (errno != EINTR) {
            perror("sleeper: nanosleep");
            return 1;
        }
        req = rem;
    }

    printf("sleeper: slept %ld ms\n", ms);
    return 0;
}
