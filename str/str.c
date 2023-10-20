#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "c.eldidi.org/c/arena"

char*
str_format(Arena* mem, char* format, ...)
{
	va_list ap;
	va_list bp;
	va_start(ap, format);
	va_copy(bp, ap);
	ptrdiff_t size   = vsnprintf(NULL, 0, format, ap);
	char*     result = arena_make(mem, char, size + 1);
	vsnprintf(result, size + 1, format, bp);
	va_end(ap);
	return result;
}
