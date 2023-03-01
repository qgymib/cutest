#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "strtok_f.h"
#include "foreachline.h"

#if defined(_WIN32)
#define strdup(s)                       _strdup(s)
#endif

char* foreach_line_add(const char* src, const char* beg, const char* end)
{
    char* ret = NULL;
    char* src_dup = strdup(src);

    size_t ret_sz = 0;
    size_t beg_sz = beg != NULL ? strlen(beg): 0;
    size_t end_sz = end != NULL ? strlen(end): 0;

    char* saveptr = NULL;
    char* line;
    while ((line = strtok_f(src_dup, "\n", &saveptr)) != NULL)
    {
        size_t line_sz = strlen(line);
        size_t new_ret_sz = ret_sz + beg_sz + line_sz + end_sz + 1;
        char* new_ret = realloc(ret, new_ret_sz + 1);
        if (new_ret == NULL)
        {
            abort();
        }
        ret = new_ret;
        char* pos = ret + ret_sz;

        memcpy(pos, beg, beg_sz);
        pos += beg_sz;

        memcpy(pos, line, line_sz);
        pos += line_sz;

        memcpy(ret + ret_sz + beg_sz + line_sz, end, end_sz);
        pos += end_sz;

        *pos = '\n';
        pos++;

        *pos = '\0';
        pos++;

        ret_sz = new_ret_sz;
    }

    free(src_dup);
    return ret;
}

char* foreach_line_remove_trailing_space(const char* src)
{
    char* ret = NULL;
    size_t ret_sz = 0;

    char* src_dup = strdup(src);
    char* saveptr = NULL;

    char* line;
    while ((line = strtok_f(src_dup, "\n", &saveptr)) != NULL)
    {
        size_t line_sz = strlen(line);
        size_t new_ret_sz = ret_sz + line_sz + 1; /* don't forget \n */
        char* new_ret = realloc(ret, new_ret_sz + 1);
        if (new_ret == NULL)
        {
            abort();
        }

        ret = new_ret;
        memcpy(ret + ret_sz, line, line_sz);
        ret_sz = new_ret_sz;

        if (line_sz > 0)
        {
            while (ret[ret_sz - 2] == ' ')
            {
                ret[ret_sz - 2] = '\n';
                ret_sz--;
            }
        }

        ret[ret_sz - 1] = '\n';
        ret[ret_sz] = '\0';
    }

    free(src_dup);
    return ret;
}
