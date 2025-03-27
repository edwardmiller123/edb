#ifndef SESSION_H
#define SESSION_H

#include <stdbool.h>
#include <stdint.h>

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

// General info about the ELF file
typedef struct ElfInfo {
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;

	// location of the binary in memory
	uint64_t e_entry;

	// program header table offset
	uint64_t e_phoff;

	// section header table offset
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;

	// size of a section header table entry
	uint16_t e_shentsize;

	// number of section header table entries
	uint16_t e_shnum;

	// index of the section name string table in the header table
	uint16_t e_shstrndx;
} ElfInfo;

// Info about a specific elf section
typedef struct ElfSectionHeader {
	// name of the section as an offset into the section name string table
    uint32_t sh_name;

    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;

	// the sections offset in memory from the begining of the elf
    uint64_t sh_offset;

	// size of the section
    uint64_t sh_size;

    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} ElfSectionHeader;

typedef struct DwarfDebugLineHeader {
    uint64_t length;
    uint16_t version;
    uint64_t header_length;
    uint8_t min_instruction_length;
    uint8_t default_is_stmt;
    int8_t line_base;
    uint8_t line_range;
    uint8_t opcode_base;
    uint8_t std_opcode_lengths[12];
} __attribute__((packed)) DwarfDebugLineHeader;

typedef struct {
	uint64_t addr;
	char * src_file;
	uint64_t line_number;
} DwarfParserStateMachine;

typedef struct {
	uint32_t line_number;
	char * file;
} LineNumberInfo;

DebugSession *new_debug_session(char *prog, int pid);

void remove_debug_session(DebugSession *session);

int start_tracing(char *prog);

int parse_dwarf_info(DebugSession * session);

#endif