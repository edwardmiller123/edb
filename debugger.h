#include <stdbool.h>

#include "map.h"
#include "session.h"

typedef struct Debugger {
	DebugSession * session;
	Map * break_points;
} Debugger;

Debugger * new_debugger();

int run_cmd_loop(Debugger *db, const char * prog);