#define TEST_INITIALIZER(f) \
    void f(void)

#include "test.h"

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

typedef struct test_manual_reigster
{
    int count_0;
} test_manual_reigster_t;

static test_manual_reigster_t s_test_manual_reigster;

TEST(manual_register, t0)
{
    s_test_manual_reigster.count_0++;
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

#define MY_TEST_TABLE(xx)   \
    xx(manual_register, t0)

MY_TEST_TABLE(TEST_MANUAL_DECLARE_TEST_INTERFACE)

DEFINE_TEST_SETUP(manual_register)
{
    memset(&s_test_manual_reigster, 0, sizeof(s_test_manual_reigster));

    MY_TEST_TABLE(TEST_MANUAL_REGISTER_TEST_INTERFACE);
}

DEFINE_TEST_TEARDOWN(manual_register)
{
}

DEFINE_TEST_F(manual_register, check)
{
    TEST_PORTING_ASSERT(s_test_manual_reigster.count_0 == 1);
}
