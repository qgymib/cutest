#undef NDEBUG
#include <assert.h>

#include "cutest.h"

TEST_FIXTURE_SETUP(fixture)
{
}

TEST_FIXTURE_TEAREDOWN(fixture)
{
}

TEST_PARAMETERIZED_DEFINE(fixture, int, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10);

static int s_sum;

TEST_P(fixture, simple)
{
    s_sum += TEST_GET_PARAM();
}

int main(int argc, char* argv[])
{
    s_sum = 0;
    assert(cutest_run_tests(argc, argv, NULL) == 0);
    assert(s_sum == 55);

    return 0;
}
