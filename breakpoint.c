#include <stdlib.h>
#include <errno.h>
#include <sys/ptrace.h>

#include "breakpoint.h"
#include "logger.h"

// the x86 opcode for int3
#define int_3 0xCC

BreakPoint * new_bp(int pid, BreakPointType type, int break_pos) {
    BreakPoint * bp = (BreakPoint *)malloc(sizeof(BreakPoint));
    if (bp == NULL)
	{
		logger(ERROR, "Failed to allocate heap memory for breakpoint. ERRNO: %d", errno);
		return NULL;
	}
    bp->enabled = 0;
    bp->type = type;
    bp->pos = break_pos;
    bp->pid = pid;
    return bp;
}

// allows the program to stop when reaching the given instruction
int enable(BreakPoint * bp) {
    switch (bp->type) {
        case ADDR:
        // get the instruction at the specified address
        int instruction = ptrace(PTRACE_PEEKDATA, bp->pid, bp->pos, NULL);
        if (instruction < 0) {
            logger(ERROR, "failed to save instruction at breakpoint %d. ERRNO: %d", bp->pos, errno);
            return -1;
        };

        // save the first byte of the instruction so we can restore it later
        bp->saved_data = (char)(instruction & 0xff);
        // insert the interrupt into the instruction
        int int3_instruction = ((instruction & ~0xff) | int_3);

        int res = ptrace(PTRACE_POKEDATA, bp->pid, bp->pos, int3_instruction);
        if (res < 0) {
            logger(ERROR, "failed to write interrupt at breakpoint %d. ERRNO: %d", bp->pos, errno);
            return -1;
        };

        break;
    }
    return 0;
}

