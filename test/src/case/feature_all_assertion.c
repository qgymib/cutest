#include "test.h"

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

TEST(all_assertion, 0)
{
    ASSERT_EQ_INT(0, 0);
    ASSERT_NE_INT(0, 1);
    ASSERT_LT_INT(0, 1);
    ASSERT_LE_INT(0, 1);
    ASSERT_GT_INT(1, 0);
    ASSERT_GE_INT(1, 0);

    ASSERT_EQ_UINT(0, 0);
    ASSERT_NE_UINT(0, 1);
    ASSERT_LT_UINT(0, 1);
    ASSERT_LE_UINT(0, 1);
    ASSERT_GT_UINT(1, 0);
    ASSERT_GE_UINT(1, 0);

    ASSERT_EQ_LONG(0, 0);
    ASSERT_NE_LONG(0, 1);
    ASSERT_LT_LONG(0, 1);
    ASSERT_LE_LONG(0, 1);
    ASSERT_GT_LONG(1, 0);
    ASSERT_GE_LONG(1, 0);

    ASSERT_EQ_ULONG(0, 0);
    ASSERT_NE_ULONG(0, 1);
    ASSERT_LT_ULONG(0, 1);
    ASSERT_LE_ULONG(0, 1);
    ASSERT_GT_ULONG(1, 0);
    ASSERT_GE_ULONG(1, 0);

    ASSERT_EQ_PTR((void*)0, (void*)0);
    ASSERT_NE_PTR((void*)0, (void*)1);
    ASSERT_LT_PTR((void*)0, (void*)1);
    ASSERT_LE_PTR((void*)0, (void*)1);
    ASSERT_GT_PTR((void*)1, (void*)0);
    ASSERT_GE_PTR((void*)1, (void*)0);

    ASSERT_EQ_FLOAT(0, 0);
    ASSERT_NE_FLOAT(0, 1);
    ASSERT_LT_FLOAT(0, 1);
    ASSERT_LE_FLOAT(0, 1);
    ASSERT_GT_FLOAT(1, 0);
    ASSERT_GE_FLOAT(1, 0);

    ASSERT_EQ_DOUBLE(0, 0);
    ASSERT_NE_DOUBLE(0, 1);
    ASSERT_LT_DOUBLE(0, 1);
    ASSERT_LE_DOUBLE(0, 1);
    ASSERT_GT_DOUBLE(1, 0);
    ASSERT_GE_DOUBLE(1, 0);

    ASSERT_EQ_STR("a", "a");
    ASSERT_NE_STR("a", "");
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

DEFINE_TEST(all_assertion, 0)
{
}
