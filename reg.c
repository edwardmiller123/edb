#include <stdlib.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <string.h>

#include "logger.h"
#include "utils.h"

#define REGISTER_COUNT 27

// maps a register to its DWARF number
typedef struct reg
{
	char *name;
	int dwarf_num;
} reg;

const reg registers[REGISTER_COUNT] = {
	{"r15", 15},
	{"r14", 14},
	{"r13", 13},
	{"r12", 12},
	{"rbp", 6},
	{"rbx", 3},
	{"r11", 11},
	{"r10", 10},
	{"r9", 9},
	{"r8", 8},
	{"rax", 0},
	{"rcx", 2},
	{"rdx", 1},
	{"rsi", 4},
	{"rdi", 5},
	{"orig_rax", -1},
	{"rip", -1},
	{"cs", 51},
	{"eflags", 49},
	{"rsp", 7},
	{"ss", 52},
	{"fs_base", 58},
	{"gs_base", 59},
	{"ds", 53},
	{"es", 50},
	{"fs", 54},
	{"gs", 55}};

// Gets the value of a given register. Returns -1 if the requested register is
// not found.
int get_reg_value(int pid, char *reg_name)
{
	// first get all the registers
	struct user_regs_struct regs;
	PTraceResult regs_res = ptrace_with_error(PTRACE_GETREGS, pid, NULL, (int)&regs);
	if (!regs_res.success)
	{
		logger(ERROR, "Failed to get register values");
		return -1;
	}

	// "index" into the struct to return the value of the requested register.
	int reg_address = (int)&regs;

	for (int i = 0; i < REGISTER_COUNT; i++)
	{
		if (strcmp(registers[i].name, reg_name) == 0)
		{
			return *(int *)(reg_address + (i * sizeof(int)));
		}
	}
	return -1;
}