#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "file.h"

#if !defined(_WIN32)
static int fopen_s(FILE** out, const char* path, const char* mode)
{
    if ((*out = fopen(path, mode)) == NULL)
    {
        return errno;
    }
    return 0;
}
#endif

static int _test_file_read_inner(const char* data, size_t size, void* arg)
{
    test_str_t* dat = arg;
    test_str_append(dat, data, size);
    return 0;
}

int test_file_read(const char* path, test_str_t* data)
{
    return test_file_reader(path, _test_file_read_inner, data);
}

int test_file_reader(const char* path,
    int (*cb)(const char* data, size_t size, void* arg), void* arg)
{
    FILE* f = NULL;
    char buf[64];

    int ret = fopen_s(&f, path, "rb");
    if (ret != 0)
    {
        return -ret;
    }

    while (!feof(f))
    {
        if (ferror(f))
        {
            ret = -1;
            break;
        }

        size_t nread = fread(buf, 1, sizeof(buf) - 1, f);
        buf[nread] = '\0';

        if ((ret = cb(buf, nread, arg)) != 0)
        {
            break;
        }
    }
    fclose(f);

    return ret;
}
