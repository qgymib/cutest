#include <stddef.h>
#include <string.h>
#include "strtok_f.h"

char* strtok_f(char* str, const char* delim, char** saveptr)
{
    char* pos;
    size_t delim_sz = strlen(delim);

    if (*saveptr == NULL)
    {
        if ((pos = strstr(str, delim)) == NULL)
        {
            return NULL;
        }

        *saveptr = pos + delim_sz;
        *pos = '\0';
        return str;
    }

    if (**saveptr == '\0')
    {
        return NULL;
    }

    if ((pos = strstr(*saveptr, delim)) == NULL)
    {
        *saveptr += strlen(*saveptr);
        return *saveptr;
    }

    char* ret = *saveptr;
    *saveptr = pos + delim_sz;
    *pos = '\0';

    return ret;
}
