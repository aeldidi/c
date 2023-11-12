#ifndef C_MODFILE_H
#define C_MODFILE_H
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct Arena Arena;

typedef struct CFlags CFlags;
struct CFlags {
	char** data;
	size_t len;
	size_t cap;
};

typedef struct CPlatformFlags CPlatformFlags;
struct CPlatformFlags {
	CPlatformFlags* child[4];
	char*           key;
	CFlags          value;
};

// CModfile is the result of parsing a c.mod file.
typedef struct CModfile {
	int64_t        status;
	char*          import_path;
	char*          version;
	CPlatformFlags platform_flags;
} CModfile;

// modfile_parse parses the given path to a c.mod file.
//
// If status is 0, the parse succeeded.
// If status is negative, some error occurred before parsing could be done.
// Otherwise status is the line where an error occurred.
CModfile modfile_parse(Arena* mem, Arena scratch, char* path);

#endif // C_MODFILE_H
