#include <stdlib.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <string.h>
#include <stdio.h>

#include "breakpoint.h"
#include "logger.h"
#include "utils.h"

// the x86 opcode for int3
#define int_3 0xCC

BreakPoint *new_bp(int pid, BreakPointType type, int break_pos)
{
    BreakPoint *bp = (BreakPoint *)malloc(sizeof(BreakPoint));
    if (bp == NULL)
    {
        logger(ERROR, "Failed to allocate heap memory for breakpoint. ERRNO: %d", errno);
        return NULL;
    }
    bp->enabled = false;
    bp->type = type;
    bp->pos = break_pos;
    bp->pid = pid;
    return bp;
}

// allows the program to stop when reaching the given instruction
int enable(BreakPoint *bp)
{
    switch (bp->type)
    {
    case ADDR:
        // get the instruction at the specified address
        ErrResult peek_res = ptrace_with_error(PTRACE_PEEKDATA, bp->pid, (void *)bp->pos, NULL);
        if (!peek_res.success)
        {
            logger(ERROR, "failed to get instruction at breakpoint %d", bp->pos);
            return -1;
        };

        unsigned int instruction = (unsigned int)peek_res.val;

        // save the first byte of the instruction so we can restore it later
        bp->saved_data = (char)(instruction & (unsigned int)0xff);

        // insert the interrupt into the instruction
        unsigned int int3_instruction = ((instruction & ~0xff) | (unsigned int)int_3);

        ErrResult poke_res = ptrace_with_error(PTRACE_POKEDATA, bp->pid, (void *)bp->pos, (void *)int3_instruction);
        if (!poke_res.success)
        {
            logger(ERROR, "failed to write interrupt at breakpoint %d", bp->pos);
            return -1;
        };

        break;
    }

    bp->enabled = true;

    return 0;
}

// Disables the given breakpoint. Returns 1 if the breakpoint is already disabled.
// Returns -1 for errors.
int disable(BreakPoint *bp)
{
    if (!bp->enabled)
    {
        return 1;
    }

    // get the edited instruction
    ErrResult peek_res = ptrace_with_error(PTRACE_PEEKDATA, bp->pid, (void *)bp->pos, NULL);
    if (!peek_res.success)
    {
        logger(ERROR, "failed to get instruction at breakpoint %d", bp->pos);
        return -1;
    };

    unsigned int current_instruction = (unsigned int)peek_res.val;

    // zero the last byte containing our injected int3 
    unsigned int current_intstruction_zeroed = (unsigned int)(current_instruction & ~0xff);

    // restore the saved byte of the instruction making sure to first zero al the unessecary bits
    unsigned int restored_instruction = current_intstruction_zeroed | ((unsigned int)bp->saved_data & 0xff);

    // write back the restored instruction
    ErrResult poke_res = ptrace_with_error(PTRACE_POKEDATA, bp->pid, (void *)bp->pos, (void *)restored_instruction);
    if (!poke_res.success)
    {
        logger(ERROR, "failed to restore instruction at address %d", bp->pos);
        return -1;
    };

    bp->enabled = false;
    bp->saved_data = 0;

    return 0;
}
