#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "c.eldidi.org/c/arena"

void
process_exec(Arena* temp, int argc, char* argv[])
{
	char** exec_argv = arena_make(temp, char*, argc + 1);
	for (int i = 0; i < argc; i += 1) {
		exec_argv[i] = argv[i];
	}

	// printf("execing: ");
	// for (int i = 0; i < argc; i += 1) {
	// 	printf("%s ", argv[i]);
	// }
	// puts("");
	execv(exec_argv[0], exec_argv);
}
