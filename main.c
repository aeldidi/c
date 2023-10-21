#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c.eldidi.org/c/arena"
#include "c.eldidi.org/c/fs"
#include "c.eldidi.org/c/process"
#include "c.eldidi.org/c/str"

static uint8_t memory[1 << 16];
static char*   command = NULL;
static Arena   mem     = {
		      .beg = (void*)memory,
		      .end = (void*)memory + (1 << 16),
};
static int    _argc;
static char** _argv;
static char*  _dir;

bool
find_command_to_run(char* file)
{
	if (strlen(file) <= 1 || file[0] != 'c') {
		return true;
	}

	// printf("comparing '%s' to '%s'\n", file + 1, command);
	if (strcmp(file + 1, command) != 0) {
		return true;
	}

	_argv[1] = str_format(&mem, "%s/%s", _dir, file);
	process_exec(&mem, _argc - 1, _argv + 1);
	fprintf(stderr, "error: exec failed\n");
	exit(EXIT_FAILURE);
}

static bool
list_all_commands(char* file)
{
	if (strlen(file) <= 1 || file[0] != 'c') {
		return true;
	}

	printf("\t%s\n", file + 1);
	return true;
}

// The main `c` tool. This just runs the other tools from the configured
// `C_ROOT` directory.
int
main(int argc, char** argv)
{
	_argc   = argc;
	_argv   = argv;
	command = argv[1];

	char* C_ROOT = getenv("C_ROOT");
	if (C_ROOT == NULL) {
		C_ROOT = "/c";
	}

	jmp_buf jb;
	if (setjmp(jb)) {
		// allocation error
		fprintf(stderr, "error: out of memory\n");
		return EXIT_FAILURE;
	}
	mem.jmp_buf = &jb;
	char* dir   = str_format(&mem, "%s/commands", C_ROOT);
	_dir        = dir;

	if (argc < 2) {
		puts("usage: c <command> [arguments]\n\nAvailible "
		     "commands:\n");
		fs_foreach_file(dir, list_all_commands);
		printf("\n");
		return EXIT_SUCCESS;
	}

	if (!fs_foreach_file(dir, find_command_to_run)) {
		fprintf(stderr, "error: couldn't read directory\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
