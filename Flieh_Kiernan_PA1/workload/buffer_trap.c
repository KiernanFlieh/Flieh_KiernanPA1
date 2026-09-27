/* CS 433 PA1 -- provided demonstration. Do not modify. Question 1 of
 * analysis.md asks you to run this program TWO ways and explain the result.
 *
 *   ./workload/buffer_trap              # stdout is your terminal
 *   ./workload/buffer_trap | cat        # stdout is a pipe
 *
 * The two runs do not print the same thing. Nothing about the program
 * changed -- only what stdout is connected to. Figure out why before you read
 * anyone's explanation; the answer is the single most useful thing you will
 * learn from PA1, and it is on the midterm.
 *
 * Hint: printf() does not write to the kernel. It writes into a buffer that
 * lives inside your process. Now ask what fork() does to that buffer.
 *
 * Build: gcc -Wall -Wextra -pthread -std=c11 -O2 -o buffer_trap buffer_trap.c
 */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    printf("A: printed before fork()\n");

    pid_t pid = fork();
    if (pid < 0) {
        perror("buffer_trap: fork");
        return 1;
    }

    if (pid == 0) {
        printf("B: child\n");
        return 0;
    }

    wait(NULL);
    printf("C: parent\n");
    return 0;
}
