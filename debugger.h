#include <stdbool.h>

#include "map.h"

typedef struct DebugSession {
	char * prog;
	int pid;
	int wait_status;
	bool active;
} DebugSession;

typedef struct Debugger {
	DebugSession * session;
	Map * break_points;
} Debugger;

Debugger * new_debugger();

int run_cmd_loop(Debugger *db, const char * prog);