#include "test.h"

///////////////////////////////////////////////////////////////////////////////
// Porting
///////////////////////////////////////////////////////////////////////////////

#include <setjmp.h>

struct cutest_porting_jmpbuf
{
    jmp_buf buf;
};

static void _cutest_porting_longjmp(cutest_porting_jmpbuf_t* buf, int val)
{
    longjmp(buf->buf, val);
}

void cutest_porting_setjmp(cutest_porting_setjmp_fn execute, void* data)
{
    cutest_porting_jmpbuf_t jmpbuf;
    execute(&jmpbuf,
        _cutest_porting_longjmp,
        setjmp(jmpbuf.buf),
        data);
}
