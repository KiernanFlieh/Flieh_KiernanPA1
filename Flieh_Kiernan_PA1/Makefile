# CS 433 PA1 -- Makefile. Works with GNU make (Linux server) and BSD make.
# Deliberately written without GNU-only features so it behaves the same
# everywhere. Do not remove any warning flag.

CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -std=c11 -O2

WORKLOAD = workload/spin workload/sleeper workload/statuser \
           workload/noisy workload/buffer_trap

all: ptime $(WORKLOAD)

ptime: ptime.c
	$(CC) $(CFLAGS) -o ptime ptime.c

workload/spin: workload/spin.c
	$(CC) $(CFLAGS) -o workload/spin workload/spin.c

workload/sleeper: workload/sleeper.c
	$(CC) $(CFLAGS) -o workload/sleeper workload/sleeper.c

workload/statuser: workload/statuser.c
	$(CC) $(CFLAGS) -o workload/statuser workload/statuser.c

workload/noisy: workload/noisy.c
	$(CC) $(CFLAGS) -o workload/noisy workload/noisy.c

workload/buffer_trap: workload/buffer_trap.c
	$(CC) $(CFLAGS) -o workload/buffer_trap workload/buffer_trap.c

# Run the self-check suite. The starter kit ships tests/ next to this
# Makefile; ../tests/ is accepted too, in case you moved things around.
#
# We invoke the script through `bash` rather than running it directly, so that
# a lost execute bit (which happens routinely when a kit is downloaded as a
# zip, or copied through a Windows filesystem) is not mistaken for a missing
# file.
test: all
	@if [ -f tests/run_tests.sh ]; then \
	    bash tests/run_tests.sh . ; \
	elif [ -f ../tests/run_tests.sh ]; then \
	    bash ../tests/run_tests.sh . ; \
	else \
	    echo "tests/run_tests.sh not found."; \
	    echo "The starter kit ships a tests/ directory next to this Makefile."; \
	    echo "If yours is missing, download the kit again from Canvas."; \
	    exit 1; \
	fi

clean:
	rm -f ptime $(WORKLOAD)
	rm -rf ptime.dSYM workload/*.dSYM
	rm -f core core.*

.PHONY: all test clean
