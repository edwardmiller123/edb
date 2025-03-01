#include <stdio.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/personality.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "session.h"
#include "logger.h"
#include "utils.h"

#define MAX_PROG_NAME_SIZE 32

DebugSession *new_debug_session(char *prog, int pid)
{
	DebugSession *dbs = (DebugSession *)malloc(sizeof(DebugSession));
	if (dbs == NULL)
	{
		logger(ERROR, "Failed to allocate heap memory for debug session. %s", strerror(errno));
		return NULL;
	}

	char *prog_name_buf = (char *)malloc(MAX_PROG_NAME_SIZE);
	if (prog_name_buf == NULL)
	{
		logger(ERROR, "Failed to allocate heap memory for program name. %s", strerror(errno));
		return NULL;
	}

	memcpy(prog_name_buf, prog, MAX_PROG_NAME_SIZE);
	dbs->prog = prog_name_buf;

	dbs->pid = pid;
	dbs->wait_status = 0;
	return dbs;
}

void remove_debug_session(DebugSession *session)
{
	free(session->prog);
	free(session);
}

int start_tracing(char *prog)
{
	// we are the child process we should allow the parent to trace us
	// and start executing the program we wish to debug.

	// We return the EXIT code on any errors so that the child process terminates and
	// isnt left hanging around.

	int last_persona = personality(ADDR_NO_RANDOMIZE);
	if (last_persona < 0)
	{
		logger(ERROR, "Failed to set child personaility. ERRNO: %d\n", errno);
		return EXIT;
	}

	ErrResult trace_res = ptrace_with_error(PTRACE_TRACEME, 0, NULL, NULL);
	if (!trace_res.success)
	{
		logger(ERROR, "Failed to start tracing child.");
		return EXIT;
	}

	int exec_err = execl(prog, prog, NULL);
	if (exec_err < 0)
	{
		logger(ERROR, "Failed to execute %s. %s\n", prog, strerror(errno));
		return EXIT;
	}
	return EXIT;
}

// TODO:
//	- Load first part of elf into memory
//	- Look up section header table and load (some sections) into memory
// 	- find .debug_line section by cross referencing with header name string table
// 	- load .debug_line section into memory and parse to create map of line numbers to memory addresses

int parse_dwarf_info(DebugSession * session)
{
}