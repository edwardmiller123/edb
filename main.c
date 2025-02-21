#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <sys/personality.h>
#include <string.h>

#include "logger.h"
#include "debugger.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    set_log_level(DEBUG);

    char *prog = NULL;

    if (argc >= 2)
    {
        prog = argv[1];;
    }

    Debugger * db = new_debugger();

    int res = run_cmd_loop(db, prog);
    if (res == -1) {
        logger(ERROR, "Failed to rum command loop");
    }

    free(db);

    return 0;
}