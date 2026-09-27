/* CS 433 PA1 -- provided workload. You do not need to modify this file.
 *
 * spin MS -- burn about MS milliseconds of CPU time in user mode. It never
 * sleeps and never blocks, so it is on a CPU for essentially every instant of
 * its life and its wall time and its *user* CPU time come out nearly equal.
 * (It does read the monotonic clock once per batch of arithmetic. On both
 * Linux and macOS that read is served out of a page the kernel shares with
 * every process, so it usually costs no system call at all -- which is why
 * the *sys* number stays near zero too.) Use it to check that your user-CPU
 * measurement is real.
 *
 *   ./ptime ./workload/spin 500     -> wall ~= 0.5 s, user ~= 0.5 s
 *
 * Build: gcc -Wall -Wextra -pthread -std=c11 -O2 -o spin spin.c
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
    long ms = 300;
    if (argc > 1)
        ms = strtol(argv[1], NULL, 10);
    if (ms < 0)
        ms = 0;

    double target = (double)ms / 1000.0;

    struct timespec t0, now;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* volatile so the optimizer cannot delete the arithmetic. */
    volatile double acc = 0.0;
    double elapsed = 0.0;

    while (elapsed < target) {
        for (int i = 0; i < 20000; i++)
            acc = acc + 1.0;
        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed = (double)(now.tv_sec - t0.tv_sec)
                + (double)(now.tv_nsec - t0.tv_nsec) / 1e9;
    }

    printf("spin: burned %ld ms of CPU (checksum %.0f)\n", ms, (double)acc);
    return 0;
}
