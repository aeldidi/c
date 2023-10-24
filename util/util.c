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

	Backtrace b = backtrace(mem, 0, 1);
	if (b.status < 0) {
		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "backtrace (most recent call last):\n");
	for (size_t i = 0; i < b.len; i += 1) {
		fprintf(stderr, "%" PRIxPTR "\n", b.addresses[i]);
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
	exit(EXIT_FAILURE);
}
