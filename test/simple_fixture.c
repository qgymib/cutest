#undef NDEBUG
#include <assert.h>

#include "cutest.h"

TEST_FIXTURE_SETUP(fixture)
{
}

TEST_FIXTURE_TEAREDOWN(fixture)
{
}

TEST_F(fixture, simple)
{
    ASSERT_EQ_D32(0, 0);
}

int main(int argc, char* argv[])
{
    assert(cutest_run_tests(argc, argv, NULL) == 0);
    return 0;
}
