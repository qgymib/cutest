#include "test.h"

///////////////////////////////////////////////////////////////////////////////
// Porting
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>

int cutest_porting_abort(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);

    abort();
}
