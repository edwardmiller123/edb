#include <stdio.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/personality.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "session.h"
#include "logger.h"
#include "utils.h"

#define MAX_PROG_NAME_SIZE 32
#define MAX_HEADER_NAME_SIZE 32
#define FILE_TABLE_SIZE 128
#define DEBUG_LINE_HEADER ".debug_line"

DebugSession *new_debug_session(char *prog, int pid)
{
	DebugSession *dbs = (DebugSession *)malloc(sizeof(DebugSession));
	if (dbs == NULL)
	{
		logger(ERROR, "Failed to allocate heap memory for debug session. %s", strerror(errno));
		return NULL;
	}

	char *prog_name_buf = (char *)malloc(MAX_PROG_NAME_SIZE);
	if (prog_name_buf == NULL)
	{
		logger(ERROR, "Failed to allocate heap memory for program name. %s", strerror(errno));
		return NULL;
	}

	memcpy(prog_name_buf, prog, MAX_PROG_NAME_SIZE);
	dbs->prog = prog_name_buf;

	dbs->pid = pid;
	dbs->wait_status = 0;
	return dbs;
}

void remove_debug_session(DebugSession *session)
{
	free(session->prog);
	free(session);
}

int start_tracing(char *prog)
{
	// we are the child process we should allow the parent to trace us
	// and start executing the program we wish to debug.

	// We return the EXIT code on any errors so that the child process terminates and
	// isnt left hanging around.

	int last_persona = personality(ADDR_NO_RANDOMIZE);
	if (last_persona < 0)
	{
		logger(ERROR, "Failed to set child personaility. ERRNO: %d\n", errno);
		return EXIT;
	}

	ErrResult trace_res = ptrace_with_error(PTRACE_TRACEME, 0, NULL, NULL);
	if (!trace_res.success)
	{
		logger(ERROR, "Failed to start tracing child.");
		return EXIT;
	}

	if (execl(prog, prog, NULL) < 0)
	{
		logger(ERROR, "Failed to execute %s. %s", prog, strerror(errno));
		return EXIT;
	}
	return EXIT;
}

// TODO:
//	- Load first part of elf into memory
//	- Look up section header table and load (some sections) into memory
// 	- find .debug_line section by cross referencing with header name string table
// 	- load .debug_line section into memory and parse to create map of line numbers to memory addresses

int validate_elf(FILE *elf_file)
{
	char elf_identifier[] = {0x7f, 'E', 'L', 'F'};
	// check the elf identifier header
	char ident_buf[16];
	size_t ident_bytes_read = fread(ident_buf, 1, 16, elf_file);
	if (ident_bytes_read == 0)
	{
		logger(ERROR, "Failed to read elf itentifier. %s", strerror(errno));
		return -1;
	}

	for (int i = 0; i < 4; i++)
	{
		if (ident_buf[i] != elf_identifier[i])
		{
			logger(ERROR, "Program is not ELF format.");
			return -1;
		}
	}
	return 0;
}

// Finds the the elf section header matching the given input string
int locate_elf_section(char *section_name, char *name_table, uint32_t name_table_size, ElfSectionHeader header_table[], uint16_t header_count, ElfSectionHeader *header)
{
	char *current_header_name = NULL;
	for (int i = 0; i <= header_count; i++)
	{
		current_header_name = (char *)(name_table + header_table[i].sh_name);
		if (strcmp(section_name, current_header_name) == 0)
		{
			*header = header_table[i];
			logger(DEBUG, "%s section found at offset %p.", section_name, (void *)header_table[i].sh_offset);
			return 0;
		}
	}

	return -1;
}

// Extracts a map of memory addresses to line numbers from the given buffer containing a .debug_section
Map *get_line_numbers(uint8_t *debug_line_section, uint64_t section_size)
{
	Map *line_numbers = new_map();
	if (line_numbers == NULL)
	{
		logger(ERROR, "Failed to initialise map");
		return NULL;
	}
	// TODO: parse dwarf using statemachine

	DwarfParserStateMachine state_machine = {
		.addr = 0,
		.line_number = 0,
		.src_file = NULL,
	};

	DwarfDebugLineHeader current_cu_header;
	int file_table_buf_idx = 0;
	// buffer containing both the directory and file tables
	uint8_t file_table_buffer[FILE_TABLE_SIZE];
	int compilation_unit = 0;
	bool parsing_file_table = false;
	uint64_t section_current_byte = 0;

	// keeps track of the current byte we are on in the CU
	uint64_t cu_current_byte = 0;
	while(section_current_byte < section_size)
	{

		if (cu_current_byte < sizeof(DwarfDebugLineHeader))
		{
			// read the header for the current CU
			logger(DEBUG, "Reading header of CU %d.", compilation_unit);
			memcpy(&current_cu_header, debug_line_section, sizeof(DwarfDebugLineHeader));

			cu_current_byte += sizeof(DwarfDebugLineHeader);
			section_current_byte += sizeof(DwarfDebugLineHeader);

			logger(DEBUG, "Found %d lines in CU %d.", current_cu_header.line_range, compilation_unit);
			parsing_file_table = true;
		}

		if (parsing_file_table)
		{
			// the opcodes offset from the beginning of the CU. Plus 18 as the header length field doesnt
			// include the field itself or the previous two fields
			uint64_t opcodes_offset = current_cu_header.header_length + 18;
			logger(DEBUG, "Opcodes for CU %d found at offset %d.", compilation_unit, opcodes_offset);
			// read the file and directory table
			while(cu_current_byte < opcodes_offset && section_current_byte < section_size) {
				file_table_buffer[file_table_buf_idx] = debug_line_section[section_current_byte];
				
				file_table_buf_idx++;
				section_current_byte++;
				cu_current_byte++;
				printf("%c\n", file_table_buffer[file_table_buf_idx]);
			}
			parsing_file_table = true;
		}
	}
	printf("%s\n", file_table_buffer);
	return line_numbers;
}

