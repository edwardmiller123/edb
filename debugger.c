#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#include "logger.h"
#include "debugger.h"

#define MAX_LINE_SIZE 64
#define MAX_COMMAND_PARTS 5
#define MAX_PART_SIZE 32

Debugger *new_debugger(int pid)
{
	Debugger *debugger = (Debugger *)malloc(sizeof(Debugger));
	if (debugger == NULL)
	{
		logger(ERROR, "Failed to allocate heap memory. ERRNO: %d", errno);
		return NULL;
	}
	debugger->pid = pid;
	debugger->wait_status = 0;
	return debugger;
}

// Restarts a paused process
int continue_execution(Debugger *debug)
{
	long ptrace_err = ptrace(PTRACE_CONT, debug->pid, NULL, NULL);
	if (ptrace_err < 0)
	{
		logger(ERROR, "Ptrace failed. ERRNO: %d\n", errno);
		return -1;
	}
	waitpid(debug->pid, &debug->wait_status, 0);
}

int parse_command(Debugger *debug, char *input)
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

	if (strcmp(base_command, "continue") == 0)
	{
		return continue_execution(debug);
	}
}

// starts the main debugging loop.
int run_debugger(Debugger *debug)
{
	// first pause the child process
	int options = 0;
	waitpid(debug->pid, &debug->wait_status, options);

	char line_buf[MAX_LINE_SIZE];
	char *current_line = NULL;
	do
	{
		fputs("edb> ", stdout);
		current_line = fgets(line_buf, MAX_LINE_SIZE, stdin);

		// fgets will stop hanging when either a \n or a EOF is found

		int result = parse_command(debug, current_line);
		if (result < 0)
		{
			logger(ERROR, "failed to parse command: %s", current_line);
			return -1;
		}
	} while (current_line != NULL);

	return -1;
}