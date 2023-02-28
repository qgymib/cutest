#include "cutest.h"

//! [DEFINE_SIMPLE_TEST]
TEST(example, test)
{
	ASSERT_LT_INT(0, 1);
}
//! [DEFINE_SIMPLE_TEST]
