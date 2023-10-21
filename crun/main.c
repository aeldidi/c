#include <stdio.h>

#include "c.eldidi.org/c/arena"
#include "c.eldidi.org/c/process"
#include "c.eldidi.org/c/str"

static char  memory[1 << 16];
static Arena mem = {
		.beg = (void*)memory,
		.end = (void*)memory + (1 << 16),
};

// Meant to be equivalent to `c build` and then manually running the
// executable.
int
main()
{
	puts("TODO: actually implement this");
}
