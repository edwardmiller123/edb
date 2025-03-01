#ifndef SESSION_H
#define SESSION_H

#include <stdbool.h>

#include "map.h"

// The magic number to exit the program
#define EXIT -73

typedef struct DebugSession {
	char * prog;
	int pid;
	int wait_status;
	bool active;
	Map * line_numbers;
} DebugSession;

#include <stdint.h>

// General info about the ELF file
typedef struct elf_info {
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	void * e_entry;

	// program header table offset
	void * e_phoff;

	void * e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;

	// size of a section header table entry
	uint16_t e_shentsize;

	// number of section header table entries
	uint16_t e_shnum;

	// index of the section name string table in the section table
	uint16_t e_shstrndx;
} elf_info;

// Info about a specific elf section
typedef struct elf_section_header {
	// name of the section as an index into the section name string table
    uint32_t sh_name;

    uint32_t sh_type;
    void * sh_flags;
    void * sh_addr;

	// the sections offset in memory from the beggining of the elf
    void * sh_offset;

	// size of the section
    uint64_t sh_size;

    uint32_t sh_link;
    uint32_t sh_info;
    void * sh_addralign;
    void * sh_entsize;
} elf_section_header;

DebugSession *new_debug_session(char *prog, int pid);

void remove_debug_session(DebugSession *session);

int start_tracing(char *prog);

#endif