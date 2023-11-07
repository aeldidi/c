#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c.eldidi.org/c/util"
#include "c.eldidi.org/x/arena"
#include "c.eldidi.org/x/fs"
#include "c.eldidi.org/x/process"
#include "c.eldidi.org/x/str"

typedef struct FindParams {
	Arena* mem;
	Arena  scratch;
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

	fp->argv[1] = str_format(fp->mem, "%s/%s", fp->dir, file);
	process_exec(fp->mem, fp->scratch, fp->argc - 1, fp->argv + 1);
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
	char*   memory         = malloc(1 << 16);
	char*   memory_scratch = malloc(1 << 16);
	jmp_buf jb;
	if (memory == NULL || memory_scratch == NULL || setjmp(jb)) {
		// allocation error
		free(memory);
		free(memory_scratch);
		fprintf(stderr, "error: out of memory\n");
		return EXIT_FAILURE;
	}
	Arena mem = {
			.beg     = memory,
			.end     = memory + (1 << 16),
			.jmp_buf = jb,
	};
	Arena scratch = {
			.beg     = memory_scratch,
			.end     = memory_scratch + (1 << 16),
			.jmp_buf = jb,
	};

	FindParams fp = {
			.argc    = argc,
			.argv    = argv,
			.command = argv[1],
			.mem     = &mem,
			.scratch = scratch,
	};

	char* C_ROOT = getenv("C_ROOT");
	if (C_ROOT == NULL) {
		C_ROOT = "/c";
	}

	char* dir = str_format(&mem, "%s/commands", C_ROOT);
	fp.dir    = dir;

	if (argc < 2) {
		puts("usage: c <command> [arguments]\n\nAvailible "
		     "commands:\n");
		fs_foreach_file(dir, list_all_commands, NULL);
		printf("\n");
		free(memory);
		free(memory_scratch);
		return EXIT_SUCCESS;
	}

	if (!fs_foreach_file(dir, find_command_to_run, &fp)) {
		fprintf(stderr, "error: couldn't read directory\n");
		free(memory);
		free(memory_scratch);
		return EXIT_FAILURE;
	}

	free(memory);
	free(memory_scratch);
	return EXIT_SUCCESS;
}
