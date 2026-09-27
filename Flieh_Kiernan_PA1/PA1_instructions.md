# CS 433 Operating Systems — Programming Assignment 1
## `ptime`: running another program, and finding out what happened to it

**CSU San Marcos · Fall 2026 · Dr. Reza Ramezan**

| | |
|---|---|
| **Released** | Friday, September 4, 2026 |
| **Due** | **Monday, September 28, 2026, 11:59 PM** |
| **Weight** | 6% of the course grade (one of five programming assignments worth 30% together) |
| **Work** | Alone, or in a fixed group of any size (see *Submitting*) |
| **Language** | C (C11) on **any Linux** — the course server, WSL, or a VM. See §7 |
| **Late work** | Not accepted. See *Deadlines*, below, and start early. |
| **Turn in** | Canvas, one `.tar.gz` named `LastName_FirstName_PA1.tar.gz` |

---

## 1. What you are building

A program called `ptime`. You give it a command; it runs that command as a
child process, waits for it to finish, and reports how the command ended and
how long it took.

```
$ ./ptime /bin/echo alpha beta gamma
alpha beta gamma
=== ptime ===
command : /bin/echo alpha beta gamma
pid     : 85747
status  : exited 0
wall    : 0.001 s
user    : 0.000 s
sys     : 0.001 s
```

It is about seventy lines of new code, all of it inside `main()`. The starter
kit hands you the output formatting and the fiddly struct-to-seconds
conversions, so that your time goes into the process lifecycle, which is the
actual subject.

Every later assignment assumes you can compile C with `gcc` on Linux, run the test
script, and submit a tarball on Canvas.

---

## 2. Learning objectives

| # | You will be able to… | Course outcome |
|---|---|---|
| 1 | Create a process with `fork()` and explain, precisely, what the two return values mean | **LO3** |
| 2 | Load a new program into a process with `execvp()` (replacing its memory image) and explain why a successful `exec` never returns | **LO3** |
| 3 | Wait for a child ("reap" it) with `waitpid()` and decode its termination status — normal exit versus death by signal | **LO3** |
| 4 | Use system calls (`fork`, `execvp`, `waitpid`, `clock_gettime`, `getrusage`) and describe where the user/kernel boundary sits in each | **LO2** |
| 5 | Distinguish wall-clock time from CPU time, and user CPU time from system CPU time, and explain what each one measures | **LO1, LO2** |
| 6 | Explain what `fork()` copies — including the parts of your process you did not know you had, like the stdio buffer | **LO1, LO3** |
| 7 | Write C that handles system-call failure instead of assuming success | **LO1** |

---

## 3. Why anyone cares

Creating a process (`fork()`) and choosing its program (`exec()`) are separate operations; between the two the child still runs your code, and that gap is where redirection and pipes live.

The operating system accounts, per process, for user CPU time and system CPU time, and neither is the same as wall time; `getrusage()` reads those accounting fields out of the process control block (PCB).

---

## 4. Specification

### 4.1 Command line

```
ptime COMMAND [ARG]...
```

Everything after `ptime` is the command and its arguments. `ptime` takes no
options of its own in the required part. Examples:

```
./ptime /bin/ls
./ptime /bin/ls -l /etc
./ptime ./workload/spin 500
./ptime sleep 2
```

With no arguments at all, print a usage line to stderr and exit 2.

### 4.2 The report

After the child finishes, print exactly seven lines **to stderr**:

```
=== ptime ===
command : /bin/echo alpha beta gamma
pid     : 85747
status  : exited 0
wall    : 0.001 s
user    : 0.000 s
sys     : 0.001 s
```

| Field | Meaning |
|---|---|
| `command` | The command and its arguments, joined by single spaces |
| `pid` | The process ID of the child you created |
| `status` | How the child ended — see below |
| `wall` | Elapsed real time from just before `fork()` to just after `waitpid()` returns, in seconds, three decimals |
| `user` | The child's user-mode CPU time, three decimals |
| `sys` | The child's kernel-mode CPU time, three decimals |

`status` takes one of two forms:

```
status  : exited 42
status  : signaled 11 (SIGSEGV)
```

The starter's `print_report()` already produces all of this. Do not rewrite it,
and do not change the spacing — the tests compare it character for character.

**The report goes to stderr, not stdout.** This matters and is graded. It is
what makes `ptime` composable:

```
$ ./ptime /bin/ls > files.txt     # files.txt holds ls's output, and nothing else
$ ./ptime ./build.sh 2> timing.log
```

If you put the report on stdout, you have contaminated your child's output
stream, and anyone piping `ptime` into another program gets garbage. `T03` in
the test suite exists for this.

