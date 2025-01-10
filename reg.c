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

// Retrieves the value of the given register. Returns a pointer to the register's
// position in the given struct;
unsigned long long *get_register(int pid, struct user_regs_struct *regs, Reg reg)
{
	ErrResult regs_res = ptrace_with_error(PTRACE_GETREGS, pid, NULL, (void *)regs);
	if (!regs_res.success)
	{
		logger(ERROR, "failed to get register values");
		return NULL;
	}
	switch (reg)
	{
	case R15:
		return &regs->r15;
		break;
	case R14:
		return &regs->r14;
		break;
	case R13:
		return &regs->r13;
		break;
	case R12:
		return &regs->r12;
		break;
	case R11:
		return &regs->r11;
		break;
	case R10:
		return &regs->r10;
		break;
	case R9:
		return &regs->r9;
		break;
	case R8:
		return &regs->r8;
		break;
	case RBP:
		return &regs->rbp;
		break;
	case RBX:
		return &regs->rbx;
		break;
	case RAX:
		return &regs->rax;
		break;
	case RCX:
		return &regs->rcx;
		break;
	case RDX:
		return &regs->rdx;
		break;
	case RSI:
		return &regs->rsi;
		break;
	case RDI:
		return &regs->rdi;
		break;
	case ORIG_RAX:
		return &regs->orig_rax;
		break;
	case RIP:
		return &regs->rip;
		break;
	case CS:
		return &regs->cs;
		break;
	case EFLAGS:
		return &regs->eflags;
		break;
	case RSP:
		return &regs->r13;
		break;
	case SS:
		return &regs->ss;
		break;
	case FS_BASE:
		return &regs->fs_base;
		break;
	case GS_BASE:
		return &regs->gs_base;
		break;
	case DS:
		return &regs->ds;
		break;
	case FS:
		return &regs->fs;
		break;
	case ES:
		return &regs->es;
		break;
	case GS:
		return &regs->gs;
		break;
	default:
		return NULL;
	}
}

// Gets the value of a given register by its name
ErrResult get_reg_value_by_name(int pid, char *reg_name)
{
	ErrResult res = {.success = false};

	if (reg_name == NULL)
	{
		logger(ERROR, "register name must not be NULL");
		return res;
	}

	struct user_regs_struct regs;

	for (int i = 0; i < REGISTER_COUNT; i++)
	{
		if (strcmp(registers[i].name, reg_name) == 0)
		{
			int *reg = (int *)get_register(pid, &regs, registers[i].id);
			if (reg == NULL)
			{
				logger(ERROR, "Failed to get register");
			}
			else
			{
				res.success = true;
				res.val = *reg;
			}
			return res;
		}
	}
	logger(WARN, "Unknown register");
	return res;
}

// set the value of the given register
int set_reg_value(int pid, Reg reg, int val)
{
	struct user_regs_struct regs;

	int *reg_addr = (int *)get_register(pid, &regs, reg);
	if (reg_addr == NULL)
	{
		logger(ERROR, "Failed to get register");
		return -1;
	}

	// set the value
	*reg_addr = val;

	// write the result back
	ErrResult set_regs_res = ptrace_with_error(PTRACE_SETREGS, pid, NULL, (void *)&regs);
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
	logger(WARN, "Unknown register name");
	return -1;
}

// gets the value of the instruction pointer
int get_ip(int pid)
{
	struct user_regs_struct regs;
	int *reg = (int*)get_register(pid, &regs, RIP);
	if (reg == NULL)
	{
		logger(ERROR, "Faled to get instruction pointer");
		// value from RIP shouldnt ever be negative so can safely return -1 for errors
		return -1;
	}
	return *reg;
}

// sets the value of the instruction pointer
int set_ip(int pid, int val)
{
	return set_reg_value(pid, RIP, val);
}
