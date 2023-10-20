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
	// printf("'%s'\n", dirname);
	char* paths[] = {dirname, NULL};
	FTS*  dir     = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
	if (dir == NULL) {
		perror("fts_open");
		return false;
	}

	for (;;) {
		errno           = 0;
		FTSENT* current = fts_read(dir);
		if (current == NULL) {
			if (errno != 0) {
				perror("fts_read");
				return false;
			}

			// we're done.
			break;
		}

		if (current->fts_info & FTS_F) {
			fn(current->fts_name);
		} else if (current->fts_info & FTS_D) {
			// TODO
		} else if (current->fts_info & FTS_DP) {
			// TODO
		} else {
			// TODO
		}
	}

	return fts_close(dir) == 0;
}
