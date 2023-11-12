#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c.eldidi.org/c/util"
#include "c.eldidi.org/x/arena"
#include "c.eldidi.org/x/process"

// A wrapper for clang. Allows calling the configured clang without manually
// looking at which one is set.
int
main(int argc, char** argv)
{
	char*   memory      = malloc(1 << 16);
	char*   mem_scratch = malloc(1 << 16);
	jmp_buf jb;
	if (memory == NULL || mem_scratch == NULL || setjmp(jb)) {
		free(memory);
		free(mem_scratch);
		fprintf(stderr, "error: allocation failure");
		return EXIT_FAILURE;
	}
	Arena mem = {
			.beg     = (void*)memory,
			.end     = (void*)memory + (1 << 16),
			.jmp_buf = jb,
	};
	Arena scratch = {
			.beg     = (void*)mem_scratch,
			.end     = (void*)mem_scratch + (1 << 16),
			.jmp_buf = jb,
	};

	char* clang_cmd = getenv("C_CLANG_COMMAND");
	if (clang_cmd == NULL) {
		clang_cmd = find_in_path(&mem, scratch, "clang");
		if (clang_cmd == NULL) {
			free(memory);
			free(mem_scratch);
			fprintf(stderr, "error: "
					"C_CLANG_COMMAND not set and "
					"'clang' not found in PATH\n");
			return EXIT_FAILURE;
		}
	}

	argv[0] = clang_cmd;
	process_exec(&mem, mem, argc, argv);
	fprintf(stderr, "error: failed to execute '%s'\n", clang_cmd);
	free(memory);
	free(mem_scratch);
	return EXIT_FAILURE;
}
