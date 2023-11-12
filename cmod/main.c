#include <assert.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c.eldidi.org/c/modfile"
#include "c.eldidi.org/c/util"
#include "c.eldidi.org/x/arena"

void
fmt_platform_flags(CPlatformFlags* current)
{
	assert(current != NULL);
	char* key = current->key;
	if (key != NULL) {
		printf("os %s ", key);
		CFlags flags = current->value;
		for (size_t i = 0; i < flags.len - 1; i += 1) {
			printf("%s ", flags.data[i]);
		}
		printf("%s\n", flags.data[flags.len - 1]);
	}

	for (size_t i = 0; i < 4; i += 1) {
		if (current->child[i] == NULL) {
			continue;
		}
		fmt_platform_flags(current->child[i]);
	}
}

int
fmt(Arena* mem, Arena scratch, char* path)
{
	CModfile modfile = modfile_parse(mem, scratch, path);
	if (modfile.status < 0) {
		panic(mem, "error reading modfile");
	} else if (modfile.status > 0) {
		fprintf(stderr, "%s:%" PRId64 ": syntax error\n", path,
				modfile.status);
		return EXIT_FAILURE;
	}

	if (modfile.import_path == NULL) {
		fprintf(stderr, "cmod: no 'module' directive specified");
		return EXIT_FAILURE;
	}
	printf("module %s\n", modfile.import_path);

	if (modfile.version != NULL) {
		printf("version %s\n", modfile.version);
	}

	CPlatformFlags* current = &modfile.platform_flags;
	fmt_platform_flags(current);

	return EXIT_SUCCESS;
}

int
usage(char* argv_0)
{
	fprintf(stderr,
			"usage: %s <command> [args...]\n\n"
			"Availible commands:\n"
			"\tinit - initialize a c.mod file in the "
			"current dir\n"
			"\tfmt - formats a given c.mod file to "
			"stdout\n\n",
			argv_0);
	return EXIT_FAILURE;
}

int
main(int argc, char** argv)
{
	if (argc < 2) {
		return usage(argv[0]);
	}

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

	if (strcmp(argv[1], "init") == 0) {
		todo("cmod init");
	}

	if (strcmp(argv[1], "fmt") == 0) {
		if (argc != 3) {
			// is there a c.mod in the current directory?
			StatResult meta = fs_metadata("./c.mod");
			if (meta.status != 0) {
				fprintf(stderr,
						"usage: %s fmt <c.mod "
						"file>\n",
						argv[0]);
				return EXIT_FAILURE;
			}

			char* path = fs_resolve(&mem, scratch, "./c.mod");
			if (path == NULL) {
				panic(&mem, "couldn't resolve './c.mod'");
			}

			int ret = fmt(&mem, scratch, path);
			free(memory);
			free(mem_scratch);
			return ret;
		}

		char* path = fs_resolve(&mem, scratch, argv[2]);
		if (path == NULL) {
			panic(&mem, "couldn't resolve '%s'", argv[2]);
		}

		int ret = fmt(&mem, scratch, path);
		free(memory);
		free(mem_scratch);
		return ret;
	}

	int ret = usage(argv[0]);
	free(memory);
	free(mem_scratch);
	return ret;
}