### 4.3 Exit status of `ptime` itself

`ptime` reports the child's fate through its own exit code, the way a shell
does:

| Situation | `ptime` exits with |
|---|---|
| Child exited normally with code *N* | *N* |
| Child was killed by signal *S* | 128 + *S* |
| The command was not found (`ENOENT`) | 127 |
| The command was found but could not be executed | 126 |
| Usage error, or `fork`/`waitpid`/`clock_gettime`/`getrusage` failed | 2 |

The 126/127 split is the shell's own convention — try `bash -c 'nosuchthing';
echo $?` and then `bash -c '/etc/hosts'; echo $?` on the server and you will
see 127 and 126. The 128+signal rule is why `echo $?` shows 130 after you press
Ctrl-C: SIGINT is signal 2.

Check it yourself with `echo $?`:

```
$ ./ptime ./workload/statuser exit 42
...
$ echo $?
42
```

### 4.4 Handling failure

Every one of these must be handled, not assumed away:

- **`fork()` returns −1.** Print an error naming the call and the reason from
  `strerror(errno)`; exit 2. (It really happens on a shared server when
  somebody hits the per-user process limit.)
- **`execvp()` returns.** It only returns on failure. Save `errno`
  *immediately* — anything else you call may overwrite it — then print
  `ptime: cannot run 'NAME': REASON` to stderr and leave the child with
  `_exit(127)` for `ENOENT` or `_exit(126)` otherwise.
- **`waitpid()` fails with `EINTR`.** A signal arrived while you were waiting.
  That is not an error; retry the wait. Any other failure is fatal: report it
  and exit 2.
- **`clock_gettime()` or `getrusage()` fails.** Report and exit 2. These
  effectively never fail, and you should still check them; the habit is worth
  more than the branch.

Error messages go to stderr, start with `ptime: `, and say what failed and
why. `perror()` is acceptable; a bare `printf("error\n")` is not.

### 4.5 REQUIRED — the checklist

- [ ] `ptime COMMAND [ARG]...` runs the command as a **child process**, using
      `fork()` and `execvp()`. The parent does not exec.
- [ ] The parent waits for **that specific child** with `waitpid()`.
- [ ] The child's stdout and stderr pass through untouched.
- [ ] The seven-line report is printed to **stderr**, in the exact given format.
- [ ] `status` correctly distinguishes a normal exit from death by a signal,
      using `WIFEXITED` / `WEXITSTATUS` / `WIFSIGNALED` / `WTERMSIG`.
- [ ] `wall` comes from `clock_gettime(CLOCK_MONOTONIC, ...)`, sampled just
      before `fork()` and just after `waitpid()` returns.
- [ ] `user` and `sys` come from `getrusage(RUSAGE_CHILDREN, ...)`.
- [ ] `ptime`'s own exit code follows the table in §4.3.
- [ ] All stdio buffers are flushed immediately before `fork()` (order: read the clock, flush, fork — the flush is inside the timed interval and costs microseconds), with a comment
      in the code saying why.
- [ ] The child uses `_exit()`, never `exit()` and never `return`.
- [ ] All the failure cases in §4.4 are handled.
- [ ] Compiles with the Makefile's flags, `gcc -Wall -Wextra -pthread -std=c11 -O2` (i.e. `make clean && make`), and **zero warnings**.
- [ ] `make test` prints `all tests passed` (14 of 14).
- [ ] `analysis.md` is complete, in your own words, with real pasted output.
- [ ] The code is readable: consistent indentation, lines under about 100
      columns, no leftover debug printing, and a short comment at each place
      where the *why* is not obvious (the flush, `_exit`, the `EINTR` retry,
      the fact that `execvp` does not return on success).
- [ ] `README.md` is **yours** — your name(s), how to build, and anything you
      cited. The starter ships a `README.md` explaining the kit; replace it.

### 4.6 Where the 100 points are

| Section | Points | What it is |
|---|---|---|
| **A** Build and submission hygiene | 10 | Builds clean on the server; tarball named and laid out per §9; authorship and citations |
| **B** Process lifecycle | 30 | `fork`, `execvp`, `waitpid`, the report on stderr, the flush |
| **C** Timing and measurement | 15 | `clock_gettime` and `getrusage`, sampled in the right places |
| **D** Error handling and exit status | 15 | The `W` macros, the exit-code table in §4.3, the failure cases in §4.4 |
| **E** Code quality | 10 | Structure, formatting, comments that are correct |
| **F** Written analysis | 20 | The five questions in `analysis.md`, 4 points each |
| **Total** | **100** | |
| **S** Stretch | **+8 max** | §4.7 |

Two things worth knowing before you plan your time. Sections C, D and E come
to 40 points and *none* of them are fully covered by the test suite — a
submission that passes 14 of 14 can still lose most of them. And section F is
20 points for two pages of writing, which is the best points-per-hour rate in
the assignment.

### 4.7 STRETCH — extra credit, up to +8 points

Do these only after the required part passes all 14 tests. Each one must be
described in the "stretch features" section of `analysis.md`, with an example
command and its output, or it earns nothing. Partial credit is available.

**Read this before you spend an evening on it.** The four items below add up to
+10, but stretch credit on PA1 is **capped at +8**, and extra credit cannot
take this assignment above 100. So if your required part is already scoring in
the nineties, stretch work is mostly for the learning, not the points. That is
a fine reason to do it — S3 and S4 are real techniques — but decide with the
numbers in front of you.

**S1. `-n COUNT` — repeat and summarize (+2).** Run the command `COUNT` times
and, after the individual reports, print min, mean, and max wall time. There is
a real trap in here: `RUSAGE_CHILDREN` is *cumulative*. Question 5 of
`analysis.md` walks you into it on purpose.

**S2. `-m` — peak memory (+2).** Add a `maxrss` line reporting the child's peak
resident set size in kilobytes. Watch out: Linux reports `ru_maxrss` in
kilobytes and macOS reports it in bytes, so a laptop-tested version can be off
by 1024× on the server. Say in your write-up how you handled that.

**S3. `-t SECONDS` — a deadline (+3).** If the child is still alive after
`SECONDS`, kill it with `SIGKILL` and mark the report. Two things bite here:
your `SIGALRM` handler must do nothing but set a `volatile sig_atomic_t` flag,
and it must be installed with `sigaction()` *without* `SA_RESTART`, or
`waitpid()` will quietly resume instead of returning `EINTR` and your timeout
will never fire.

**S4. Tell a failed exec apart from a child that really exited 127 (+3).**
As specified, `ptime ./workload/statuser exit 127` and `ptime nosuchcommand`
both report `exited 127`, and the parent genuinely cannot tell them apart. Fix
it: create a `pipe()` before forking, set `FD_CLOEXEC` on the write end, have
the child write `errno` into it if `execvp` fails, and have the parent read it.
A successful `exec` closes the descriptor automatically and the parent's read
returns 0 bytes. This is how real shells do it.

---

## 5. Worked examples

Real runs, pasted unedited. The `pid`, the three timing values, and the `spin` checksum differ on your machine; everything else must match line for line and space for space.

**A command that succeeds**

```
$ ./ptime /bin/echo hello world
hello world
=== ptime ===
command : /bin/echo hello world
pid     : 85171
status  : exited 0
wall    : 0.002 s
user    : 0.001 s
sys     : 0.001 s
$ echo $?
0
```

**A command that fails with a code**

```
$ ./ptime ./workload/statuser exit 42
statuser: exiting with code 42
=== ptime ===
command : ./workload/statuser exit 42
pid     : 85176
status  : exited 42
wall    : 0.002 s
user    : 0.001 s
sys     : 0.001 s
$ echo $?
42
```

**A command that dies from a signal**

```
$ ./ptime ./workload/statuser signal 11
statuser: raising signal 11 on myself
=== ptime ===
command : ./workload/statuser signal 11
pid     : 85180
status  : signaled 11 (SIGSEGV)
wall    : 0.003 s
user    : 0.001 s
sys     : 0.001 s
$ echo $?
139
```

**A command that does not exist**

```
$ ./ptime no_such_command_xyz
ptime: cannot run 'no_such_command_xyz': No such file or directory
=== ptime ===
command : no_such_command_xyz
pid     : 85185
status  : exited 127
wall    : 0.002 s
user    : 0.000 s
sys     : 0.001 s
$ echo $?
127
```

**Sleeping costs wall time, not CPU time**

```
$ ./ptime ./workload/sleeper 800
sleeper: slept 800 ms
=== ptime ===
command : ./workload/sleeper 800
pid     : 85190
status  : exited 0
wall    : 0.810 s
user    : 0.001 s
sys     : 0.002 s
```

**Spinning costs both**

```
$ ./ptime ./workload/spin 500
spin: burned 500 ms of CPU (checksum 206720000)
=== ptime ===
command : ./workload/spin 500
pid     : 85203
status  : exited 0
wall    : 0.510 s
user    : 0.474 s
sys     : 0.010 s
```

**The report stays off stdout**

```
$ ./ptime ./workload/noisy > out.txt
noisy: this line is on stderr
=== ptime ===
command : ./workload/noisy
...
$ cat out.txt
noisy: this line is on stdout
```

`out.txt` must contain exactly one line.

---

## 6. The written analysis

`analysis.md` has five questions, 4 points each:

1. **The buffering trap.** Run `workload/buffer_trap` on a terminal and then piped into `cat`; the output differs and nothing in the program changed. Explain why, in terms of what `printf` does and what `fork()` copies.
2. **Wall time versus CPU time**, using your own `sleeper` and `spin` reports.
3. **`CLOCK_MONOTONIC` versus `CLOCK_REALTIME`**, including how the wrong one yields a *negative* duration.
4. **`_exit` versus `exit` in the child**, and how it connects back to Q1.
5. **`RUSAGE_CHILDREN` is cumulative**: whose CPU time did you actually measure?

Every lettered part is graded: two to four sentences each, with real pasted output. Budget about two hours.

---

## 7. Getting started: pick a Linux environment

You need **a Linux machine with `gcc`**. Any of these three is fine:

| Option | How you get it | Notes |
|---|---|---|
| **The course server** | `ssh YOUR_USERNAME@cs433.cs.csusm.edu` | Account details announced on Canvas. Off campus you may need the campus VPN — see the CSUSM IT pages. |
| **WSL** (Windows) | `wsl --install` in PowerShell, then `sudo apt install build-essential gdb` | The simplest route on Windows. |
| **A Linux VM** | VirtualBox, UTM or similar, any recent Ubuntu or Debian | Works on any host, including an Apple-silicon Mac. |

**macOS on its own is not one of the options.** You may develop there, but the
code must build and pass the tests on Linux before you submit; see the two differences below.

**Get the starter kit** from Canvas, unpack it, move it where you want to work, and build it:

```
cd ~
tar xzf PA1_starter.tar.gz       # wherever you put the download
mkdir -p ~/cs433
mv PA1_starter ~/cs433/pa1
cd ~/cs433/pa1
make
./ptime /bin/echo hello
```

The stub exits 2 and passes 1 of the 14 tests; that confirms the toolchain works.

**What is in the kit:**

```
PA1_starter/
├── PA1_instructions.md <- this handout
├── README.md           <- a tour of the kit; you replace this before submitting
├── Makefile
├── ptime.c             <- the only file you edit
├── analysis.md         <- the five written questions
├── workload/           <- spin.c sleeper.c statuser.c noisy.c buffer_trap.c
└── tests/              <- run_tests.sh, expected/, and a README explaining all 14
```

**Edit** with `nano`, `vim`, or `emacs` on the server, or locally and `scp ptime.c USER@cs433.cs.csusm.edu:~/cs433/pa1/`.

**macOS versus Linux.** Under `-std=c11`, glibc hides the POSIX functions (`fork`, `execvp`, `clock_gettime`); the `#define _POSIX_C_SOURCE 200809L` at the top of the starter asks for them back, so keep it above the `#include`s.
`struct rusage` fields and `strsignal()` text also differ; check any extra fields you use for stretch credit on Linux.

