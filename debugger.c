#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/personality.h>

#include "logger.h"
#include "breakpoint.h"
#include "debugger.h"
#include "map.h"
#include "utils.h"
#include "reg.h"

#define MAX_LINE_SIZE 64
#define MAX_PROG_NAME_SIZE 32
#define MAX_COMMAND_PARTS 5
#define MAX_PART_SIZE 32
#define WAIT_OPTIONS 0

// The magic number to exit the program
#define EXIT -73

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

Debugger *new_debugger()
{
	Debugger *debugger = (Debugger *)malloc(sizeof(Debugger));
	if (debugger == NULL)
	{
		logger(ERROR, "Failed to allocate heap memory for debugger. %s", strerror(errno));
		return NULL;
	}

	Map *break_points = new_map();
	debugger->break_points = break_points;
	debugger->session = NULL;
	return debugger;
}

// Creates a new break point. Returns 1 if the max number of break points has
// already been reached. Returns -1 for errors.
int add_break_point(Debugger *db, char *cmd_arg)
{
	if (db->session == NULL)
	{
		logger(WARN, "No active debugging session.");
		return 0;
	}

	if (m_is_full(db->break_points))
	{
		return 1;
	}

	BreakPointType bp_type = LINE;
	int base = 10;
	if (has_prefix(cmd_arg, "0x"))
	{
		bp_type = ADDR;
		base = 16;
	}

	unsigned int pos = strtoll(cmd_arg, NULL, base);

	BreakPoint *bp = new_bp(db->session->pid, bp_type, pos);
	if (bp == NULL)
	{
		logger(ERROR, "failed to create break point %s", cmd_arg);
		return -1;
	}

	// use the line or address of the bp as the key for the map.
	m_set(db->break_points, cmd_arg, (void *)bp);

	int enable_ret = enable(bp);
	if (enable_ret < 0)
	{
		logger(ERROR, "failed to enable breakpoint: %s", cmd_arg);
		return -1;
	}
	return 0;
}

// Removes the given break point
int remove_break_point(Debugger *db, char *cmd_arg)
{
	if (db->session == NULL)
	{
		logger(WARN, "No active debugging session.");
		return 0;
	}

	if (m_is_empty(db->break_points))
	{
		return 1;
	}
	BreakPoint *bp = (BreakPoint *)m_get(db->break_points, cmd_arg);
	if (bp == NULL)
	{
		logger(ERROR, "Breakpoint not found.");
		return 1;
	}

	int res = disable(bp);
	if (res < 0)
	{
		logger(ERROR, "Failed to disable breakpoint %s.", cmd_arg);
		return -1;
	}

	m_remove(db->break_points, cmd_arg);

	free(bp);
	return 0;
}

// Checks the current instruction for a break point and steps over it if one exists
int step_over_breakpoint(Debugger *db)
{
	// use the instruction pointer to check for break points and if one is found
	// we temporarily disable it.

	void *next_instruction_addr = get_ip(db->session->pid);
	if (next_instruction_addr == NULL)
	{
		logger(ERROR, "Failed to get instruction pointer");
		return -1;
	}

	void *current_instruction_addr = next_instruction_addr - 1;

	logger(DEBUG, "Checking for breakpoint at address %p. RIP at %p", current_instruction_addr, next_instruction_addr);

	// check for break point at that address
	char bp_key[MAX_KEY_SIZE];
	sprintf(bp_key, "%p", current_instruction_addr);

	BreakPoint *bp = (BreakPoint *)m_get(db->break_points, bp_key);

	if (bp == NULL)
	{
		return 0;
	}

	if (!bp->enabled)
	{
		return 0;
	}

	logger(DEBUG, "Found breakpoint: %s", bp_key);

	int set_ip_res = set_ip(db->session->pid, current_instruction_addr);
	if (set_ip_res == -1)
	{
		logger(ERROR, "failed to set instruction pointer");
		return -1;
	}

	logger(DEBUG, "RIP set to %p", current_instruction_addr);

	int dis_res = disable(bp);
	if (dis_res == -1)
	{
		logger(ERROR, "failed to disable breakpoint %s", bp_key);
		return -1;
	}

	ErrResult step_res = ptrace_with_error(PTRACE_SINGLESTEP, db->session->pid, NULL, NULL);
	if (!step_res.success)
	{
		logger(ERROR, "failed stepping to next instruction");
		return -1;
	}

	int pid_res = waitpid(db->session->pid, &db->session->wait_status, WAIT_OPTIONS);
	if (pid_res == -1)
	{
		logger(ERROR, "failed to wait for process %d. %s", db->session->pid, strerror(errno));
		return -1;
	}

	int en_res = enable(bp);
	if (en_res == -1)
	{
		logger(ERROR, "failed to re-enable breakpoint %s", bp_key);
		return -1;
	}

	return 0;
}

