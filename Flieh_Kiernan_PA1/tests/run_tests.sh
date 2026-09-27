#!/usr/bin/env bash
# ============================================================================
# CS 433 PA1 -- self-check test suite.
#
#   ./tests/run_tests.sh          # test the build in the current directory
#   ./tests/run_tests.sh DIR      # test the build in DIR
#   make test                     # same thing, from the Makefile
#
# This is the same suite the graders start from. Passing all 14 tests does not
# guarantee full marks -- code quality, error handling and analysis.md are
# graded by a human -- but FAILING one is a guaranteed lost point, so run it
# before you submit.
#
# Timing tests compare against generous ranges, because a shared login server
# is noisy. If a timing test fails twice in a row on an idle server, the bug
# is yours, not the server's.
# ============================================================================
set -u

BUILD_DIR="${1:-.}"
TEST_DIR="$(cd "$(dirname "$0")" && pwd)"
EXPECT_DIR="$TEST_DIR/expected"

cd "$BUILD_DIR" || { echo "cannot cd to $BUILD_DIR"; exit 1; }
BUILD_ABS="$(pwd)"

PTIME="$BUILD_ABS/ptime"
WL="$BUILD_ABS/workload"

pass=0
fail=0
failed_names=""

TMP="$(mktemp -d "${TMPDIR:-/tmp}/pa1_tests.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT

# ---------------------------------------------------------------- helpers --

ok()   { pass=$((pass+1)); printf '  PASS  %s\n' "$1"; }
bad()  { fail=$((fail+1)); failed_names="$failed_names\n    $1";
         printf '  FAIL  %s\n        %s\n' "$1" "$2"; }

# Replace the pid and the three timing values with placeholders so a report
# can be compared exactly. Everything else must match character for character.
normalize() {
    sed -E -e 's/^(pid +): [0-9]+$/\1: <PID>/' \
           -e 's/^((wall|user|sys) +): [0-9]+\.[0-9]{3} s$/\1: <TIME> s/'
}

# Pull one numeric field out of a report:  field_of wall FILE
field_of() {
    awk -v k="$1" '$1 == k && $2 == ":" { print $3; exit }' "$2"
}

# ge A B  -> true when A >= B, using awk because sh cannot compare floats
ge() { awk -v a="$1" -v b="$2" 'BEGIN { exit !(a+0 >= b+0) }'; }
le() { awk -v a="$1" -v b="$2" 'BEGIN { exit !(a+0 <= b+0) }'; }

count_lines() { grep -c -- "$1" "$2" 2>/dev/null || true; }

# ------------------------------------------------------------ preflight ----

echo "CS 433 PA1 self-check"
echo "build directory: $BUILD_ABS"
echo

missing=0
for prog in "$PTIME" "$WL/spin" "$WL/sleeper" "$WL/statuser" "$WL/noisy"; do
    if [ ! -x "$prog" ]; then
        echo "MISSING: $prog"
        missing=1
    fi
done
if [ "$missing" -ne 0 ]; then
    echo
    echo "Run 'make' first. All of ptime and workload/* must be built."
    exit 1
fi

# ------------------------------------------------------------- the tests --

# T01 -- no command at all is a usage error, and ptime must not exit 0.
"$PTIME" >"$TMP/o1" 2>"$TMP/e1"
rc=$?
if [ "$rc" -eq 2 ] && grep -qi "usage" "$TMP/e1"; then
    ok "T01 no arguments -> usage message, exit 2"
else
    bad "T01 no arguments -> usage message, exit 2" \
        "got exit $rc; stderr was: $(head -1 "$TMP/e1")"
fi

# T02 -- the child's stdout must reach ptime's stdout unchanged.
"$PTIME" "$WL/noisy" >"$TMP/o2" 2>"$TMP/e2"
rc=$?
if [ "$rc" -eq 0 ] && diff -u "$EXPECT_DIR/noisy_stdout.txt" "$TMP/o2" >"$TMP/d2" 2>&1
then
    ok "T02 child stdout passes through unchanged"
else
    bad "T02 child stdout passes through unchanged" \
        "exit $rc; stdout diff: $(head -5 "$TMP/d2" | tr '\n' ' ')"
fi

