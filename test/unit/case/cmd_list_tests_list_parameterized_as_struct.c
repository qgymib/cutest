#include "test.h"

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

typedef struct as_struct
{
    int         field_a;
    const char* field_b;
} as_struct_t;

TEST_FIXTURE_SETUP(parameterized)
{
}

TEST_FIXTURE_TEAREDOWN(parameterized)
{
}

TEST_PARAMETERIZED_DEFINE(parameterized, as_struct, as_struct_t, { 0, "hello" }, { 1, "world" }, { 99, "hello world" });

TEST_P(parameterized, as_struct)
{
    TEST_PARAMETERIZED_SUPPRESS_UNUSED;
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

DEFINE_TEST(parameterized, as_struct, "--test_list_tests")
{
    const char* pattern;
    string_matrix_t* matrix = string_matrix_create_from_file(_TEST.out, "");

    pattern = "  as_struct/0  # <as_struct_t> { 0, \"hello\" }";
    ASSERT_STRING_EQ(string_matrix_access(matrix, 1, 0), pattern);

    pattern = "  as_struct/1  # <as_struct_t> { 1, \"world\" }";
    ASSERT_STRING_EQ(string_matrix_access(matrix, 2, 0), pattern);

    pattern = "  as_struct/2  # <as_struct_t> { 99, \"hello world\" }";
    ASSERT_STRING_EQ(string_matrix_access(matrix, 3, 0), pattern);

    string_matrix_destroy(matrix);

    TEST_PORTING_ASSERT(_TEST.rret == 0);
}