// Restarts a paused process
int continue_execution(Debugger *db)
{
	if (db->session == NULL || (db->session != NULL && !db->session->active))
	{
		logger(WARN, "No active debugging session.");
		return 0;
	}

	int step_err = step_over_breakpoint(db);
	if (step_err == -1)
	{
		logger(ERROR, "failed to step over breakpoints");
		return -1;
	}

	ErrResult cont_res = ptrace_with_error(PTRACE_CONT, db->session->pid, NULL, NULL);
	if (!cont_res.success)
	{
		return -1;
	}

	int pid_result = waitpid(db->session->pid, &db->session->wait_status, WAIT_OPTIONS);
	if (pid_result == -1)
	{
		logger(ERROR, "failed to wait for process %d. %s", db->session->pid, strerror(errno));
		return -1;
	}
	return 0;
}

// Starts a new debugging session by forking the current process and running the given executable.
int start_debug_session(Debugger *db, char *prog)
{
	if (prog == NULL)
	{
		logger(WARN, "No executable provided.");
		return 0;
	}

	int pid = fork();
	if (pid == -1)
	{
		logger(ERROR, "Failed to fork. ERRNO: %d\n", errno);
		return -1;
	}

	if (pid == 0)
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

	// Since prog still potentially points to the old session at this point need 
	// to create the new session before freeing the old
	// one so we dont accidently store garbage
	DebugSession *dbs = new_debug_session(prog, pid);

	if (db->session != NULL) {
		logger(DEBUG, "Clearing debug session for program %s. PID: %d", db->session->prog, db->session->pid);
		remove_debug_session(db->session);
	}

	db->session = dbs;
	db->session->active = true;

	// we are the parent process so begin debugging
	logger(INFO, "Debug session started for executable %s. Session PID: %d.", db->session->prog, pid);

	// wait until child process is executing
	int pid_result = waitpid(pid, &db->session->wait_status, WAIT_OPTIONS);
	if (pid_result < 0)
	{
		logger(ERROR, "failed to wait for process %d. %s", db->session->pid, strerror(errno));
		return -1;
	}
	return 0;
}

int run(Debugger *db, char *prog)
{
	char *prog_to_run = NULL;

	if (strcmp(prog, "") == 0 && db->session == NULL)
	{
		logger(WARN, "No executable provided.");
		return 0;
	}
	else if (strcmp(prog, "") == 0)
	{
		prog_to_run = db->session->prog;
	}
	else
	{
		prog_to_run = prog;
	}

	return start_debug_session(db, prog_to_run);
}

// Parses and runs the given command. Returns 1 if the command was not recognised
// and -1 for errors.
int parse_cmd(Debugger *db, char *input)
{
	logger(DEBUG, "Parsing command: %s", input);

	char command_parts[MAX_COMMAND_PARTS][MAX_PART_SIZE];

	// zero the allocated memory
	for (int i = 0; i < MAX_COMMAND_PARTS; i++)
	{
		memset(command_parts[i], 0, MAX_PART_SIZE);
	}

	int j = 0;
	int part_idx = 0;
	for (int i = 0; i < strlen(input); i++)
	{
		if (input[i] == ' ' || input[i] == '\n')
		{
			command_parts[part_idx][j] = '\0';
			part_idx++;
			j = 0;
		}
		else
		{
			command_parts[part_idx][j] = input[i];
			j++;
		}
	}
	char *base_command = command_parts[0];
	if (strcmp(base_command, "") == 0)
	{
		return 0;
	}

	char *first_arg = command_parts[1];

	if (has_prefix(base_command, "c"))
	{
		return continue_execution(db);
	}

	if (has_prefix(base_command, "b"))
	{
		return add_break_point(db, first_arg);
	}

	if (has_prefix(base_command, "del"))
	{
		return remove_break_point(db, first_arg);
	}

	if (has_prefix(base_command, "run"))
	{
		return run(db, first_arg);
	}

	if (has_prefix(base_command, "q"))
	{
		logger(INFO, "Exiting.");
		return EXIT;
	}
	return 1;
}

// starts the main debugging loop.
int run_cmd_loop(Debugger *db, const char *prog)
{
	// intially try to debug the given program
	int dbs_res = start_debug_session(db, (char *)prog);
	switch (dbs_res)
	{
	case -1:
		logger(ERROR, "Failed to start debug session for executable %s.", prog);
		break;
	case EXIT:
		return 0;
	}

	char current_line[MAX_LINE_SIZE];

	do
	{
		// exit if the child process terminates
		if (db->session != NULL && db->session->wait_status == 0 && db->session->active)
		{
			logger(INFO, "Debug session for executable %s has terminated. Session PID: %d.", db->session->prog, db->session->pid);
			db->session->active = false;
		}

		fputs("edb> ", stdout);
		// fgets will stop hanging when either a \n or a EOF is found
		fgets(current_line, MAX_LINE_SIZE, stdin);

		// Run the command but dont exit on failure
		int cmd_result = parse_cmd(db, current_line);
		switch (cmd_result)
		{
		case 1:
			logger(WARN, "Command not recognised: %s", current_line);
			break;
		case -1:
			logger(ERROR, "Failed to run command: %s", current_line);
			break;
		case EXIT:
			return 0;
		default:
			break;
		}
	} while (current_line != NULL);

	logger(ERROR, "Failed to read line", strerror(errno));
	return -1;
}