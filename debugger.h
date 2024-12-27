#include "map.h"

typedef struct Debugger {
	int pid;
	int wait_status;
	Map * break_points;
} Debugger;

Debugger * new_debugger(int pid);

int run_debugger(Debugger * debug);