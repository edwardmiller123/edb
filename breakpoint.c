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
        PTraceResult peek_res = ptrace_with_error(PTRACE_PEEKDATA, bp->pid, bp->pos, NULL);
        if (!peek_res.success)
        {
            logger(ERROR, "failed to get instruction at breakpoint %d");
            return -1;
        };

        int instruction = peek_res.val;

        // save the first byte of the instruction so we can restore it later
        bp->saved_data = (char)(instruction & 0xff);
        // insert the interrupt into the instruction
        int int3_instruction = ((instruction & ~0xff) | int_3);

        PTraceResult poke_res = ptrace_with_error(PTRACE_POKEDATA, bp->pid, bp->pos, int3_instruction);
        if (!poke_res.success)
        {
            logger(ERROR, "failed to write interrupt at breakpoint %d", bp->pos);
            return -1;
        };

        break;
    }
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
    PTraceResult peek_res = ptrace_with_error(PTRACE_PEEKDATA, bp->pid, bp->pos, NULL);
    if (!peek_res.success)
    {
        logger(ERROR, "failed to get instruction at breakpoint %d", bp->pos);
        return -1;
    };

    int current_instruction = peek_res.val;

    int restored_instruction = ((current_instruction & ~0xff) | bp->saved_data);

    // write back the restored instruction
    PTraceResult poke_res = ptrace_with_error(PTRACE_POKEDATA, bp->pid, bp->pos, restored_instruction);
    if (poke_res.success)
    {
        logger(ERROR, "failed to restore instruction at breakpoint %d", bp->pos);
        return -1;
    };

    bp->enabled = false;
    bp->saved_data = 0;

    return 0;
}
