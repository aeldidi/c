#ifndef UTIL_H
#define UTIL_H
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdnoreturn.h>

typedef struct Arena    Arena;
typedef struct StrSlice StrSlice;

// panic exits the program, printing an error message to stderr.
noreturn void panic(const char* format, ...);

// todo exits the program, printing a TODO message.
noreturn void todo(const char* format, ...);

#undef assert
#define assert(...)                                                           \
	((__VA_ARGS__) ? ((void)0)                                            \
		       : panic("assertion failed: %s:%d: %s", __FILE__,       \
					 __LINE__, #__VA_ARGS__))

// find_in_path looks for a binary in PATH, returning NULL if it wasn't found.
char* find_in_path(Arena* mem, Arena scratch, char* binary_name);

typedef struct Bytes {
	bool     ok;
	uint8_t* data;
	size_t   len;
} Bytes;

// readfull reads the file f fully and returns a buffer containing its
// contents. An extra NULL terminator will be appended to the result in the
// but will not be counted in the result's 'len' member.
Bytes readfull(Arena* mem, FILE* f);

// strslice_remove_empty removes all empty strings from the given StrSlice.
void strslice_remove_empty(StrSlice* parts);

#endif // UTIL_H
