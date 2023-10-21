#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c.eldidi.org/c/arena"
#include "c.eldidi.org/c/fs"
#include "c.eldidi.org/c/str"

char  memory[1 << 16];
Arena mem = {
		.beg = (void*)memory,
		.end = (void*)memory + (1 << 16),
};

static bool
list_all_tools(char* file)
{
	if (strlen(file) <= 1 || file[0] != 'c') {
		return true;
	}

	printf("\t%s\n", file + 1);
	return true;
}

// ctool builds a specified .c file from the `tools/` subdirectory into an
// executable and runs it with the given arguments.
//
// The binary doesn't go in the current folder. It goes in the cache folder.
int
main(int argc, char** argv)
{
	jmp_buf jb;
	if (setjmp(jb)) {
		fprintf(stderr, "error: allocation failure\n");
		return EXIT_FAILURE;
	}
	mem.jmp_buf = &jb;

	if (argc < 2) {
		printf("usage: c tool <name of tool> [arguments]\n\n");
		printf("Availible tools:");
		fs_foreach_file("tools", list_all_tools);
		puts("");
	}

	char* cache_dir = getenv("XDG_CACHE_HOME");
	if (cache_dir == NULL) {
		cache_dir = ".cache/c";
	} else {
		cache_dir = str_format(&mem, "%s/c", cache_dir);
	}

	puts("TODO: parse c.mod to find module name, build tool and run");
}
