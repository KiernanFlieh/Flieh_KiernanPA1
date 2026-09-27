# PA1 Written Analysis

**Name(s):** _(Kiernan Flieh, Cyril Tabaranza, Nathan Nguyen, Jared Fabian, Nick Pieratos)_
**Date:*9/27/2026*
**Built and tested on:** _(built on Ubuntu (WSL) GCC version 13.3.0 and tested on the CSUSM CS433 course server. GCC version 11.5.0)_

Answer all five questions, **including every lettered part** — a question with
(a), (b) and (c) is not answered until all three are. Two to four sentences per
part is the right size; the whole file should come to two or three pages, not
one and not eight. Paste real output from your own runs where a question asks
for it; invented output is easy to spot and is handled as an academic-honesty
issue, not as a wrong answer.

Budget about two hours for this, and write it while the code is fresh in your
head. It is 20 points, which is more than any two tests are worth.

Write your own explanation. "It gets buffered" is not an explanation. Say
*what* is buffered, *where* that buffer lives, and *what happens to it*.

---

## Q1. The buffering trap (4 points)

Build and run the provided demonstration two different ways:

```
make
./workload/buffer_trap
./workload/buffer_trap | cat
```

Run the first command in a real terminal (an ssh session or a WSL shell), not from an editor's Run button. If both outputs look the same, your stdout is not a terminal.

**(a)** Paste both outputs exactly as you got them.

./workload/buffer_trap OUTPUT: 
A: printed before fork()
B: child
C: parent
./workload/buffer_trap | cat OUTPUT: 
A: printed before fork()
B: child
A: printed before fork()
C: parent

**(b)** They are different, and the program did not change. Explain why, in
terms of where `printf` actually writes and what `fork()` copies.

I believe its because printf writes to the stdout buffer before the actual output is sent. So when we run the program in the terminal we end up flushing the buffer before fork because of the newline. For buffert_trap | cat, I believe its because the output is being held in a buffer and isn't printed instantly. So when the fork happens we give the child a copy of the parent with the output still in the buffer, causing the parent and child to both print the line. 

**(c)** Name the one library call that fixes it, say exactly where it goes in
`ptime.c`, and explain why placing it *after* the `fork()` would not work.

fflush(NULL) should fix it, but it has to go directly before fork() in ptime.c. It will clear any output waiting in the buffers before we create the child. If you placed it after the fork() the child would already have a copy of the output, causing both to print it. 

---

## Q2. Wall time is not CPU time (4 points)

Run both of these with your finished `ptime` and paste both reports:

```
./ptime ./workload/sleeper 1000
./ptime ./workload/spin 1000
```

**(a)** For each one, say which is larger — wall, or user+sys — and why.
./ptime ./workload/sleeper 1000 OUTPUT:
sleeper: slept 1000 ms
=== ptime ===
command : ./workload/sleeper 1000
pid     : 2001703
status  : exited 0
wall    : 1.003 s
user    : 0.001 s
sys     : 0.000 s

The reason wall time is larger than user + sys time here is that the program spends most of its time sleeping instead of using the CPU.

./ptime ./workload/spin 1000 OUTPUT: 
spin: burned 1000 ms of CPU (checksum 391680000)
=== ptime ===
command : ./workload/spin 1000
pid     : 2002024
status  : exited 0
wall    : 1.003 s
user    : 1.074 s
sys     : 0.000 s

User + sys is larger than wall for this one because spin is actively using the CPU 

**(b)** A program can finish with `user + sys` *greater* than `wall`. Describe
a program that would do that. (None of the provided workload programs on its own will do it; say what
kind of program would, and what hardware makes it possible. You can check your answer with
`./ptime sh -c "./workload/spin 500 & ./workload/spin 500 & wait"`.)

Its possible if the program used multiple processes or threads at the same time on different cores. This would cause the CPU times to be added together despite having the same amount of wall time.

---

## Q3. Which clock, and why (4 points)

You used `CLOCK_MONOTONIC`. `CLOCK_REALTIME` also exists, and it reports the
wall-clock time of day.

**(a)** Describe a concrete situation in which measuring an interval with
`CLOCK_REALTIME` would give a wrong answer — including one where the measured
duration comes out *negative*.

clock_realtime would give you the wrong answer if the system clock changed while the program runs. This would cause you to get a negative time if the clock is 12:00 and is changed to 11:59 causing the time calculated to be -60s. 

**(b)** Given that, why does `CLOCK_REALTIME` exist at all? Name one job it is
right for and `CLOCK_MONOTONIC` is wrong for.

clock_realtime is really good for logging since it show the current date and time. clock_monotonic would be bad at this task since it only measure time passed. 

---

## Q4. `_exit` versus `exit` in the child (4 points)

Your child process calls `_exit()` after a failed `execvp()` — never `exit()`,
and never `return`.

**(a)** What does `exit()` do that `_exit()` does not?

exit() flushes the stdio buffers before it ends, while _exit() does not and ends before cleanup. 

**(b)** Connect this to Q1. Suppose `ptime` also printed a line to stdout before `fork()` (a debug line, say). Describe the specific wrong output a student would see if they used `exit()` in the child and had also skipped the fix from Q1. Pasting a real run is the best answer.

kiernan@K:/mnt/c/Users/Kiern/Downloads/PA1_starter/PA1_starter$ ./ptime this_command_does_not_exist
ptime: cannot run 'this_command_does_not_exist': No such file or directory
Debug: before fork=== ptime ===
command : this_command_does_not_exist
pid     : 2006446
status  : exited 127
wall    : 0.017 s
user    : 0.000 s
sys     : 0.002 s
Debug: before forkkiernan@K:/mnt/c/Users/Kiern/Downloads/PA1_starter/PA1_starter$

line should have only been printed once in code, but appeared twice as shown here so this is the output they would see.  


**(c)** In a program larger than this one, why is `return` from `main()` in the
child worse still?

I mean _exit() is just a better way to end the child immediately, return doesn't guarantee the process ends if your inside another function.  

---

## Q5. Whose CPU time did you measure? (4 points)

You called `getrusage(RUSAGE_CHILDREN, &ru)` once, after `waitpid` returned.

**(a)** Suppose `ptime` were changed to run the command three times in a row,
calling `getrusage(RUSAGE_CHILDREN, ...)` after each one. What would the third
call report — that run's CPU time, or something else? Say precisely what.

The third call would report the total CPU time for all three children, rusage_children is the accumulated CPU usage of the children.

**(b)** Give a correct way to get *per-run* CPU time out of `RUSAGE_CHILDREN`
anyway.

You could save the current total and then call it, then just subtract the total time and from the current total and that should give you the per-run. 

**(c)** What would `getrusage(RUSAGE_SELF, ...)` have reported instead, and
roughly what number would you have seen in your report?

rusage_self reports the CPU time for ptime the number is very small like 0.000 something. 

---

## Optional: stretch features

If you implemented any STRETCH items, list which ones and give one example
command line plus its output for each. Extra credit is not awarded for stretch
work that is not documented here.

_Your answer:_
