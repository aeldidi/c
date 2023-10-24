#include <stdio.h>

#include "c.eldidi.org/x/arena"
#include "c.eldidi.org/x/process"
#include "c.eldidi.org/x/str"

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
