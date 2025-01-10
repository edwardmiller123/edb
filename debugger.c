#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#include "logger.h"
#include "breakpoint.h"
#include "debugger.h"
#include "map.h"
#include "utils.h"
#include "reg.h"

#define MAX_LINE_SIZE 64
#define MAX_COMMAND_PARTS 5
#define MAX_PART_SIZE 32
#define WAIT_OPTIONS 0

Debugger *new_debugger(int pid)
{
	Debugger *debugger = (Debugger *)malloc(sizeof(Debugger));
	if (debugger == NULL)
	{
		logger(ERROR, "Failed to allocate heap memory for debugger. %s", strerror(errno));
		return NULL;
	}
	debugger->pid = pid;
	debugger->wait_status = 0;
	Map *break_points = new_map();
	debugger->break_points = break_points;
	return debugger;
}

// Creates a new break point. Returns 1 if the max number of break points has
// already been reached. Returns -1 for errors.
int add_break_point(Debugger *debug, char *cmd_arg)
{
	if (m_is_full(debug->break_points))
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

	int pos = strtoll(cmd_arg, NULL, base);

	BreakPoint *bp = new_bp(debug->pid, bp_type, pos);
	if (bp == NULL)
	{
		logger(ERROR, "failed to create break point %s", cmd_arg);
		return -1;
	}

	// use the line or address of the bp as the key for the map.
	m_set(debug->break_points, cmd_arg, (void *)bp);

	int enable_ret = enable(bp);
	if (enable_ret < 0)
	{
		logger(ERROR, "failed to enable breakpoint: %s", cmd_arg);
		return -1;
	}
	return 0;
}

// Removes the given break point
int remove_break_point(Debugger *debug, char *cmd_arg)
{
	if (m_is_empty(debug->break_points))
	{
		return 1;
	}
	BreakPoint *bp = (BreakPoint *)m_get(debug->break_points, cmd_arg);
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

	m_remove(debug->break_points, cmd_arg);

	free(bp);
	return 0;
}

// Checks the current instruction for a break point and steps over it if one exists
int step_over_breakpoint(Debugger *debug)
{
	// use the instruction pointer to check for break points and if one is found
	// we temporarily disable it.

	int current_instruction_addr = get_ip(debug->pid);
	if (current_instruction_addr == -1)
	{
		logger(ERROR, "failed to get instruction pointer");
		return -1;
	}

	int last_instruction_addr = current_instruction_addr - 1;

	// check for break point at that address
	char bp_key[MAX_KEY_SIZE];
	sprintf(bp_key, "0x%04x", last_instruction_addr);

	BreakPoint *bp = (BreakPoint *)m_get(debug->break_points, bp_key);

	if (bp == NULL)
	{
		return 0;
	}

	if (!bp->enabled)
	{
		return 0;
	}

	int set_ip_res = set_ip(debug->pid, last_instruction_addr);
	if (set_ip_res == -1)
	{
		logger(ERROR, "failed to set instruction pointer");
		return -1;
	}

	int dis_res = disable(bp);
	if (dis_res == -1)
	{
		logger(ERROR, "failed to disable breakpoint %s", bp_key);
		return -1;
	}

	ErrResult step_res = ptrace_with_error(PTRACE_SINGLESTEP, debug->pid, NULL, NULL);
	if (!step_res.success)
	{
		logger(ERROR, "failed stepping to next instruction");
		return -1;
	}

	int pid_res = waitpid(debug->pid, &debug->wait_status, WAIT_OPTIONS);
	if (pid_res == -1)
	{
		logger(ERROR, "failed to wait for process %d. %s", debug->pid, strerror(errno));
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
int continue_execution(Debugger *debug)
{
	int step_err = step_over_breakpoint(debug);
	if (step_err == -1)
	{
		logger(ERROR, "failed to step over breakpoints");
		return -1;
	}

	ErrResult cont_res = ptrace_with_error(PTRACE_CONT, debug->pid, NULL, NULL);
	if (!cont_res.success)
	{
		return -1;
	}

	int pid_result = waitpid(debug->pid, &debug->wait_status, WAIT_OPTIONS);
	if (pid_result == -1)
	{
		logger(ERROR, "failed to wait for process %d. %s", debug->pid, strerror(errno));
		return -1;
	}
	return 0;
}

// Parses and runs the given command. Returns 1 if the command was not recognised
// and -1 for errors.
int run_command(Debugger *debug, char *input)
{
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
	char *first_arg = command_parts[1];

	if (has_prefix(base_command, "c"))
	{
		return continue_execution(debug);
	}

	if (has_prefix(base_command, "b"))
	{
		return add_break_point(debug, first_arg);
	}

	if (has_prefix(base_command, "del"))
	{
		return remove_break_point(debug, first_arg);
	}
	return 1;
}

// starts the main debugging loop.
int run_debugger(Debugger *debug)
{
	// wait until child process is executing
	int pid_result = waitpid(debug->pid, &debug->wait_status, WAIT_OPTIONS);
	if (pid_result < 0)
	{
		logger(ERROR, "failed to wait for process %d. %s", debug->pid, strerror(errno));
		return 0;
	}

	char line_buf[MAX_LINE_SIZE];
	char *current_line = NULL;
	do
	{
		// exit if the child process terminates
		if (debug->wait_status == 0)
		{
			logger(INFO, "Debugged process %d has terminated", debug->pid);
			return 0;
		}

		fputs("edb> ", stdout);
		// fgets will stop hanging when either a \n or a EOF is found
		current_line = fgets(line_buf, MAX_LINE_SIZE, stdin);

		// Run the command but dont exit on failure
		int cmd_result = run_command(debug, current_line);
		switch (cmd_result)
		{
		case 1:
			logger(WARN, "Command not recognised: %s", current_line);
			break;
		case -1:
			logger(ERROR, "failed to run command: %s", current_line);
			break;
		default:
			break;
		}
	} while (current_line != NULL);

	logger(ERROR, "Failed to read line", strerror(errno));
	return -1;
}