#define MAX_BREAKPOINTS 64

typedef enum BreakPointType {
	LINE,
	ADDR,
} BreakPointType;

// Information about the break point. Only the fields that correspond
// the the appropriate type will be filled.
typedef struct BreakPoint {
	BreakPointType type;
	int addr;
	int line;
	// Is the breakpoint set?
	int enabled;
	// The instruction that has been temporarily replaced with the interrupt
	int saved_data;

} BreakPoint;

typedef struct Debugger {
	int pid;
	int wait_status;
} Debugger;

Debugger * new_debugger(int pid);

int run_debugger(Debugger * debug);