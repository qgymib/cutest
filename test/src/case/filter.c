#include "test.h"
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

typedef struct test_filter
{
    size_t cnt_p1;
    size_t cnt_p2;
    size_t cnt_p3;
} test_filter_t;

static test_filter_t s_test_filter;

TEST_FIXTURE_SETUP(filter) {}
TEST_FIXTURE_TEAREDOWN(filter) {}

TEST(filter, p1)
{
    s_test_filter.cnt_p1++;
}

TEST_F(filter, p2)
{
    s_test_filter.cnt_p2++;
}

TEST_PARAMETERIZED_DEFINE(filter, p3, size_t, 1, 2, 3);
TEST_P(filter, p3)
{
    s_test_filter.cnt_p3 += TEST_GET_PARAM();
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

DEFINE_SETUP(filter)
{
    memset(&s_test_filter, 0, sizeof(s_test_filter));
}

DEFINE_TEARDOWN(filter)
{
}

/**
 * filter:
 * + filter.p1
 * + filter.p2
 * + filter.p3/0
 * + filter.p3/1
 * + filter.p3/2
 */
DEFINE_TEST_F(filter, any, "--test_filter=*")
{
    assert(s_test_filter.cnt_p1 == 1);
    assert(s_test_filter.cnt_p2 == 1);
    assert(s_test_filter.cnt_p3 == 6);
}

DEFINE_TEST_F(filter, asdf, "--test_filter=asdf")
{
    assert(s_test_filter.cnt_p1 == 0);
    assert(s_test_filter.cnt_p2 == 0);
    assert(s_test_filter.cnt_p3 == 0);
}

/**
 * filter:
 * + filter.p1
 * + filter.p2
 * + filter.p3/0
 * + filter.p3/1
 * + filter.p3/2
 */
DEFINE_TEST_F(filter, filter_dot_any, "--test_filter=filter.*")
{
    assert(s_test_filter.cnt_p1 == 1);
    assert(s_test_filter.cnt_p2 == 1);
    assert(s_test_filter.cnt_p3 == 6);
}

/**
 * filter:
 * + filter.p1
 * + filter.p2
 */
DEFINE_TEST_F(filter, p_dot_ask, "--test_filter=filter.p?")
{
    assert(s_test_filter.cnt_p1 == 1);
    assert(s_test_filter.cnt_p2 == 1);
    assert(s_test_filter.cnt_p3 == 0);
}

/**
 * filter:
 * + filter.p3/0
 * + filter.p3/1
 * + filter.p3/2
 */
DEFINE_TEST_F(filter, any_slash_any, "--test_filter=*/*")
{
    assert(s_test_filter.cnt_p1 == 0);
    assert(s_test_filter.cnt_p2 == 0);
    assert(s_test_filter.cnt_p3 == 6);
}
