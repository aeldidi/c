#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "c.eldidi.org/c/util"
#include "c.eldidi.org/x/arena"
#include "c.eldidi.org/x/fs"
#include "c.eldidi.org/x/process"
#include "c.eldidi.org/x/str"

// Meant to be equivalent to `c build` and then manually running the
// executable.
// TODO: run the main executable if it exists.
int
main(int argc, char** argv)
{
	if (argc > 2) {
		todo("handle arguments so they're not all forwarded to "
		     "cbuild");
	}

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

	// argv = ['/path/to/crun', 'program', args..]
	char* argv_0 = argv[0];
	argv[0]      = str_format(&mem, "%s/commands/cbuild", C_ROOT);
	Process p    = process_spawn(scratch, argc, argv);
	if (p.status != 0) {
		panic(&mem, "failed to launch process");
	}
	(void)process_wait(p, true);

	if (argc == 1) {
		// run main executable
		todo("parse modfile and run executable with the modname");
		process_exec(&mem, scratch, argc, argv);
	}

	// TODO: support remote modules
	char* exe       = str_format(&mem, "./%s/%s", argv[1], argv[1]);
	exe             = fs_resolve(&mem, scratch, exe);
	StatResult meta = fs_metadata(exe);
	if (meta.status != 0) {
		free(memory);
		free(memory_scratch);
		return EXIT_FAILURE;
	}

	argv += 1;
	argc -= 1;
	argv[0] = exe;
	process_exec(&mem, scratch, argc, argv);
	panic(&mem, "error: couldn't exec binary '%s'", exe);
}
