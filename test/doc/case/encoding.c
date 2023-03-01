#include "test.h"

static const char* s_warning =
"The file format in source file is not LF only. Please remove `\\r' in source file.\n"
"If you are using git, please set config `core.autocrlf` to `false` and clone\n"
"again.";

TEST(encoding, cutest_h)
{
    size_t i;
    for (i = 0; i < cutest_h_hex_sz; i++)
    {
        ASSERT_NE_CHAR(cutest_h_hex[i], '\r', "%s", s_warning);
    }
}

TEST(encoding, cutest_c)
{
    size_t i;
    for (i = 0; i < cutest_c_hex_sz; i++)
    {
        ASSERT_NE_CHAR(cutest_c_hex[i], '\r', "%s", s_warning);
    }
}

TEST(encoding, README_md)
{
    size_t i;
    for (i = 0; i < readme_md_hex_sz; i++)
    {
        ASSERT_NE_CHAR(readme_md_hex[i], '\r', "%s", s_warning);
    }
}
