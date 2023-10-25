#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "github.com/aeldidi/encoding/hex"
#include "python.eldidi.org/cbuild/tests/simple_remote/sub"

uint8_t out[1 << 16] = {0};
int
main()
{
	uint8_t data[] = "Lorem ipsum dolor sit amet, consectetur adipiscing "
			 "elit, sed do eiusmod tempor incididunt ut labore et "
			 "dolore magna aliqua. Ut enim ad minim veniam, quis "
			 "nostrud exercitation ullamco laboris nisi ut "
			 "aliquip ex ea commodo consequat. Duis aute irure "
			 "dolor in reprehenderit in voluptate velit esse "
			 "cillum dolore eu fugiat nulla pariatur. Excepteur "
			 "sint occaecat cupidatat non proident, sunt in culpa "
			 "qui officia deserunt mollit anim id est laborum.";
	hex_dump(sizeof(data) / sizeof(data[0]), data, 1 << 16, out, NULL);
	printf("%s\n", out);
	coolio();
}
