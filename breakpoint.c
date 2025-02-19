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

BreakPoint *new_bp(int pid, BreakPointType type, unsigned int break_pos)
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
            logger(ERROR, "Failed to get instruction at breakpoint %d", bp->pos);
            return -1;
        };

        unsigned long instruction = (unsigned long)peek_res.val;

        // save the first byte of the instruction so we can restore it later
        bp->saved_data = (char)(instruction & (unsigned long)0xff);

        // insert the interrupt into the instruction
        unsigned long int3_instruction = ((instruction & ~0xff) | (unsigned long)int_3);
        logger(DEBUG, "Inserting breakpoint at %p. Editing instruction %p -> %p", (void *)bp->pos, (void *)instruction, (void *)int3_instruction);

        ErrResult poke_res = ptrace_with_error(PTRACE_POKEDATA, bp->pid, (void *)bp->pos, (void *)int3_instruction);
        if (!poke_res.success)
        {
            logger(ERROR, "Failed to insert interrupt at breakpoint %d", bp->pos);
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
        logger(ERROR, "Failed to get instruction at breakpoint %d", bp->pos);
        return -1;
    };

    unsigned long current_instruction = (unsigned long)peek_res.val;

    // zero the last byte containing our injected int3 
    unsigned long current_intstruction_zeroed = (unsigned long)(current_instruction & ~0xff);

    // restore the saved byte of the instruction making sure to first zero all the unessecary bits
    unsigned long restored_instruction = current_intstruction_zeroed | ((unsigned long)bp->saved_data & 0xff);

    logger(DEBUG, "Restoring instruction at breakpoint: %p", (void *) restored_instruction);

    // write back the restored instruction
    ErrResult poke_res = ptrace_with_error(PTRACE_POKEDATA, bp->pid, (void *)bp->pos, (void *)restored_instruction);
    if (!poke_res.success)
    {
        logger(ERROR, "Failed to restore instruction at address %d", bp->pos);
        return -1;
    };

    bp->enabled = false;
    bp->saved_data = 0;

    return 0;
}
