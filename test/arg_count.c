#undef NDEBUG
#include "cutest.h"
#include <assert.h>

int main(int argc, char* argv[])
{
	(void)argc; (void)argv;

	assert(TEST_ARG_COUNT(1, 2) == 2);
	assert(TEST_ARG_COUNT(3, 4, 5) == 3);
	assert(TEST_ARG_COUNT({6, 7}) == 2);

	return 0;
}
