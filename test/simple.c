#undef NDEBUG
#include <assert.h>

#include "cutest.h"
#include "assertion_template.h"

TEST_FIXTURE_SETUP(simple)
{
}
TEST_FIXTURE_TEAREDOWN(simple)
{
}
TEST_PARAMETERIZED_DEFINE(simple, parameterized, size_t, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

static size_t s_sum;

TEST_F(simple, fixture)
{
    TEST_ASSERTION;
}
TEST_P(simple, parameterized)
{
    TEST_ASSERTION;
    s_sum += TEST_GET_PARAM();
}
TEST(simple, simple)
{
    TEST_ASSERTION;
}

int main(int argc, char* argv[])
{
    s_sum = 0;
    assert(cutest_run_tests(argc, argv, NULL) == 0);
    assert(s_sum == 55);
    return 0;
}
