#ifndef UTIL_H
#define UTIL_H
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdnoreturn.h>

typedef struct Arena Arena;

// panic exits the program, printing an error message to stderr.
noreturn void panic(Arena* mem, const char* format, ...);

// todo exits the program, printing a TODO message.
noreturn void todo(const char* format, ...);

#endif // UTIL_H