# T03 -- the report belongs on stderr. Redirecting stdout must capture ONLY
# the child's output. This is the single most commonly failed requirement.
if ! grep -q "=== ptime ===" "$TMP/o2" && grep -q "=== ptime ===" "$TMP/e2" \
   && grep -q "noisy: this line is on stderr" "$TMP/e2"; then
    ok "T03 report goes to stderr, never to stdout"
else
    bad "T03 report goes to stderr, never to stdout" \
        "your report appears on stdout, or is missing from stderr"
fi

# T04 -- exact report format for a simple command with several arguments.
"$PTIME" /bin/echo alpha beta gamma >"$TMP/o4" 2>"$TMP/e4"
rc=$?
normalize <"$TMP/e4" >"$TMP/n4"
if [ "$rc" -eq 0 ] && diff -u "$EXPECT_DIR/report_echo.txt" "$TMP/n4" >"$TMP/d4" 2>&1
then
    ok "T04 report format is exact"
else
    bad "T04 report format is exact" \
        "exit $rc; diff (expected vs yours): $(sed -n '4,12p' "$TMP/d4" | tr '\n' '|')"
fi

# T05 -- a non-zero exit code must be both reported and propagated.
"$PTIME" "$WL/statuser" exit 42 >"$TMP/o5" 2>"$TMP/e5"
rc=$?
if [ "$rc" -eq 42 ] && grep -q "^status  : exited 42$" "$TMP/e5"; then
    ok "T05 child exit 42 -> reported as 'exited 42', ptime exits 42"
else
    bad "T05 child exit 42 -> reported as 'exited 42', ptime exits 42" \
        "got exit $rc; status line: $(grep '^status' "$TMP/e5")"
fi

# T06 -- and exit 0 must stay 0.
"$PTIME" "$WL/statuser" exit 0 >"$TMP/o6" 2>"$TMP/e6"
rc=$?
if [ "$rc" -eq 0 ] && grep -q "^status  : exited 0$" "$TMP/e6"; then
    ok "T06 child exit 0 -> reported as 'exited 0', ptime exits 0"
else
    bad "T06 child exit 0 -> reported as 'exited 0', ptime exits 0" \
        "got exit $rc; status line: $(grep '^status' "$TMP/e6")"
fi

# T07 -- death by SIGTERM. Not an exit code: a different branch of the
# wait status entirely, and ptime must exit 128 + 15 = 143.
"$PTIME" "$WL/statuser" signal 15 >"$TMP/o7" 2>"$TMP/e7"
rc=$?
if [ "$rc" -eq 143 ] && grep -q "^status  : signaled 15 (SIGTERM)$" "$TMP/e7"; then
    ok "T07 child killed by SIGTERM -> 'signaled 15 (SIGTERM)', ptime exits 143"
else
    bad "T07 child killed by SIGTERM -> 'signaled 15 (SIGTERM)', ptime exits 143" \
        "got exit $rc; status line: $(grep '^status' "$TMP/e7")"
fi

# T08 -- same branch, different signal. Catches hard-coded SIGTERM.
"$PTIME" "$WL/statuser" signal 11 >"$TMP/o8" 2>"$TMP/e8"
rc=$?
if [ "$rc" -eq 139 ] && grep -q "^status  : signaled 11 (SIGSEGV)$" "$TMP/e8"; then
    ok "T08 child killed by SIGSEGV -> 'signaled 11 (SIGSEGV)', ptime exits 139"
else
    bad "T08 child killed by SIGSEGV -> 'signaled 11 (SIGSEGV)', ptime exits 139" \
        "got exit $rc; status line: $(grep '^status' "$TMP/e8")"
fi

# T09 -- exec failure, ENOENT. Must not hang, must not exit 0.
"$PTIME" definitely_not_a_real_command_433 >"$TMP/o9" 2>"$TMP/e9"
rc=$?
if [ "$rc" -eq 127 ] && grep -q "cannot run" "$TMP/e9"; then
    ok "T09 command not found -> error message, exit 127"
else
    bad "T09 command not found -> error message, exit 127" \
        "got exit $rc; stderr: $(head -1 "$TMP/e9")"
fi

