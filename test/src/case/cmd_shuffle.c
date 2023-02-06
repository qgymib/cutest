#include "test.h"

#define TEST_SUIT_SZ	10
#define TEST_SUIT_DATA	0, 1, 2, 3, 4, 5, 6, 7, 8, 9

typedef struct test_ctx
{
	int arr[TEST_SUIT_SZ];
	int counter;
} test_ctx_t;

static test_ctx_t s_test_ctx;
static int s_test_dat[] = { TEST_SUIT_DATA };

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

TEST_FIXTURE_SETUP(cmd)
{
}

TEST_FIXTURE_TEAREDOWN(cmd)
{
}

TEST_PARAMETERIZED_DEFINE(cmd, shuffle, int, TEST_SUIT_DATA);

TEST_P(cmd, shuffle)
{
	TEST_PORTING_ASSERT(TEST_GET_PARAM() < (int)(sizeof(s_test_ctx.arr) / sizeof(s_test_ctx.arr[0])));
	s_test_ctx.arr[TEST_GET_PARAM()] = s_test_ctx.counter++;
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

DEFINE_TEST_SETUP(cmd)
{
	memset(&s_test_ctx, 0, sizeof(s_test_ctx));
}

DEFINE_TEST_TEARDOWN(cmd)
{
}

DEFINE_TEST_F(cmd, no_shuffle, "--test_random_seed=1")
{
	unsigned i;
	for (i = 0; i < TEST_SUIT_SZ; i++)
	{
		TEST_PORTING_ASSERT(s_test_dat[i] == s_test_ctx.arr[i]);
	}
}

DEFINE_TEST_F(cmd, shuffle, "--test_shuffle", "--test_random_seed=1")
{
	unsigned i;
	unsigned counter = 0;
	for (i = 0; i < TEST_SUIT_SZ; i++)
	{
		if (s_test_dat[i] != s_test_ctx.arr[i])
		{
			counter++;
		}
	}

	TEST_PORTING_ASSERT(counter != 0);
}
