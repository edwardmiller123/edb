#ifndef REG_H
#define REG_H

// Gets the value of a given register. Returns -1 if the requested register is
// not found.
int get_reg_value(int pid, char *reg_name);

#endif