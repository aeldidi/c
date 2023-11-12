#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>

#include "c.eldidi.org/x/arena"
#include "c.eldidi.org/x/backtrace"

// TODO: make this print the lines for each backtrace address if we can.
noreturn void
panic(Arena* mem, const char* format, ...)
{
	fprintf(stderr, "panic: ");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, "\n");

	Backtrace b = backtrace(mem, 0, 1);
	if (b.status < 0) {
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "backtrace (most recent call last):\n");
	for (size_t i = 0; i < b.len; i += 1) {
		fprintf(stderr, "%" PRIxPTR "\n", b.data[i]);
	}
	exit(EXIT_FAILURE);
}

// todo exits the program, printing a TODO message.
noreturn void
todo(const char* format, ...)
{
	fprintf(stderr, "reached unimplemented code: ");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

typedef struct FindInPathArg {
	Arena* mem;
	Arena  scratch;
	char*  result;
	char*  dir;
	char*  target;
} FindInPathArg;

bool
find_in_path_fn(char* path, void* arg)
{
	FindInPathArg* result = arg;
	char* file      = str_format(result->mem, "%s/%s", result->dir, path);
	file            = fs_resolve(result->mem, result->scratch, file);
	StatResult stat = fs_metadata(file);
	if (stat.status != 0) {
		return true;
	}

	if (strcmp(path, result->target) != 0) {
		return true;
	}

	result->result = file;
	return false;
}

char*
find_in_path(Arena* mem, Arena scratch, char* binary_name)
{
	assert(binary_name != NULL);
	char* PATH = getenv("PATH");
	if (PATH == NULL) {
		return NULL;
	}

	StrSlice paths = str_split(&scratch, PATH, ':');
	for (size_t i = 0; i < paths.len; i += 1) {
		Arena         tmp = scratch;
		FindInPathArg fp  = {
				 .dir     = paths.data[i],
				 .mem     = &tmp,
				 .scratch = tmp,
				 .target  = binary_name,
                };
		(void)fs_foreach_file(paths.data[i], find_in_path_fn, &fp);
		if (fp.result == NULL) {
			continue;
		}

		char* result = str_format(mem, "%s", fp.result);
		return result;
	}

	return NULL;
}

Bytes
readfull(Arena* mem, FILE* f)
{
	Bytes result = {.ok = false};
	fseek(f, 0, SEEK_END);
	size_t len = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t* tmp = arena_make(mem, uint8_t, len + 1);
	result.data  = tmp;

	if (fread(tmp, 1, len, f) != len) {
		free(tmp);
		return result;
	}

	result.ok  = true;
	result.len = len;
	return result;
}
