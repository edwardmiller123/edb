#ifndef BP_H
#define BP_H

#include <stdbool.h>

#define MAX_BREAKPOINTS 64

typedef enum BreakPointType {
	LINE,
	ADDR,
} BreakPointType;

// Information about the break point. Only the fields that correspond
// the the appropriate type will be filled.
typedef struct BreakPoint {
	// process id we are inserting the breakpoint for
	int pid;
	BreakPointType type;
	// The position in the program we should break at. Can either be
	// a line number or a memory address depending on the type.
	int pos;
	// Is the breakpoint set?
	bool enabled;
	// The last byte of the instruction that has been temporarily replaced 
    // with the an interrupt
	char saved_data;

} BreakPoint;

BreakPoint * new_bp(int pid, BreakPointType type, int break_pos);

// allows the program to stop when reaching the given instruction
int enable(BreakPoint * bp);

// disables the given breakpoint
int disable(BreakPoint *bp);

#endif