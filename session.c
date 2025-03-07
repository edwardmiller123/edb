#include <stdio.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/personality.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "session.h"
#include "logger.h"
#include "utils.h"

#define MAX_PROG_NAME_SIZE 32
#define MAX_HEADER_NAME_SIZE 32
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

uint64_t locate_elf_section(char *section_name, char *name_table, uint32_t name_table_size, elf_section_header header_table[], uint16_t header_count)
{
	uint64_t header_pos = 0;
	char *current_header_name = NULL;
	for (int i = 0; i <= header_count; i++)
	{
		current_header_name = (char *)(name_table + header_table[i].sh_name);
		if (strcmp(section_name, current_header_name) == 0)
		{
			header_pos = header_table[i].sh_offset;
			logger(DEBUG, "%s section found at offset %p.", section_name, (void *)header_pos);
			break;
		}
	}

	return header_pos;
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
	elf_info info;
	if (fread(&info, 1, sizeof(elf_info), elf_file) == 0)
	{
		logger(ERROR, "Failed to read elf file info. %s", strerror(errno));
		return -1;
	}

	uint16_t header_count = info.e_shnum;
	logger(DEBUG, "Section header table found at offset %p with %d entries.", (void *)info.e_shoff, header_count);

	// get the section header table
	elf_section_header header_table[header_count];
	if (fseek(elf_file, info.e_shoff, SEEK_SET) < 0)
	{
		logger(ERROR, "Failed to look ahead for header table. %s", strerror(errno));
		return -1;
	}

	if (fread(header_table, sizeof(char), header_count * sizeof(elf_section_header), elf_file) == 0)
	{
		logger(ERROR, "Failed to read header table. %s", strerror(errno));
		return -1;
	}

	elf_section_header names_header = header_table[info.e_shstrndx];
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
	uint64_t debug_line_header_pos = locate_elf_section(DEBUG_LINE_HEADER, name_table, name_table_size, header_table, header_count);
	if (debug_line_header_pos == 0)
	{
		logger(ERROR, "Failed to locate %s section.", DEBUG_LINE_HEADER);
		return -1;
	}

	if (fclose(elf_file) != 0)
	{
		logger(ERROR, "Failed to close file. %s", strerror(errno));
		return -1;
	}
	return 0;
}