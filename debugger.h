typedef struct Debugger {
	int pid;
	int wait_status;
} Debugger;

Debugger * new_debugger(int pid);

int run_debugger(Debugger * debug);