# T10 -- exec failure, EACCES. The file exists; it is just not executable.
# NOTE: the file is mode 644 (no execute bit at all), so even root gets EACCES
# and T10 passes as root -- verified on gcc 11 / glibc. Root would reach the
# ENOEXEC -> /bin/sh -> 127 path only if some x bit were set, which T10 never sets.
printf 'not a program\n' >"$TMP/notexec"
chmod 644 "$TMP/notexec"
"$PTIME" "$TMP/notexec" >"$TMP/o10" 2>"$TMP/e10"
rc=$?
if [ "$rc" -eq 126 ] && grep -q "cannot run" "$TMP/e10"; then
    ok "T10 file exists but is not executable -> exit 126"
else
    bad "T10 file exists but is not executable -> exit 126" \
        "got exit $rc; stderr: $(head -1 "$TMP/e10")"
fi

# T11 -- a sleeping child burns wall-clock time but almost no CPU. If your
# 'user' number tracks wall time, you measured the wrong thing.
"$PTIME" "$WL/sleeper" 800 >"$TMP/o11" 2>"$TMP/e11"
rc=$?
w=$(field_of wall "$TMP/e11"); u=$(field_of user "$TMP/e11"); s=$(field_of sys "$TMP/e11")
cpu=$(awk -v a="${u:-0}" -v b="${s:-0}" 'BEGIN { printf "%.3f", a+b }')
if [ "$rc" -eq 0 ] && ge "${w:-0}" 0.60 && le "${w:-99}" 4.00 && le "$cpu" 0.30; then
    ok "T11 sleeper 800: wall ~0.8 s, CPU ~0 (wall=$w cpu=$cpu)"
else
    bad "T11 sleeper 800: wall ~0.8 s, CPU ~0" \
        "exit $rc; wall=$w user=$u sys=$s (want 0.60<=wall<=4.00 and user+sys<=0.30)"
fi

# T12 -- a spinning child burns real user CPU. If your 'user' number is 0.000
# here, you never called getrusage, or you read RUSAGE_SELF by mistake.
"$PTIME" "$WL/spin" 500 >"$TMP/o12" 2>"$TMP/e12"
rc=$?
w=$(field_of wall "$TMP/e12"); u=$(field_of user "$TMP/e12")
if [ "$rc" -eq 0 ] && ge "${u:-0}" 0.20 && ge "${w:-0}" 0.40; then
    ok "T12 spin 500: user CPU is real (wall=$w user=$u)"
else
    bad "T12 spin 500: user CPU is real" \
        "exit $rc; wall=$w user=$u (want user>=0.20 and wall>=0.40)"
fi

# T13 -- with stdout on a pipe it is block-buffered, so anything you printf
# before fork() gets duplicated. Nothing may appear twice, and the report
# must still stay off stdout.
( "$PTIME" "$WL/noisy" 2>"$TMP/e13" ) | cat >"$TMP/o13"
n_out=$(count_lines "noisy: this line is on stdout" "$TMP/o13")
n_rep=$(count_lines "=== ptime ===" "$TMP/o13")
if [ "${n_out:-0}" -eq 1 ] && [ "${n_rep:-0}" -eq 0 ]; then
    ok "T13 piped stdout: nothing duplicated, report still off stdout"
else
    bad "T13 piped stdout: nothing duplicated, report still off stdout" \
        "child stdout line appeared $n_out time(s), report appeared $n_rep time(s) on stdout"
fi

# T14 -- every argument must reach the child, in order, and the command line
# in the report must show them all.
"$PTIME" /bin/echo one two three four five >"$TMP/o14" 2>"$TMP/e14"
rc=$?
if [ "$rc" -eq 0 ] && [ "$(cat "$TMP/o14")" = "one two three four five" ] \
   && grep -q "^command : /bin/echo one two three four five$" "$TMP/e14"; then
    ok "T14 all arguments reach the child and appear in the report"
else
    bad "T14 all arguments reach the child and appear in the report" \
        "exit $rc; stdout='$(cat "$TMP/o14")'; command line: $(grep '^command' "$TMP/e14")"
fi

# ---------------------------------------------------------------- summary --

echo
echo "-----------------------------------------------"
printf 'passed %d, failed %d, of %d\n' "$pass" "$fail" "$((pass+fail))"
if [ "$fail" -ne 0 ]; then
    printf 'failing tests:%b\n' "$failed_names"
    echo "-----------------------------------------------"
    exit 1
fi
echo "all tests passed"
echo "-----------------------------------------------"
exit 0
