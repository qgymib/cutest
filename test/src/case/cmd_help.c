#include "test.h"

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

static int counter = 0;

TEST(help, print)
{
	counter++;
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

DEFINE_TEST(help, print0, "--help")
{
	TEST_PORTING_ASSERT(counter == 0);
}

DEFINE_TEST(help, print1, "-h")
{
	TEST_PORTING_ASSERT(counter == 0);
}
