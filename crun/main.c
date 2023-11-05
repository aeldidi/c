#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "c.eldidi.org/x/arena"
#include "c.eldidi.org/x/process"
#include "c.eldidi.org/x/str"

// Meant to be equivalent to `c build` and then manually running the
// executable.
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

	char* C_ROOT = getenv("C_ROOT");
	if (C_ROOT == NULL) {
		C_ROOT = "/c";
	}

	argv[0] = str_format(&mem, "%s/commands/cbuild", C_ROOT);
	process_exec(&mem, scratch, argc, argv);
}
