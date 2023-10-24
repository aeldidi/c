#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

#include "c.eldidi.org/x/arena"
#include "c.eldidi.org/x/process"

char  memory[1 << 16];
Arena mem = {
		.beg = (void*)memory,
		.end = (void*)memory + (1 << 16),
};

// A wrapper for clang. Allows calling the configured clang without manually
// looking at which one is set.
int
main(int argc, char** argv)
{
	jmp_buf jb;
	if (setjmp(jb)) {
		fprintf(stderr, "error: allocation failure");
		return EXIT_FAILURE;
	}

	mem.jmp_buf = &jb;

	char* clang_cmd = getenv("C_CLANG_COMMAND");
	if (clang_cmd == NULL) {
		clang_cmd = "clang";
	}

	argv[0] = clang_cmd;
	process_exec(&mem, argc, argv);
	fprintf(stderr, "exec failed\n");
	return EXIT_FAILURE;
}
