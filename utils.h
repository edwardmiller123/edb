#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

// Returns true if the target string contains the given prefix
bool has_prefix(char *prefix, char *target);

typedef struct PTraceResult {
	int val;
	bool success;
} PTraceResult;

// Wraps ptrace with error handling by checking the value of errno. Returns a PtraceResult
// which conrtains the value and whether the call succeeded.
PTraceResult ptrace_with_error(enum __ptrace_request req, int pid, int addr, int data);

#endif