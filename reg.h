#ifndef REG_H
#define REG_H

#include <sys/user.h>

#include "utils.h"

// x86 Register identifiers.
typedef enum Reg {
	R15,
	R14,
	R13,
	R12,
	RBP,
	RBX,
	R11,
	R10,
	R9,
	R8,
	RAX,
	RCX,
	RDX,
	RSI,
	RDI,
	ORIG_RAX,
	RIP,
	CS,
	EFLAGS,
	RSP,
	SS,
	FS_BASE,
	GS_BASE,
	DS,
	ES,
	FS,
	GS,
} Reg;

// Retrieves the value of the given register. Returns a pointer to the register's
// position in the given struct;
unsigned long long * get_register(int pid, struct user_regs_struct * regs, Reg reg);

// Gets the value of a given register by its name. Returns -1 if the requested register is
// not found.
ErrResult get_reg_value_by_name(int pid, char *reg_name);

// set the value of the given register
int set_reg_value(int pid, Reg reg, unsigned long long val);

// sets the value of the given register by its name
int set_reg_value_by_name(int pid, char *reg_name, int val);

// Gets the value of the instruction pointer. Returns NULL for errors
void * get_ip(int pid);

// sets the value of the instruction pointer (this is always a memory address so takes a void pointer)
int set_ip(int pid, void * val);

#endif