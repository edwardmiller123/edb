#include <unistd.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <sys/personality.h>

#include "logger.h"
#include "debugger.h"

int main(int argc, char *argv[])
{
    set_log_level(INFO);

    if (argc < 2)
    {
        logger(ERROR, "Must specify executable to debug");
        return -1;
    }

    const char *program = argv[1];

    int pid = fork();

    if (pid == 0)
    {
        // we are the child process we should allow the parent to trace us
        // and start executing the program we wish to debug

        int last_persona = personality(ADDR_NO_RANDOMIZE);
        if (last_persona < 0) {
            logger(ERROR, "Failed to set child personaility. ERRNO: %d\n", errno);
            return -1;
        }

        long ptrace_err = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        if (ptrace_err < 0)
        {
            logger(ERROR, "Ptrace failed. ERRNO: %d\n", errno);
            return -1;
        }

        int exec_err = execl(program, program, (char *)NULL);
        if (exec_err < 0)
        {
            logger(ERROR, "Failed to run %s. ERRNO: %d\n", program, errno);
            return -1;
        }
    }
    else if (pid >= 1)
    {
        Debugger * debugger = new_debugger(pid);
        // we are the parent process so begin debugging
        logger(INFO, "Started debugging process with pid %d", pid);
        int debug_err = run_debugger(debugger);
        if (debug_err < 0) {
            logger(ERROR, "Debugger exited. ERRNO: %d\n", errno);
            return -1;
        }
       
    }

    return 0;
}