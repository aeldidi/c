#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c.eldidi.org/x/arena"
#include "c.eldidi.org/x/fs"
#include "c.eldidi.org/x/process"
#include "c.eldidi.org/x/str"

static uint8_t memory[1 << 16];
static Arena   mem = {
		  .beg = (void*)memory,
		  .end = (void*)memory + (1 << 16),
};

typedef struct FindParams {
	Arena* mem;
	char*  command;
	int    argc;
	char** argv;
	char*  dir;
} FindParams;

bool
find_command_to_run(char* file, void* arg)
{
	FindParams* fp = arg;
	if (strlen(file) <= 1 || file[0] != 'c') {
		return true;
	}

	// printf("comparing '%s' to '%s'\n", file + 1, command);
	if (strcmp(file + 1, fp->command) != 0) {
		return true;
	}

	fp->argv[1] = str_format(&fp->mem, "%s/%s", fp->dir, file);
	process_exec(&mem, fp->argc - 1, fp->argv + 1);
	fprintf(stderr, "error: exec failed\n");
	exit(EXIT_FAILURE);
}

static bool
list_all_commands(char* file, void* arg)
{
	(void)arg;
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
	FindParams fp = {
			.argc    = argc,
			.argv    = argv,
			.command = argv[1],
			.mem     = &mem,
	};

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
	fp.dir      = dir;

	if (argc < 2) {
		puts("usage: c <command> [arguments]\n\nAvailible "
		     "commands:\n");
		fs_foreach_file(dir, list_all_commands, NULL);
		printf("\n");
		return EXIT_SUCCESS;
	}

	if (!fs_foreach_file(dir, find_command_to_run, &fp)) {
		fprintf(stderr, "error: couldn't read directory\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
