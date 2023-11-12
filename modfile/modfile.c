#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "c.eldidi.org/c/util"
#include "c.eldidi.org/x/containers"
#include "c.eldidi.org/x/hash"
#include "c.eldidi.org/x/str"

CFlags*
CPlatformFlags_get(Arena* arena, CPlatformFlags** map, char* k)
{
	assert(map != NULL);
	for (uint64_t h = fnv_1a_str(k); *map; h <<= 2) {
		if ((*map)->key != NULL && strcmp(k, (*map)->key) == 0) {
			return &(*map)->value;
		}
		map = &(*map)->child[h >> 62];
	}

	if (!arena) {
		return NULL;
	}

	*map        = arena_make(arena, CPlatformFlags);
	(*map)->key = k;
	return &(*map)->value;
}

CModfile
modfile_parse(Arena* mem, Arena scratch, char* path)
{
	CModfile result = {};

	FILE* f = fopen(path, "rb");
	if (f == NULL) {
		result.status = -1;
		return result;
	}

	Bytes cmod = readfull(&scratch, f);
	if (!cmod.ok) {
		result.status = -1;
		return result;
	}

	CPlatformFlags* platform_flags = &result.platform_flags;
	StrSlice        lines = str_split(&scratch, (char*)cmod.data, '\n');
	for (size_t i = 0; i < lines.len; i += 1) {
		Arena tmp  = scratch;
		char* line = (char*)lines.data[i];
		// TODO: handle all types of whitespace.
		StrSlice parts = str_split(&tmp, line, ' ');
		if (parts.len == 0) {
			continue;
		}

		if (parts.len < 2) {
			result.status = i + 1;
			return result;
		}

		if (strcmp(parts.data[0], "module") == 0) {
			if (parts.len != 2 || result.import_path != NULL) {
				result.status = i + 1;
				return result;
			}

			result.import_path =
					str_format(mem, "%s", parts.data[1]);
			continue;
		}

		if (strcmp(parts.data[0], "version") == 0) {
			if (parts.len != 2 || result.version != NULL) {
				result.status = i + 1;
				return result;
			}

			result.version = str_format(mem, "%s", parts.data[1]);
			continue;
		}

		if (strcmp(parts.data[0], "os") == 0) {
			// TODO: only a set amount of OSs should be recognized.
			if (parts.len < 3) {
				result.status = i + 1;
				return result;
			}

			CFlags* flags = CPlatformFlags_get(mem,
					&platform_flags,
					str_format(mem, "%s", parts.data[1]));
			for (size_t j = 2; j < parts.len; j += 1) {
				char* tmp = str_format(
						mem, "%s", parts.data[j]);
				*slice_push(mem, flags) = tmp;
			}

			continue;
		}

		result.status = i + 1;
		return result;
	}

	return result;
}