// Parses the dwarf info from the program path in the given session
int parse_dwarf_info(DebugSession *session)
{
	FILE *elf_file = fopen(session->prog, "rb");
	if (elf_file == NULL)
	{
		logger(ERROR, "Failed to open file. %s", strerror(errno));
		return -1;
	}

	if (validate_elf(elf_file) == -1)
	{
		logger(ERROR, "Failed to validate elf file. %s", strerror(errno));
		return -1;
	}

	logger(DEBUG, "Parsing DWARF info from %s.", session->prog);

	// read the general file info
	ElfInfo info;
	if (fread(&info, 1, sizeof(ElfInfo), elf_file) == 0)
	{
		logger(ERROR, "Failed to read elf file info. %s", strerror(errno));
		return -1;
	}

	uint16_t header_count = info.e_shnum;
	logger(DEBUG, "Section header table found at offset %p with %d entries.", (void *)info.e_shoff, header_count);

	// get the section header table
	ElfSectionHeader header_table[header_count];
	if (fseek(elf_file, info.e_shoff, SEEK_SET) < 0)
	{
		logger(ERROR, "Failed to look ahead for header table. %s", strerror(errno));
		return -1;
	}

	if (fread(header_table, sizeof(char), header_count * sizeof(ElfSectionHeader), elf_file) == 0)
	{
		logger(ERROR, "Failed to read header table. %s", strerror(errno));
		return -1;
	}

	ElfSectionHeader names_header = header_table[info.e_shstrndx];
	uint64_t names_table_pos = names_header.sh_offset;
	uint64_t name_table_size = names_header.sh_size;
	logger(DEBUG, "Header names found at offset %p.", (void *)names_table_pos);

	if (fseek(elf_file, names_table_pos, SEEK_SET) < 0)
	{
		logger(ERROR, "Failed to look ahead for section name table. %s", strerror(errno));
		return -1;
	}

	// get header name table
	char name_table[name_table_size];
	if (fread(name_table, sizeof(char), name_table_size, elf_file) != name_table_size)
	{
		logger(ERROR, "Failed to read name table. %s", strerror(errno));
		return -1;
	}

	// find .debug_line section
	ElfSectionHeader debug_line_section_table_header;
	if (locate_elf_section(DEBUG_LINE_HEADER, name_table, name_table_size, header_table, header_count, &debug_line_section_table_header) == -1)
	{
		logger(ERROR, "Failed to locate %s section.", DEBUG_LINE_HEADER);
		return -1;
	}

	if (fseek(elf_file, debug_line_section_table_header.sh_offset, SEEK_SET) == -1)
	{
		logger(ERROR, "Failed to look ahead for .debug_line section. %s", strerror(errno));
		return -1;
	}

	// the debug line section is comprised of a CUs each with a header struct,
	// then a directory table then a file name table and finally a list of opcodes encoding a line number and memory address.
	// We read the whole section into memory then break it up into the CUs.
	uint8_t debug_line_section[debug_line_section_table_header.sh_size];
	if (fread(debug_line_section, sizeof(char), debug_line_section_table_header.sh_size, elf_file) != debug_line_section_table_header.sh_size)
	{
		logger(ERROR, "Failed to read .debug_line section header. %s", strerror(errno));
		return -1;
	}

	logger(DEBUG, "Parsing .debug_line section.");

	Map *line_numbers = get_line_numbers(debug_line_section, debug_line_section_table_header.sh_size);
	if (line_numbers == NULL)
	{
		logger(ERROR, "Failed to extract line numbers");
		return -1;
	}

	if (fclose(elf_file) != 0)
	{
		logger(ERROR, "Failed to close file. %s", strerror(errno));
		return -1;
	}
	return 0;
}