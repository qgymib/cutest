#include "test.h"

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

TEST_FIXTURE_SETUP(parameterized)
{
}

TEST_FIXTURE_TEARDOWN(parameterized)
{
}

TEST_PARAMETERIZED_DEFINE(parameterized, as_int, int, 0,     1   , 2, 3);

TEST_P(parameterized, as_int)
{
    TEST_PARAMETERIZED_SUPPRESS_UNUSED;
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

DEFINE_TEST(parameterized, as_int, "--test_list_tests")
{
    string_matrix_t* matrix = string_matrix_create_from_file(_TEST.out, " ");

    ASSERT_STRING_EQ(string_matrix_access(matrix, 1, 3), "0");
    ASSERT_STRING_EQ(string_matrix_access(matrix, 2, 3), "1");
    ASSERT_STRING_EQ(string_matrix_access(matrix, 3, 3), "2");
    ASSERT_STRING_EQ(string_matrix_access(matrix, 4, 3), "3");

    string_matrix_destroy(matrix);

    TEST_PORTING_ASSERT(_TEST.rret == 0);
}
