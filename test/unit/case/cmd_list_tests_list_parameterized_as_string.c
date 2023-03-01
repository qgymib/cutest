#include "test.h"

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

TEST_FIXTURE_SETUP(parameterized)
{
}

TEST_FIXTURE_TEAREDOWN(parameterized)
{
}

TEST_PARAMETERIZED_DEFINE(parameterized, as_string, const char*, "hello", "world", "a space", "two" "string");

TEST_P(parameterized, as_string)
{
    TEST_PARAMETERIZED_SUPPRESS_UNUSED;
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

DEFINE_TEST(parameterized, as_string, "--test_list_tests")
{
    const char* pattern;
    string_matrix_t* matrix = string_matrix_create_from_file(_TEST.out, "");

    pattern = "  as_string/0  # <const char*> \"hello\"";
    ASSERT_STRING_EQ(string_matrix_access(matrix, 1, 0), pattern);

    pattern = "  as_string/1  # <const char*> \"world\"";
    ASSERT_STRING_EQ(string_matrix_access(matrix, 2, 0), pattern);

    pattern = "  as_string/2  # <const char*> \"a space\"";
    ASSERT_STRING_EQ(string_matrix_access(matrix, 3, 0), pattern);

    pattern = "  as_string/3  # <const char*> \"two\" \"string\"";
    ASSERT_STRING_EQ(string_matrix_access(matrix, 4, 0), pattern);

    string_matrix_destroy(matrix);

    TEST_PORTING_ASSERT(_TEST.rret == 0);
}
