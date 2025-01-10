#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

// Returns true if the target string contains the given prefix
bool has_prefix(char *prefix, char *target);

// helper struct to allow errors to be returned if the return value
// could be negative
typedef struct ErrResult {
	int val;
	bool success;
} ErrResult;

// Wraps ptrace with error handling by checking the value of errno. Returns a PtraceResult
// which conrtains the value and whether the call succeeded.
ErrResult ptrace_with_error(enum __ptrace_request req, int pid, void * addr, void * data);

#endif