**Debugging tools.** Build with `-g -O0` for `gdb`; `strace -f ./ptime /bin/echo hi` shows the system calls (`clone`, `execve`, `wait4`, `getrusage`); `ps -ef --forest` shows the process tree.

---

## 8. Checking your own work

```
make clean && make      # must produce zero warnings
make test               # must print: all tests passed
```

`tests/README.md` explains what each of the 14 tests is looking for and which
specific mistake it catches. Read that table when something fails — it will
usually name your bug directly.

Passing all 14 does not guarantee full marks: error handling, code quality, and
`analysis.md` are read by a person. But every failing test is a guaranteed lost
point, and every one of them is fixable.

---

## 9. Submitting

**What to hand in.** One gzipped tar file containing exactly this:

```
LastName_FirstName_PA1/
├── ptime.c
├── Makefile
├── analysis.md
├── README.md          <- your names, how to build, anything the grader should know
│                         (replace the starter's README; do not just leave it)
├── workload/          <- source only, unchanged
│   ├── spin.c  sleeper.c  statuser.c  noisy.c  buffer_trap.c
├── tests/             <- unchanged, exactly as it came in the kit
└── AI_USE.md          <- ONLY if you used a generative AI tool; see §11
```

Leave `tests/` in, unchanged. `PA1_instructions.md` may stay or go; it is not graded.

