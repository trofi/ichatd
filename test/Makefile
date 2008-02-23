# -*- Makefile -*-
.PHONY: test

CFLAGS := -MMD -W -Wall -g -O0

test: run-tests
run-tests: compile-tests

#todo autogen targets
run-tests:      run-rc4
compile-tests: test-rc4

#rc4
test-rc4: test-rc4.c ../src/rc4.c
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

run-rc4: test-rc4
	./$<

#buffer
run-tests:      run-buffer
compile-tests: test-buffer

test-buffer: test-buffer.c ../src/buffer.c
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

run-buffer: test-buffer
	./$<
