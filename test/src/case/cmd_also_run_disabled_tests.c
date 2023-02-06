#include "test.h"
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

typedef struct disabled_ctx
{
	int point_0;
	int point_1;
} disabled_ctx_t;

static disabled_ctx_t g_disabled_ctx;

TEST(disabled, DISABLED_point_0)
{
	g_disabled_ctx.point_0++;
}

TEST(disabled, disabled_point_1)
{
	g_disabled_ctx.point_1++;
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

DEFINE_TEST_SETUP(disabled)
{
	memset(&g_disabled_ctx, 0, sizeof(g_disabled_ctx));
}

DEFINE_TEST_TEARDOWN(disabled)
{
}

DEFINE_TEST_F(disabled, normal, "")
{
	TEST_PORTING_ASSERT(g_disabled_ctx.point_0 == 0);
	TEST_PORTING_ASSERT(g_disabled_ctx.point_1 == 1);
}

DEFINE_TEST_F(disabled, run_all, "--test_also_run_disabled_tests")
{
	TEST_PORTING_ASSERT(g_disabled_ctx.point_0 == 1);
	TEST_PORTING_ASSERT(g_disabled_ctx.point_1 == 1);
}
