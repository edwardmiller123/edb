#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

#include "logger.h"
#include "debugger.h"

#define MAX_LINE_SIZE 64

Debugger * new_debugger(int pid) {
	Debugger * debugger = (Debugger *)malloc(sizeof(Debugger));
	if (debugger == NULL) {
		logger(ERROR, "Failed to allocate heap memory. ERRNO: %d", errno);
		return NULL;
	}
	debugger->pid = pid;
	debugger->wait_status = 0;
	return debugger;
}

int continue_execution() {

}

int parse_command(char *command)
{
	printf("%s\n", command);
}

int run_debugger(Debugger * debug)
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

		int result = parse_command(current_line);
		if (result < 0)
		{
			logger(ERROR, "failed to parse command: %s", current_line);
			return -1;
		}
	} while (current_line != NULL);

	return -1;
}