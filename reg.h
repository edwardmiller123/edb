#ifndef REG_H
#define REG_H

#include "utils.h"

// x86 Register identifiers. They are in the same order as the user_reg_struct so that it can
// be treated as an array and we can use the numerical value of the enum as the index.
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

// Returns the value of the given register
ErrResult get_reg_value(int pid, Reg reg);

// Gets the value of a given register by its name. Returns -1 if the requested register is
// not found.
ErrResult get_reg_value_by_name(int pid, char *reg_name);

// set the value of the given register
int set_reg_value(int pid, Reg reg, int val);

// sets the value of the given register by its name
int set_reg_value_by_name(int pid, char *reg_name, int val);

// gets the value of the instruction pointer
int get_ip(int pid);

// sets the value of the instruction pointer
int set_ip(int pid, int val);

#endif