**Make it:**

```
make clean                                  # no binaries, no .o files, no core dumps
cd ..
mv pa1 Ramezan_Reza_PA1                     # your name, not mine
tar czf Ramezan_Reza_PA1.tar.gz Ramezan_Reza_PA1
```

Make the tarball on Linux (on a Mac: `COPYFILE_DISABLE=1 tar --no-xattrs -czf ...`). Upload it to the PA1 assignment on Canvas, with your name exactly as it appears in Canvas: no spaces, no accents, no student ID.

**Sanity check.** Extract your own tarball somewhere else and build it:

```
rm -rf /tmp/check && mkdir /tmp/check && cd /tmp/check
tar xzf ~/cs433/Ramezan_Reza_PA1.tar.gz   # wherever `cd ..` above left it
cd Ramezan_Reza_PA1 && make && make test
```

This must end in `all tests passed`; if `make test` cannot find `tests/run_tests.sh`, `tests/` is missing.

**If you are working in a group.** A group submits **one** copy. Name the
directory after whichever member uploads it, and put **every** member's name in
`README.md`, in the header comment of `ptime.c`, and at the top of
`analysis.md`. Each of the other members uploads a single file called
`PARTNER.txt` containing one line:

```
Submitted by: Lastname, Firstname
```

Everyone in the group receives the same grade. Groups are fixed for the semester; changing later needs the instructor's approval in advance. Every member must be able to explain every line of the submission.

