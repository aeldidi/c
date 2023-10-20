#ifndef PROCESS_H
#define PROCESS_H
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct Arena Arena;

// process_exec is a cross platform version of UNIX exec(). Only returns if it
// fails.
void process_exec(Arena* temp, int argc, char* argv[]);

#endif // PROCESS_H
