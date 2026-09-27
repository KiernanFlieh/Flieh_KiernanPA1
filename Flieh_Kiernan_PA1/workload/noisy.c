/* CS 433 PA1 -- provided workload. You do not need to modify this file.
 *
 * noisy -- writes one line to stdout and one line to stderr, then exits 0.
 *
 * Use it to prove that your report goes to *stderr* and does not contaminate
 * the child's stdout:
 *
 *   ./ptime ./workload/noisy > out.txt
 *   cat out.txt        # must contain ONLY the stdout line below
 *
 * Build: gcc -Wall -Wextra -pthread -std=c11 -O2 -o noisy noisy.c
 */
#include <stdio.h>

int main(void)
{
    printf("noisy: this line is on stdout\n");
    fprintf(stderr, "noisy: this line is on stderr\n");
    return 0;
}