**Deadlines.** 11:59 PM on Monday, September 28, by the Canvas timestamp. No late work is accepted. Resubmit freely before the deadline; the last upload is graded.

---

## 10. Collaboration policy

- Discuss ideas freely: what `fork()` returns, why `execvp` does not come back, what a test is asking for.
- Never share, show, or send your code to another student, and never look at anyone else's code or any online solution, from any term or site. Sharing is a violation for the sharer as well as the copier.
- Cite anything that shaped your code (an answer, a post, a classmate's sketch): a comment line in the code and a line in `README.md`.
- Man pages, the textbook, the lecture slides, and the lecture code need no citation.
- Suspected violations go to the Dean of Students Office, per the syllabus.

---

## 11. Generative AI policy

AI coding tools (ChatGPT, Copilot, Claude, Gemini) are allowed. If you use one, submit `AI_USE.md` with all four of:

1. **Your prompts**: every prompt you sent about this assignment, verbatim.
2. **The raw output**, unedited (long transcripts trimmed to the parts that touched your code).
3. **What you changed and why**: specific, naming the call and the line, not "I fixed some bugs".
4. **A short reflection**: what the tool got right, what it got wrong, what you understand now.

Undisclosed AI use, or submitting AI output without your own analysis and changes, is academic dishonesty under the syllabus. The models reliably get `wait`/`_exit`/`fflush`/`RUSAGE` wrong here, and the tests and `analysis.md` catch it.

---

## 12. Common pitfalls

**1. The report on stdout.** Fails almost the whole suite and contaminates the child's output; see §4.2.

**2. Reading the wait status directly.** `status` is not the exit code; decode it with the `W` macros in §4.5.

**3. `execvp(argv[0], argv)` instead of `execvp(argv[1], &argv[1])`.** Execs `ptime` forever; `pkill -u $USER ptime` from a second login.

**4. Code after `execvp` that assumes it returns.** Anything after it runs only when the exec failed; see §4.4.

**5. No `fflush(NULL)` before `fork()`.** Buffered output is copied into the child and printed twice on a pipe or file; see §4.5, Q1.

**6. `exit()` in the child instead of `_exit()`.** `exit()` flushes the inherited buffers a second time; see §4.5.

**7. Measuring the wrong thing.** `clock()`, `time()`, and `RUSAGE_SELF` read 0 for the child; use `CLOCK_MONOTONIC` and `RUSAGE_CHILDREN`.

**8. End time sampled too early.** Read the clock after `waitpid()` returns, not after `fork()`, or `sleeper 1000` reports 0.000 s.

**9. Not saving `errno` right away.** Copy it on the line after the failed call, before any other call; see §4.4.

**10. Building only on macOS.** POSIX declarations, `ru_maxrss` units, and `strsignal()` text differ; test on Linux before you submit.

**11. `make: *** missing separator`.** Recipe lines need a real tab; a Windows-edited Makefile may have CRLF endings (run `dos2unix`).

**12. A tarball that does not build.** It scores zero; extract it into a fresh directory and run `make && make test` before you upload.

**13. Leaving `analysis.md` for the last hour.** It is 20 points, more than any two tests; write it while the code is fresh.

---


## Questions

Bring them to class, ask in office hours (on Canvas), or email gramezan@csusm.edu.
Ask after an hour stuck, not after days.
