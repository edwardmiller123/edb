#include <stdlib.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <string.h>

#include "reg.h"
#include "logger.h"
#include "utils.h"

#define REGISTER_COUNT 27

// maps a register to its DWARF number
typedef struct reg_info
{
	Reg id;
	char *name;
	int dwarf_num;
} reg_info;

const reg_info registers[REGISTER_COUNT] = {
	{R15, "r15", 15},
	{R14, "r14", 14},
	{R13, "r13", 13},
	{R12, "r12", 12},
	{RBP, "rbp", 6},
	{RBX, "rbx", 3},
	{R11, "r11", 11},
	{R10, "r10", 10},
	{R9, "r9", 9},
	{R8, "r8", 8},
	{RAX, "rax", 0},
	{RCX, "rcx", 2},
	{RDX, "rdx", 1},
	{RSI, "rsi", 4},
	{RDI, "rdi", 5},
	{ORIG_RAX, "orig_rax", -1},
	{RIP, "rip", -1},
	{CS, "cs", 51},
	{EFLAGS, "eflags", 49},
	{RSP, "rsp", 7},
	{SS, "ss", 52},
	{FS_BASE, "fs_base", 58},
	{GS_BASE, "gs_base", 59},
	{DS, "ds", 53},
	{ES, "es", 50},
	{FS, "fs", 54},
	{GS, "gs", 55}};

// Returns the value of the given register
ErrResult get_reg_value(int pid, Reg reg)
{
	ErrResult res;
	// first get all the registers
	struct user_regs_struct regs;
	ErrResult regs_res = ptrace_with_error(PTRACE_GETREGS, pid, NULL, (int)&regs);
	if (!regs_res.success)
	{
		logger(ERROR, "failed to get register values");
		res.success = false;
		return res;
	}

	// "index" into the struct to return the value of the requested register.
	int reg_address = (int)&regs;
	res.val = *(int *)(reg_address + ((int)reg * sizeof(int)));
	res.success = true;
	return res;
}

// Gets the value of a given register.
ErrResult get_reg_value_by_name(int pid, char *reg_name)
{
	ErrResult res;
	res.success = false;
	if (reg_name == NULL)
	{
		logger(ERROR, "register name must not be NULL");
		return res;
	}

	for (int i = 0; i < REGISTER_COUNT; i++)
	{
		if (strcmp(registers[i].name, reg_name) == 0)
		{
			return get_reg_value(pid, registers[i].id);
		}
	}
	return res;
}

// set the value of the given register
int set_reg_value(int pid, Reg reg, int val)
{
	// get all the registers
	struct user_regs_struct regs;
	ErrResult get_regs_res = ptrace_with_error(PTRACE_GETREGS, pid, NULL, (int)&regs);
	if (!get_regs_res.success)
	{
		logger(ERROR, "Failed to get register values");
		return -1;
	}

	// "index" into the struct to return the value of the requested register.
	int reg_address = (int)&regs;

	// set the value
	*(int *)(reg_address + ((int)reg * sizeof(int))) = val;

	// write the result back
	ErrResult set_regs_res = ptrace_with_error(PTRACE_SETREGS, pid, NULL, (int)&regs);
	if (!set_regs_res.success)
	{
		logger(ERROR, "Failed to set register values");
		return -1;
	}
	return 0;
}

// sets the value of the given register by its name
int set_reg_value_by_name(int pid, char *reg_name, int val)
{
	if (reg_name == NULL)
	{
		logger(ERROR, "register name must not be NULL");
		return -1;
	}

	for (int i = 0; i < REGISTER_COUNT; i++)
	{
		if (strcmp(registers[i].name, reg_name) == 0)
		{
			return set_reg_value(pid, registers[i].id, val);
		}
	}
	logger(ERROR, "failed to identify register to set");
	return -1;
}

// gets the value of the instruction pointer
int get_ip(int pid)
{
	ErrResult res = get_reg_value(pid, RIP);
	if (!res.success)
	{
		// value from RIP shouldnt ever ber negative so can safely return an int
		return -1;
	}
	return res.val;
}

// sets the value of the instruction pointer
int set_ip(int pid, int val)
{
	return set_reg_value(pid, RIP, val);
}
