#include "cutest.h"

#if defined(_WIN32)

#include <windows.h>

static LARGE_INTEGER _test_get_file_time_offset(void)
{
    SYSTEMTIME s;
    FILETIME f;
    LARGE_INTEGER t;

    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;

    return t;
}

#else

#include <time.h>

#endif

int cutest_timestamp_get(cutest_timestamp_t* ts)
{
#if defined(_MSC_VER)

    LARGE_INTEGER           t;
    FILETIME                f;
    double                  microseconds;
    static LARGE_INTEGER    offset;
    static double           frequencyToMicroseconds;
    static int              initialized = 0;
    static BOOL             usePerformanceCounter = 0;

    if (!initialized)
    {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter)
        {
            QueryPerformanceCounter(&offset);
            frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
        }
        else
        {
            offset = _test_get_file_time_offset();
            frequencyToMicroseconds = 10.;
        }
    }
    if (usePerformanceCounter)
    {
        QueryPerformanceCounter(&t);
    }
    else
    {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double)t.QuadPart / frequencyToMicroseconds;
    t.QuadPart = (LONGLONG)microseconds;
    ts->sec = t.QuadPart / 1000000;
    ts->usec = t.QuadPart % 1000000;
    return 0;

#else
    struct timespec tmp_ts;
    if (clock_gettime(CLOCK_MONOTONIC, &tmp_ts) < 0)
    {
        return -1;
    }

    ts->sec = tmp_ts.tv_sec;
    ts->usec = tmp_ts.tv_nsec / 1000;
    return 0;
#endif
}

int cutest_timestamp_dif(const cutest_timestamp_t* t1,
    const cutest_timestamp_t* t2, cutest_timestamp_t* dif)
{
    cutest_timestamp_t tmp_dif;
    const cutest_timestamp_t* large_t = t1->sec > t2->sec ? t1 : (t1->sec < t2->sec ? t2 : (t1->usec > t2->usec ? t1 : t2));
    const cutest_timestamp_t* little_t = large_t == t1 ? t2 : t1;

    tmp_dif.sec = large_t->sec - little_t->sec;
    if (large_t->usec < little_t->usec)
    {
        tmp_dif.usec = little_t->usec - large_t->usec;
        tmp_dif.sec--;
    }
    else
    {
        tmp_dif.usec = large_t->usec - little_t->usec;
    }

    if (dif != NULL)
    {
        *dif = tmp_dif;
    }

    if (tmp_dif.sec == 0 && tmp_dif.usec == 0)
    {
        return 0;
    }
    return t1 == little_t ? -1 : 1;
}
