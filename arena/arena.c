#include <inttypes.h>
#include <setjmp.h>
#include <stddef.h>
#include <string.h>

void*
arena_alloc(Arena* a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count,
		uint32_t flags)
{
	ptrdiff_t avail   = a->end - a->beg;
	ptrdiff_t padding = -(uintptr_t)a->beg & (align - 1);
	if (count > (avail - padding) / size) {
		longjmp(*((jmp_buf*)a->jmp_buf), 1);
	}

	ptrdiff_t total = size * count;
	uint8_t*  p     = (void*)(a->beg + padding);
	a->beg += padding + total;
	if (flags & ARENA_NOZERO) {
		return p;
	}

	return memset(p, 0, total);
}
