#include "test.h"

DEFINE_TEST(arg, count)
{
    assert(TEST_ARG_COUNT(1, 2) == 2);
    assert(TEST_ARG_COUNT(3, 4, 5) == 3);
    assert(TEST_ARG_COUNT({ 6, 7 }) == 2);
}
