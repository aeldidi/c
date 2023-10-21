#define _XOPEN_SOURCE 500
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <fts.h>
#include <limits.h>

#include "c.eldidi.org/c/arena"

extern int errno;

bool
fs_foreach_file(char* dirname, bool (*fn)(char*))
{
	char* paths[] = {dirname, NULL};
	FTS*  dir     = fts_open(
			     paths, FTS_PHYSICAL | FTS_NOCHDIR | FTS_NOSTAT, NULL);
	if (dir == NULL) {
		return false;
	}

	while (fts_read(dir) != NULL) {
		FTSENT* current = fts_children(dir, 0);
		for (; current != NULL; current = current->fts_link) {
			if ((current->fts_info & FTS_F) == 0) {
				continue;
			}

			fn(current->fts_name);
		}
	}

	return fts_close(dir) == 0;
}
