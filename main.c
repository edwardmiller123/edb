#include <unistd.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <errno.h>

#include "logger.h"

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("to few args\n");
        return -1;
    }

    const char *program = argv[1];

    int pid = fork();

    if (pid == 0)
    {
        // we are the child process so we should allow the parent to trace us
        // and start executing the program we wish to debug
        long ptrace_err = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        if (ptrace_err < 0)
        {
            logger(ERROR, "ptrace failed. ERRNO: %d\n", errno);
            return -1;
        }

        int exec_err = execl(program, program, (char *)NULL);
        if (exec_err < 0)
        {
            logger(ERROR, "failed to run %s. ERRNO: %d\n", program, errno);
            return -1;
        }
    }
    else if (pid >= 1)
    {
        // we are the parent process so begin debugging
        logger(INFO, "Started debugging process with pid %d", pid);
       
    }

    return 0;
}