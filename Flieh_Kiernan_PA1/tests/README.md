# PA1 self-check tests

Fourteen tests. Run them before you submit; they are the same checks the
graders start from.

```
cd /path/to/your/PA1
make test                     # or: ./tests/run_tests.sh .
```

Output ends in either `all tests passed` or a list of what failed, with the
value you produced next to the value expected.

## What each test is actually checking

| # | Test | The mistake it catches |
|---|---|---|
| T01 | no arguments → usage, exit 2 | Crashing, or running `execvp(NULL, ...)` |
| T02 | child stdout passes through | Capturing or mangling the child's output |
| T03 | report is on stderr, not stdout | The single most common failure — see PA1_instructions.md §12, pitfall 1 |
| T04 | exact report format | Changed `print_report`, wrong precision, wrong field order |
| T05 | child exit 42 → `exited 42`, ptime exits 42 | Reading the raw wait status instead of using `WEXITSTATUS` |
| T06 | child exit 0 → `exited 0`, ptime exits 0 | Always returning 0, or always returning the status word |
| T07 | SIGTERM → `signaled 15 (SIGTERM)`, exit 143 | Only handling the `WIFEXITED` branch |
| T08 | SIGSEGV → `signaled 11 (SIGSEGV)`, exit 139 | Hard-coding SIGTERM in the signal branch |
| T09 | missing command → exit 127 | Ignoring the return value of `execvp` |
| T10 | non-executable file → exit 126 | Treating every exec failure as "not found" |
| T11 | `sleeper 800`: wall ≈ 0.8 s, CPU ≈ 0 | Reporting wall time in the CPU fields |
| T12 | `spin 500`: user CPU is real | `RUSAGE_SELF` instead of `RUSAGE_CHILDREN`, or no `getrusage` at all |
| T13 | piped stdout is not duplicated | A stray `printf` to stdout before `fork()` without flushing. (A missing `fflush(NULL)` in an otherwise-clean `ptime` is NOT caught by any test; it is graded by reading the code.) |
| T14 | all arguments reach the child | Passing only `argv[1]`, or rebuilding the vector wrong |

## Files

- `run_tests.sh` — the runner. Takes an optional build directory (default `.`).
- `expected/report_echo.txt` — the exact report for `ptime /bin/echo alpha beta gamma`,
  with the pid and the three timing values replaced by `<PID>` and `<TIME>`.
  T04 compares your normalized report against this, character for character.
- `expected/noisy_stdout.txt` — what `workload/noisy` writes to stdout. T02
  compares your stdout against this.

## About the timing tests

T11 and T12 use wide ranges on purpose, because `cs433.cs.csusm.edu` is a
shared machine and everything is slower the night before a deadline. If a
timing test fails twice in a row when the server is quiet, the problem is in
your code, not in the server.

Passing all 14 does not mean full marks. Error handling, code quality and
`analysis.md` are read by a human. But failing one is a guaranteed lost point,
and every one of them is fixable in under an hour.
