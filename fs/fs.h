#ifndef FS_H
#define FS_H
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct Arena Arena;

// fs_foreach runs the given function for each file in the given directory.
// This function can return true to continue iteration, or false to stop
// iteration (equivalent to "continue" and "break").
//
// Returns false if the directory couldn't be opened for some reason.
bool fs_foreach_file(char* dirname, bool (*fn)(char*));

// // fs_resolve resolves a path to a full path.
// //
// // Returns NULL if it can't for some reason.
// char* fs_resolve(Arena* mem, char* name);

#endif // FS_H
