#include <string.h>
#include <sys/ptrace.h>
#include <errno.h>

#include "utils.h"
#include "logger.h"

// Returns true if the target string contains the given prefix
bool has_prefix(char *target, char *prefix)
{
    return strncmp(target, prefix, strlen(prefix)) == 0;
}

// Wraps ptrace with error handling by checking the value of errno. Returns a PtraceResult
// which contains the value and whether the call succeeded.
ErrResult ptrace_with_error(enum __ptrace_request req, int pid, void * addr, void * data)
{
    bool success = true;
    errno = 0;
    int val = ptrace(req, pid, addr, data);
    if (errno != 0)
    {
        logger(ERROR, "ptrace failed: %s", strerror(errno));
        success = false;
    }
    ErrResult res = {.val = val, .success = success};
    return res;
}
