#include "test.h"
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

TEST_FIXTURE_SETUP(simple)
{
}
TEST_FIXTURE_TEAREDOWN(simple)
{
}
TEST_PARAMETERIZED_DEFINE(simple, parameterized, size_t, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

static size_t s_sum = 0;

TEST_F(simple, fixture)
{
    ASSERT_EQ_STR("a", "a");
}
TEST_P(simple, parameterized)
{
    ASSERT_NE_INT32(0, 1);
    s_sum += TEST_GET_PARAM();
}
TEST(simple, simple)
{
    ASSERT_LT_FLOAT(0, 1);
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

DEFINE_TEST(simple, test)
{
    TEST_PORTING_ASSERT(s_sum == 55);
}
