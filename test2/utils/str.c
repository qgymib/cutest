#include <stdlib.h>
#include <string.h>
#include "str.h"

void test_str_exit(test_str_t* str)
{
    free(str->str);
    str->str = NULL;
    str->len = 0;
    str->cap = 0;
}

static void _test_str_ensure_cap(test_str_t* str, size_t cap)
{
    if (str->cap >= cap)
    {
        return;
    }

    char* new_str = realloc(str->str, cap);
    if (new_str == NULL)
    {
        abort();
    }
    str->str = new_str;
    str->cap = cap;
}

void test_str_append(test_str_t* str, const char* data, size_t len)
{
    size_t required_sz = str->len + len;
    _test_str_ensure_cap(str, required_sz + 1);

    memcpy(str->str + str->len, data, len);
    str->str[required_sz] = '\0';
    str->len = required_sz;
}
