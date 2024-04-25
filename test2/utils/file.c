#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

int test_file_read(const char* path, test_str_t* data)
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
            return -1;
        }

        size_t nread = fread(buf, 1, sizeof(buf), f);
        test_str_append(data, buf, nread);
    }

    fclose(f);

    return 0;
}
