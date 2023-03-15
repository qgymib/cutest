#include "cutest.h"

TEST_FIXTURE_SETUP(example)
{
}

TEST_FIXTURE_TEARDOWN(example)
{
}

TEST_F(example, test_f_0)
{
	ASSERT_NE_INT(0, 1);
}

TEST_F(example, test_f_1)
{
	ASSERT_EQ_INT(0, 0);
}
