#ifndef _WIN32_WINNT
#   define _WIN32_WINNT   0x0600
#endif

#include "cutest.h"

/*
 * Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
 * https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
 */
#if defined(_MSC_VER) && _MSC_VER < 1900
#   pragma warning(disable : 4127)
#endif

///////////////////////////////////////////////////////////////////////////////
// Porting
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Define \p aliasname as a weak alias of \p name.
 * @param[in] name - The source function.
 * @param[in] aliasname - The weak alias name.
 */
#if defined(_MSC_VER)
#   ifndef weak_alias
#       define weak_alias(name, aliasname) \
            __pragma(comment(linker,"/alternatename:" #aliasname "=" #name));
#   endif
#elif defined(__GNUC__) || defined(__clang__) || defined(__ARMCOMPILER_VERSION)
#   ifndef weak_alias
#       define weak_alias(name, aliasname) \
            extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));
#   endif
#endif

#if defined(weak_alias)
#   define WEAK_ALIAS_FUNC(name, aliasname, ret, ...) \
        weak_alias(name, aliasname);
#else
#   define WEAK_ALIAS_FUNC(name, aliasname, ret, ...) \
        TEST_JOIN(WEAK_ALIAS_FUNC_, TEST_NARG(__VA_ARGS__))(name, aliasname, ret, __VA_ARGS__)
#endif

#define WEAK_ALIAS_FUNC_0(name, aliasname, ret) \
    extern ret aliasname(void) {\
        return name(); \
    }

#define WEAK_ALIAS_FUNC_1(name, aliasname, ret, targ1) \
    extern ret aliasname(targ1 p1) {\
        return name(p1); \
    }

#define WEAK_ALIAS_FUNC_2(name, aliasname, ret, targ1, targ2) \
    extern ret aliasname(targ1 p1, targ2 p2) {\
        return name(p1, p2); \
    }

#define WEAK_ALIAS_FUNC_3(name, aliasname, ret, targ1, targ2, targ3) \
    extern ret aliasname(targ1 p1, targ2 p2, targ3 p3) {\
        return name(p1, p2, p3); \
    }

#define WEAK_ALIAS_FUNC_4(name, aliasname, ret, targ1, targ2, targ3, targ4) \
    extern ret aliasname(targ1 p1, targ2 p2, targ3 p3, targ4 p4) {\
        return name(p1, p2, p3, p4); \
    }

#if defined(CUTEST_PORTING)
#   define CUTEST_PORTING_SETJMP
#   define CUTEST_PORTING_CLOCK_GETTIME
#   define CUTEST_PORTING_ABORT
#   define CUTEST_PORTING_GETTID
#   define CUTEST_PORTING_CVFPRINTF
#endif

/**
 * @brief Abort the program if assertion \p x is false.
 * @note This macro does not respect `NDEBUG`.
 */
#define CUTEST_PORTING_ASSERT(x) \
    ((x) ? (void)0 : (void)cutest_abort(\
        "%s:%d: %s: Assertion `%s' failed.\n", __FILE__, __LINE__, __FUNCTION__, #x))

/**
 * @brief Like #CUTEST_PORTING_ASSERT, but with print support.
 */
#define CUTEST_PORTING_ASSERT_P(x, fmt, ...)  \
    ((x) ? (void)0 : (void)cutest_abort(\
        "%s:%d: %s: Assertion `%s' failed: " fmt "\n", __FILE__, __LINE__, __FUNCTION__, #x, ##__VA_ARGS__))

static unsigned long s_test_rand_seed = 1;

static void cutest_porting_srand(unsigned long s)
{
    s_test_rand_seed = s;
}

static unsigned long cutest_porting_grand(void)
{
    return s_test_rand_seed;
}

static unsigned long cutest_porting_rand(unsigned long range)
{
    s_test_rand_seed = 1103515245UL * s_test_rand_seed + 12345;
    return s_test_rand_seed % range;
}

static int cutest_porting_cfprintf(FILE* stream, int color, const char* fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = cutest_porting_cvfprintf(stream, color, fmt, ap);
    va_end(ap);

    return ret;
}

static int cutest_porting_vfprintf(FILE* stream, const char* fmt, va_list ap)
{
    return cutest_porting_cvfprintf(stream, CUTEST_COLOR_DEFAULT, fmt, ap);
}

static int cutest_porting_fprintf(FILE* stream, const char* fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = cutest_porting_vfprintf(stream, fmt, ap);
    va_end(ap);

    return ret;
}

static void* cutest_porting_memcpy(void* dst, const void* src, unsigned long n)
{
    unsigned char* p_dst = dst;
    const unsigned char* p_src = src;

    unsigned long i;
    for (i = 0; i < n; i++)
    {
        p_dst[i] = p_src[i];
    }

    return dst;
}

static void* cutest_porting_memmove(void* dest, const void* src, unsigned long n)
{
    char* d = dest;
    const char* s = src;

    if (d == s)
    {
        return d;
    }
    if (s - d - n <= -2 * n)
    {
        return cutest_porting_memcpy(d, s, n);
    }

    if (d < s)
    {
        for (; n; n--) *d++ = *s++;
    }
    else
    {
        while (n) n--, d[n] = s[n];
    }

    return dest;
}

static int cutest_porting_isspace(int c)
{
    return c == ' ' || (unsigned)c - '\t' < 5;
}

static int cutest_porting_isdigit(int c)
{
    return (unsigned)c - '0' < 10;
}

static int cutest_porting_atoul(const char* src, unsigned long* dst)
{
    unsigned long ret = 0;

    while (cutest_porting_isspace(*src))
    {
        src++;
    }

    switch (*src)
    {
    case '-':
        return -1;
    case '+':
        src++;
    }

    while (cutest_porting_isdigit(*src))
    {
        ret = 10 * ret + (*src++ - '0');
    }

    *dst = ret;
    return *src == '\0' ? 0 : -1;
}

/**
 * @param[out] p    Buffer to store, must at least 20 bytes.
 * @param[in] x     Value to convert.
 */
static char* cutest_porting_ultoa(char* p, unsigned long src)
{
    char* tmp_p = p + 19;
    *--tmp_p = '\0';

    unsigned long num = 1;
    do {
        *--tmp_p = '0' + src % 10;
        src /= 10;
        num++;
    } while (src);

    cutest_porting_memmove(p, tmp_p, num);

    return p;
}

static int cutest_porting_memcmp(const void* vl, const void* vr, unsigned long n)
{
    const unsigned char* l = vl, * r = vr;
    for (; n && *l == *r; n--, l++, r++);
    return n ? *l - *r : 0;
}

static void* cutest_porting_memset(void* dest, int c, unsigned long n)
{
    unsigned char* s = dest;

    unsigned long i;
    for (i = 0; i < n; i++)
    {
        s[i] = (unsigned char)c;
    }

    return dest;
}

static unsigned long cutest_porting_strlen(const char* s)
{
    const char* a = s;
    for (; *s; s++);
    return (unsigned long)(s - a);
}

static int cutest_porting_strcmp(const char* l, const char* r)
{
    for (; *l == *r && *l; l++, r++);
    return *(unsigned char*)l - *(unsigned char*)r;
}

static int cutest_porting_strncmp(const char* _l, const char* _r, unsigned long n)
{
    const unsigned char* l = (void*)_l, * r = (void*)_r;
    if (!n--) return 0;
    for (; *l && *r && n && *l == *r; l++, r++, n--);
    return *l - *r;
}

static char* cutest_porting_strchrnul(const char* s, int c)
{
    c = (unsigned char)c;
    if (!c)
    {
        return (char*)s + cutest_porting_strlen(s);
    }

    for (; *s && *(unsigned char*)s != c; s++)
    {
    }
    return (char*)s;
}

static char* cutest_porting_strchr(const char* s, int c)
{
    char* r = cutest_porting_strchrnul(s, c);
    return *(unsigned char*)r == (unsigned char)c ? r : 0;
}

/**
 * @{
 * BEG: cutest_porting_clock_gettime()
 */
#if defined(CUTEST_PORTING_CLOCK_GETTIME)

/* Do nothing */

#elif defined(_WIN32)

#include <windows.h>

typedef struct test_timestamp_win32
{
    BOOL            usePerformanceCounter;
    LARGE_INTEGER   performanceFrequency;
    LARGE_INTEGER   offset;
    double          frequencyToMicroseconds;
} test_timestamp_win32_t;

static test_timestamp_win32_t s_timestamp_win32;

static LARGE_INTEGER _cutest_get_file_time_offset(void)
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

static BOOL _cutest_init_timestamp_win32(PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context)
{
    (void)InitOnce; (void)Parameter; (void)Context;

    s_timestamp_win32.usePerformanceCounter = QueryPerformanceFrequency(&s_timestamp_win32.performanceFrequency);

    if (s_timestamp_win32.usePerformanceCounter)
    {
        QueryPerformanceCounter(&s_timestamp_win32.offset);
        s_timestamp_win32.frequencyToMicroseconds = (double)s_timestamp_win32.performanceFrequency.QuadPart / 1000000.;
    }
    else
    {
        s_timestamp_win32.offset = _cutest_get_file_time_offset();
        s_timestamp_win32.frequencyToMicroseconds = 10.;
    }

    return TRUE;
}

void _cutest_porting_clock_gettime(cutest_porting_timespec_t* tp)
{
    /* One time initialize */
    static INIT_ONCE token = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce(&token, _cutest_init_timestamp_win32, NULL, NULL);

    /* Get PerformanceCounter */
    LARGE_INTEGER t;
    if (s_timestamp_win32.usePerformanceCounter)
    {
        QueryPerformanceCounter(&t);
    }
    else
    {
        FILETIME f;
        GetSystemTimeAsFileTime(&f);

        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    /* Shift to basis */
    t.QuadPart -= s_timestamp_win32.offset.QuadPart;

    /* Convert to microseconds */
    double microseconds = (double)t.QuadPart / s_timestamp_win32.frequencyToMicroseconds;
    t.QuadPart = (LONGLONG)microseconds;

    /* Set result */
    tp->tv_sec = (long)(t.QuadPart / 1000000);
    tp->tv_nsec = (t.QuadPart % 1000000) * 1000;
}

WEAK_ALIAS_FUNC(_cutest_porting_clock_gettime, cutest_porting_clock_gettime,
    void, cutest_porting_timespec_t*)

#elif defined(__linux__)

#include <time.h>
#include <stdlib.h>

void _cutest_porting_clock_gettime(cutest_porting_timespec_t* tp)
{
    struct timespec tmp_ts;
    if (clock_gettime(CLOCK_MONOTONIC, &tmp_ts) < 0)
    {
        abort();
    }

    tp->tv_sec = tmp_ts.tv_sec;
    tp->tv_nsec = tmp_ts.tv_nsec;
}

WEAK_ALIAS_FUNC(_cutest_porting_clock_gettime, cutest_porting_clock_gettime,
    void, cutest_porting_timespec_t*)

#endif

/**
 * END: cutest_porting_clock_gettime()
 * @}
 */

/**
 * @{
 * BEG: cutest_porting_abort()
 */

#if defined(CUTEST_PORTING_ABORT)

/* Do nothing */

#else

#include <stdlib.h>

void _cutest_porting_abort(const char* fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fflush(stderr);
    abort();
}

WEAK_ALIAS_FUNC(_cutest_porting_abort, cutest_porting_abort,
    void, va_list)

#endif

static void cutest_abort(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    cutest_porting_abort(fmt, ap);
    va_end(ap);
}

/**
 * END: cutest_porting_abort()
 * @}
 */

/**
 * @{
 * BEG: cutest_porting_gettid()
 */

#if defined(CUTEST_PORTING_GETTID)

/* Do nothing */

#elif defined(CUTEST_NO_THREADS)

void* cutest_porting_gettid(void)
{
    return NULL;
}

#elif defined(_WIN32)

#include <windows.h>

void* _cutest_porting_gettid(void)
{
    return (void*)(uintptr_t)GetCurrentThreadId();
}

WEAK_ALIAS_FUNC(_cutest_porting_gettid, cutest_porting_gettid,
    void*)

#elif defined(__linux__)

#include <pthread.h>

void* _cutest_porting_gettid(void)
{
    return (void*)pthread_self();
}

WEAK_ALIAS_FUNC(_cutest_porting_gettid, cutest_porting_gettid,
    void*)

#endif

/**
 * END: cutest_porting_gettid()
 * @}
 */

/**
 * @{
 * BEG: cutest_porting_setjmp()
 */

#if defined(CUTEST_PORTING_SETJMP)

/* Do nothing */

#else

#include <setjmp.h>

struct cutest_porting_jmpbuf
{
    jmp_buf buf;
};

static void _cutest_porting_longjmp(cutest_porting_jmpbuf_t* buf, int val)
{
    longjmp(buf->buf, val);
}

void _cutest_porting_setjmp(cutest_porting_setjmp_fn execute, void* data)
{
    cutest_porting_jmpbuf_t jmpbuf;
    execute(&jmpbuf,
        _cutest_porting_longjmp,
        setjmp(jmpbuf.buf),
        data);
}

WEAK_ALIAS_FUNC(_cutest_porting_setjmp, cutest_porting_setjmp,
    void, cutest_porting_setjmp_fn, void*)

#endif

/**
 * END: cutest_porting_setjmp()
 * @}
 */

/**
 * @{
 * BEG: cutest_porting_compare_floating_number()
 */

#if defined(CUTEST_PORTING_COMPARE_FLOATING_NUMBER)

/* Do nothing */

#else

#include <float.h>

#if defined(_MSC_VER)

typedef unsigned __int32 cutest_uint32_t;
typedef unsigned __int64 cutest_uint64_t;

#else

#include <stdint.h>

typedef uint32_t cutest_uint32_t;
typedef uint64_t cutest_uint64_t;

#endif

#define KBITCOUNT_32            (8 * sizeof(((float_point_t*)NULL)->value_))
#define KSIGNBITMASK_32         ((cutest_uint32_t)1 << (KBITCOUNT_32 - 1))
#define KFRACTIONBITCOUNT_32    (FLT_MANT_DIG - 1)
#define KEXPONENTBITCOUNT_32    (KBITCOUNT_32 - 1 - KFRACTIONBITCOUNT_32)
#define KFRACTIONBITMASK_32     ((~(cutest_uint32_t)0) >> (KEXPONENTBITCOUNT_32 + 1))
#define KEXPONENTBITMASK_32     (~(KSIGNBITMASK_32 | KFRACTIONBITMASK_32))

#define KBITCOUNT_64            (8 * sizeof(((double_point_t*)NULL)->value_))
#define KSIGNBITMASK_64         ((cutest_uint64_t)1 << (KBITCOUNT_64 - 1))
#define KFRACTIONBITCOUNT_64    (DBL_MANT_DIG - 1)
#define KEXPONENTBITCOUNT_64    (KBITCOUNT_64 - 1 - KFRACTIONBITCOUNT_64)
#define KFRACTIONBITMASK_64     ((~(cutest_uint64_t)0) >> (KEXPONENTBITCOUNT_64 + 1))
#define KEXPONENTBITMASK_64     (~(KSIGNBITMASK_64 | KFRACTIONBITMASK_64))

typedef union float_point
{
    float                   value_;
    cutest_uint32_t         bits_;
} float_point_t;

typedef union double_point
{
    double                  value_;
    cutest_uint64_t         bits_;
} double_point_t;

typedef struct cutest_float_precision
{
    unsigned long           kMaxUlps;

    struct
    {
        unsigned long       kBitCount_32;
        unsigned long       kFractionBitCount_32;
        unsigned long       kExponentBitCount_32;
        cutest_uint32_t     kSignBitMask_32;
        cutest_uint32_t     kFractionBitMask_32;
        cutest_uint32_t     kExponentBitMask_32;
    }_float;

    struct
    {
        unsigned long       kBitCount_64;
        unsigned long       kFractionBitCount_64;
        unsigned long       kExponentBitCount_64;
        cutest_uint64_t     kSignBitMask_64;
        cutest_uint64_t     kFractionBitMask_64;
        cutest_uint64_t     kExponentBitMask_64;
    }_double;
} cutest_float_precision_t;

static cutest_float_precision_t s_float_precision = {
    4,
    {
        KBITCOUNT_32,
        KFRACTIONBITCOUNT_32,
        KEXPONENTBITCOUNT_32,
        KSIGNBITMASK_32,
        KFRACTIONBITMASK_32,
        KEXPONENTBITMASK_32,
    },
    {
        KBITCOUNT_64,
        KFRACTIONBITCOUNT_64,
        KEXPONENTBITCOUNT_64,
        KSIGNBITMASK_64,
        KFRACTIONBITMASK_64,
        KEXPONENTBITMASK_64,
    },
};

static cutest_uint32_t _cutest_float_point_exponent_bits(const float_point_t* p)
{
    return s_float_precision._float.kExponentBitMask_32 & p->bits_;
}

static cutest_uint32_t _cutest_float_point_fraction_bits(const float_point_t* p)
{
    return s_float_precision._float.kFractionBitMask_32 & p->bits_;
}

static int _cutest_porting_is_float_point_nan(const float_point_t* p)
{
    return (_cutest_float_point_exponent_bits(p) == s_float_precision._float.kExponentBitMask_32)
        && (_cutest_float_point_fraction_bits(p) != 0);
}

static cutest_uint32_t _cutest_float_point_sign_and_magnitude_to_biased(cutest_uint32_t sam)
{
    if (s_float_precision._float.kSignBitMask_32 & sam)
    {
        // sam represents a negative number.
        return ~sam + 1;
    }

    // sam represents a positive number.
    return s_float_precision._float.kSignBitMask_32 | sam;
}

static cutest_uint32_t _cutest_float_point_distance_between_sign_and_magnitude_numbers(cutest_uint32_t sam1, cutest_uint32_t sam2)
{
    const cutest_uint32_t biased1 = _cutest_float_point_sign_and_magnitude_to_biased(sam1);
    const cutest_uint32_t biased2 = _cutest_float_point_sign_and_magnitude_to_biased(sam2);

    return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
}

static int _cutest_porting_floating_number_f32_eq(float v1, float v2)
{
    float_point_t f32_v1; f32_v1.value_ = v1;
    float_point_t f32_v2; f32_v2.value_ = v2;

    if (_cutest_porting_is_float_point_nan(&f32_v1) || _cutest_porting_is_float_point_nan(&f32_v2))
    {
        return 0;
    }

    return _cutest_float_point_distance_between_sign_and_magnitude_numbers(f32_v1.bits_, f32_v2.bits_)
        <= s_float_precision.kMaxUlps;
}

static int _cutest_porting_floating_number_f32(float v1, float v2)
{
    if (_cutest_porting_floating_number_f32_eq(v1, v2))
    {
        return 0;
    }
    return v1 < v2 ? -1 : 1;
}

static cutest_uint64_t _cutest_double_point_exponent_bits(const double_point_t* p)
{
    return s_float_precision._double.kExponentBitMask_64 & p->bits_;
}

static cutest_uint64_t _cutest_double_point_fraction_bits(const double_point_t* p)
{
    return s_float_precision._double.kFractionBitMask_64 & p->bits_;
}

static int _cutest_porting_double_point_is_nan(const double_point_t* p)
{
    return (_cutest_double_point_exponent_bits(p) == s_float_precision._double.kExponentBitMask_64)
        && (_cutest_double_point_fraction_bits(p) != 0);
}

static cutest_uint64_t _cutest_double_point_sign_and_magnitude_to_biased(cutest_uint64_t sam)
{
    if (s_float_precision._double.kSignBitMask_64 & sam)
    {
        // sam represents a negative number.
        return ~sam + 1;
    }

    // sam represents a positive number.
    return s_float_precision._double.kSignBitMask_64 | sam;
}

static cutest_uint64_t _cutest_double_point_distance_between_sign_and_magnitude_numbers(
    cutest_uint64_t sam1, cutest_uint64_t sam2)
{
    const cutest_uint64_t biased1 = _cutest_double_point_sign_and_magnitude_to_biased(sam1);
    const cutest_uint64_t biased2 = _cutest_double_point_sign_and_magnitude_to_biased(sam2);

    return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
}

static int _cutest_porting_floating_number_f64_eq(double v1, double v2)
{
    double_point_t f64_v1; f64_v1.value_ = v1;
    double_point_t f64_v2; f64_v2.value_ = v2;

    if (_cutest_porting_double_point_is_nan(&f64_v1) || _cutest_porting_double_point_is_nan(&f64_v2))
    {
        return 0;
    }

    return _cutest_double_point_distance_between_sign_and_magnitude_numbers(f64_v1.bits_, f64_v2.bits_)
        <= s_float_precision.kMaxUlps;
}

static int _cutest_porting_floating_number_f64(double v1, double v2)
{
    if (_cutest_porting_floating_number_f64_eq(v1, v2))
    {
        return 0;
    }
    return v1 < v2 ? -1 : 1;
}

int _cutest_porting_compare_floating_number(int type, const void* v1, const void* v2)
{
    /* Ensure size match */
    CUTEST_PORTING_ASSERT(sizeof(((double_point_t*)NULL)->bits_) == sizeof(((double_point_t*)NULL)->value_));
    CUTEST_PORTING_ASSERT(sizeof(((float_point_t*)NULL)->bits_) == sizeof(((float_point_t*)NULL)->value_));

    if (type)
    {
        return _cutest_porting_floating_number_f64(*(const double*)v1, *(const double*)v2);
    }

    return _cutest_porting_floating_number_f32(*(const float*)v1, *(const float*)v2);
}

WEAK_ALIAS_FUNC(_cutest_porting_compare_floating_number, cutest_porting_compare_floating_number,
    int, int, const void*, const void*)

#endif

/**
 * END: cutest_porting_compare_floating_number()
 * @}
 */

/**
 * BEG: cutest_porting_cvfprintf()
 * @{
 */

#if defined(CUTEST_PORTING_CVFPRINTF)

/* do nothing */

#else

#if defined(_WIN32)

#include <io.h>
#define isatty(x)                        _isatty(x)
#define fileno(x)                        _fileno(x)

static int _cutest_should_use_color(int is_tty)
{
    /**
     * On Windows the $TERM variable is usually not set, but the console there
     * does support colors.
     */
    return is_tty;
}

static int _cutest_get_bit_offset(WORD color_mask)
{
    if (color_mask == 0) return 0;

    int bitOffset = 0;
    while ((color_mask & 1) == 0)
    {
        color_mask >>= 1;
        ++bitOffset;
    }
    return bitOffset;
}

static WORD _cutest_get_color_attribute(cutest_porting_color_t color)
{
    switch (color)
    {
    case CUTEST_COLOR_RED:
        return FOREGROUND_RED;
    case CUTEST_COLOR_GREEN:
        return FOREGROUND_GREEN;
    case CUTEST_COLOR_YELLOW:
        return FOREGROUND_RED | FOREGROUND_GREEN;
    default:
        return 0;
    }
}

static WORD _cutest_get_new_color(cutest_porting_color_t color, WORD old_color_attrs)
{
    // Let's reuse the BG
    static const WORD background_mask = BACKGROUND_BLUE | BACKGROUND_GREEN |
        BACKGROUND_RED | BACKGROUND_INTENSITY;
    static const WORD foreground_mask = FOREGROUND_BLUE | FOREGROUND_GREEN |
        FOREGROUND_RED | FOREGROUND_INTENSITY;
    const WORD existing_bg = old_color_attrs & background_mask;

    WORD new_color =
        _cutest_get_color_attribute(color) | existing_bg | FOREGROUND_INTENSITY;
    const int bg_bitOffset = _cutest_get_bit_offset(background_mask);
    const int fg_bitOffset = _cutest_get_bit_offset(foreground_mask);

    if (((new_color & background_mask) >> bg_bitOffset) ==
        ((new_color & foreground_mask) >> fg_bitOffset))
    {
        new_color ^= FOREGROUND_INTENSITY;  // invert intensity
    }
    return new_color;
}

static int _cutest_porting_color_vfprintf(FILE* stream, cutest_porting_color_t color,
    const char* fmt, va_list ap)
{
    int ret;
    const HANDLE stdout_handle = (HANDLE)_get_osfhandle(fileno(stream));

    // Gets the current text color.
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
    const WORD old_color_attrs = buffer_info.wAttributes;
    const WORD new_color = _cutest_get_new_color(color, old_color_attrs);

    // We need to flush the stream buffers into the console before each
    // SetConsoleTextAttribute call lest it affect the text that is already
    // printed but has not yet reached the console.
    fflush(stream);
    SetConsoleTextAttribute(stdout_handle, new_color);

    ret = vfprintf(stream, fmt, ap);

    fflush(stream);
    // Restores the text color.
    SetConsoleTextAttribute(stdout_handle, old_color_attrs);

    return ret;
}

#elif defined(__linux__)

#include <pthread.h>
#include <unistd.h>

typedef struct color_printf_ctx
{
    int terminal_color_support;
} color_printf_ctx_t;

static color_printf_ctx_t g_color_printf = {
    0,
};

static void _cutest_initlize_color_unix(void)
{
    static const char* support_color_term_list[] = {
        "xterm",
        "xterm-color",
        "xterm-256color",
        "screen",
        "screen-256color",
        "tmux",
        "tmux-256color",
        "rxvt-unicode",
        "rxvt-unicode-256color",
        "linux",
        "cygwin",
    };

    /* On non-Windows platforms, we rely on the TERM variable. */
    const char* term = getenv("TERM");
    if (term == NULL)
    {
        return;
    }

    unsigned long i;
    for (i = 0; i < TEST_ARRAY_SIZE(support_color_term_list); i++)
    {
        if (cutest_porting_strcmp(term, support_color_term_list[i]) == 0)
        {
            g_color_printf.terminal_color_support = 1;
            break;
        }
    }
}

static int _cutest_should_use_color(int is_tty)
{
    static pthread_once_t once_control = PTHREAD_ONCE_INIT;
    pthread_once(&once_control, _cutest_initlize_color_unix);

    return is_tty && g_color_printf.terminal_color_support;
}

static const char* _cutest_get_ansi_color_code_fg(cutest_porting_color_t color)
{
    switch (color)
    {
    case CUTEST_COLOR_RED:
        return "31";
    case CUTEST_COLOR_GREEN:
        return "32";
    case CUTEST_COLOR_YELLOW:
        return "33";
    default:
        break;
    }

    return NULL;
}

static int _cutest_porting_color_vfprintf(FILE* stream, cutest_porting_color_t color,
    const char* fmt, va_list ap)
{
    int ret;
    fprintf(stream, "\033[0;%sm", _cutest_get_ansi_color_code_fg(color));
    ret = vfprintf(stream, fmt, ap);
    fprintf(stream, "\033[m");  // Resets the terminal to default.
    fflush(stream);
    return ret;
}

#endif

/**
 * @brief Print data to \p stream.
 */
int _cutest_porting_cvfprintf(FILE* stream, int color, const char* fmt, va_list ap)
{
    int ret;
    CUTEST_PORTING_ASSERT(stream != NULL);

    int stream_fd = fileno(stream);
    if (!_cutest_should_use_color(isatty(stream_fd)) || (color == CUTEST_COLOR_DEFAULT))
    {
        ret = vfprintf(stream, fmt, ap);
    }
    else
    {
        ret = _cutest_porting_color_vfprintf(stream, color, fmt, ap);
    }
    fflush(stream);

    return ret;
}

WEAK_ALIAS_FUNC(_cutest_porting_cvfprintf, cutest_porting_cvfprintf,
    int, FILE*, int, const char*, va_list)

#endif

/**
 * END: cutest_porting_cvfprintf()
 * @}
 */

///////////////////////////////////////////////////////////////////////////////
// Map
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Map initializer helper
 * @param [in] fn       Compare function
 * @param [in] arg      User defined argument
 */
#define CUTEST_MAP_INIT(fn, arg)      { NULL, { fn, arg }, 0 }

#define RB_RED              0
#define RB_BLACK            1
#define __rb_color(pc)      ((uintptr_t)(pc) & 1)
#define __rb_is_black(pc)   __rb_color(pc)
#define __rb_is_red(pc)     (!__rb_color(pc))
#define __rb_parent(pc)     ((cutest_map_node_t*)(pc & ~3))
#define rb_color(rb)        __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb)       __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb)     __rb_is_black((rb)->__rb_parent_color)
#define rb_parent(r)        ((cutest_map_node_t*)((uintptr_t)((r)->__rb_parent_color) & ~3))

/* 'empty' nodes are nodes that are known not to be inserted in an rbtree */
#define RB_EMPTY_NODE(node)  \
    ((node)->__rb_parent_color == (cutest_map_node_t*)(node))

/**
 * @brief Compare function
 * @param [in] key1     KEY1
 * @param [in] key2     KEY2
 * @param [in] arg      User defined argument
 * @return              Compare result
 */
typedef int(*cutest_map_cmp_fn)(const cutest_map_node_t* key1,
    const cutest_map_node_t* key2, void* arg);

/**
 * @brief Map handler
 */
typedef struct cunittest_map
{
    cutest_map_node_t*      rb_root;    /**< Root node */

    struct
    {
        cutest_map_cmp_fn   cmp;        /**< Compare function */
        void*               arg;        /**< User defined data for compare function */
    } cmp;

    unsigned long           size;       /**< Data size */
} cutest_map_t;

static void _etest_map_link_node(cutest_map_node_t* node, cutest_map_node_t* parent, cutest_map_node_t** rb_link)
{
    node->__rb_parent_color = parent;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
    return;
}

static void rb_set_black(cutest_map_node_t* rb)
{
    rb->__rb_parent_color =
        (cutest_map_node_t*)((uintptr_t)(rb->__rb_parent_color) | RB_BLACK);
}

static cutest_map_node_t* rb_red_parent(cutest_map_node_t* red)
{
    return red->__rb_parent_color;
}

static void rb_set_parent_color(cutest_map_node_t* rb, cutest_map_node_t* p, int color)
{
    rb->__rb_parent_color = (cutest_map_node_t*)((uintptr_t)p | color);
}

static void __rb_change_child(cutest_map_node_t* old_node, cutest_map_node_t* new_node,
    cutest_map_node_t* parent, cutest_map_t* root)
{
    if (parent)
    {
        if (parent->rb_left == old_node)
        {
            parent->rb_left = new_node;
        }
        else
        {
            parent->rb_right = new_node;
        }
    }
    else
    {
        root->rb_root = new_node;
    }
}

/*
* Helper function for rotations:
* - old's parent and color get assigned to new
* - old gets assigned new as a parent and 'color' as a color.
*/
static void __rb_rotate_set_parents(cutest_map_node_t* old, cutest_map_node_t* new_node,
    cutest_map_t* root, int color)
{
    cutest_map_node_t* parent = rb_parent(old);
    new_node->__rb_parent_color = old->__rb_parent_color;
    rb_set_parent_color(old, new_node, color);
    __rb_change_child(old, new_node, parent, root);
}

static void _etest_map_insert(cutest_map_node_t* node, cutest_map_t* root)
{
    cutest_map_node_t* parent = rb_red_parent(node), * gparent, * tmp;

    while (1) {
        /*
        * Loop invariant: node is red
        *
        * If there is a black parent, we are done.
        * Otherwise, take some corrective action as we don't
        * want a red root or two consecutive red nodes.
        */
        if (!parent) {
            rb_set_parent_color(node, NULL, RB_BLACK);
            break;
        }
        else if (rb_is_black(parent))
            break;

        gparent = rb_red_parent(parent);

        tmp = gparent->rb_right;
        if (parent != tmp) {    /* parent == gparent->rb_left */
            if (tmp && rb_is_red(tmp)) {
                /*
                * Case 1 - color flips
                *
                *       G            g
                *      / \          / \
                *     p   u  -->   P   U
                *    /            /
                *   n            n
                *
                * However, since g's parent might be red, and
                * 4) does not allow this, we need to recurse
                * at g.
                */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->rb_right;
            if (node == tmp) {
                /*
                * Case 2 - left rotate at parent
                *
                *      G             G
                *     / \           / \
                *    p   U  -->    n   U
                *     \           /
                *      n         p
                *
                * This still leaves us in violation of 4), the
                * continuation into Case 3 will fix that.
                */
                parent->rb_right = tmp = node->rb_left;
                node->rb_left = parent;
                if (tmp)
                    rb_set_parent_color(tmp, parent,
                        RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                parent = node;
                tmp = node->rb_right;
            }

            /*
            * Case 3 - right rotate at gparent
            *
            *        G           P
            *       / \         / \
            *      p   U  -->  n   g
            *     /                 \
            *    n                   U
            */
            gparent->rb_left = tmp;  /* == parent->rb_right */
            parent->rb_right = gparent;
            if (tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, root, RB_RED);
            break;
        }
        else {
            tmp = gparent->rb_left;
            if (tmp && rb_is_red(tmp)) {
                /* Case 1 - color flips */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->rb_left;
            if (node == tmp) {
                /* Case 2 - right rotate at parent */
                parent->rb_left = tmp = node->rb_right;
                node->rb_right = parent;
                if (tmp)
                    rb_set_parent_color(tmp, parent,
                        RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                parent = node;
                tmp = node->rb_left;
            }

            /* Case 3 - left rotate at gparent */
            gparent->rb_right = tmp;  /* == parent->rb_left */
            parent->rb_left = gparent;
            if (tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, root, RB_RED);
            break;
        }
    }
}

static void _etest_map_insert_color(cutest_map_node_t* node, cutest_map_t* root)
{
    _etest_map_insert(node, root);
}

/**
 * @brief Insert the node into map.
 * @warning the node must not exist in any map.
 * @param [in] handler  The pointer to the map
 * @param [in] node     The node
 * @return              0 if success, -1 otherwise
 */
static int cutest_map_insert(cutest_map_t* handler, cutest_map_node_t* node)
{
    cutest_map_node_t** new_node = &(handler->rb_root), * parent = NULL;

    /* Figure out where to put new node */
    while (*new_node)
    {
        int result = handler->cmp.cmp(node, *new_node, handler->cmp.arg);

        parent = *new_node;
        if (result < 0)
        {
            new_node = &((*new_node)->rb_left);
        }
        else if (result > 0)
        {
            new_node = &((*new_node)->rb_right);
        }
        else
        {
            return -1;
        }
    }

    handler->size++;
    _etest_map_link_node(node, parent, new_node);
    _etest_map_insert_color(node, handler);

    return 0;
}

/**
 * @brief Returns an iterator to the beginning
 * @param [in] handler  The pointer to the map
 * @return              An iterator
 */
static cutest_map_node_t* cutest_map_begin(const cutest_map_t* handler)
{
    cutest_map_node_t* n = handler->rb_root;

    if (!n)
        return NULL;
    while (n->rb_left)
        n = n->rb_left;
    return n;
}

/**
 * @brief Returns an iterator to the end
 * @param root      The pointer to the map
 * @return          An iterator
 */
static cutest_map_node_t* cutest_map_end(const cutest_map_t* handler)
{
    cutest_map_node_t* n = handler->rb_root;

    if (!n)
        return NULL;
    while (n->rb_right)
        n = n->rb_right;
    return n;
}

/**
 * @brief Get an iterator next to the given one.
 * @param [in] node     Current iterator
 * @return              Next iterator
 */
static cutest_map_node_t* cutest_map_next(const cutest_map_node_t* node)
{
    cutest_map_node_t* parent;

    if (RB_EMPTY_NODE(node))
        return NULL;

    /*
    * If we have a right-hand child, go down and then left as far
    * as we can.
    */
    if (node->rb_right) {
        node = node->rb_right;
        while (node->rb_left)
            node = node->rb_left;
        return (cutest_map_node_t*)node;
    }

    /*
    * No right-hand children. Everything down and left is smaller than us,
    * so any 'next' node must be in the general direction of our parent.
    * Go up the tree; any time the ancestor is a right-hand child of its
    * parent, keep going up. First time it's a left-hand child of its
    * parent, said parent is our 'next' node.
    */
    while ((parent = rb_parent(node)) != NULL && node == parent->rb_right)
        node = parent;

    return parent;
}

static void rb_set_parent(cutest_map_node_t* rb, cutest_map_node_t* p)
{
    rb->__rb_parent_color = (cutest_map_node_t*)(rb_color(rb) | (uintptr_t)p);
}

static cutest_map_node_t* __rb_erase_augmented(cutest_map_node_t* node, cutest_map_t* root)
{
    cutest_map_node_t* child = node->rb_right, *tmp = node->rb_left;
    cutest_map_node_t* parent, * rebalance;
    uintptr_t pc;

    if (!tmp) {
        /*
        * Case 1: node to erase has no more than 1 child (easy!)
        *
        * Note that if there is one child it must be red due to 5)
        * and node must be black due to 4). We adjust colors locally
        * so as to bypass __rb_erase_color() later on.
        */
        pc = (uintptr_t)(node->__rb_parent_color);
        parent = __rb_parent(pc);
        __rb_change_child(node, child, parent, root);
        if (child) {
            child->__rb_parent_color = (cutest_map_node_t*)pc;
            rebalance = NULL;
        }
        else
            rebalance = __rb_is_black(pc) ? parent : NULL;
        tmp = parent;
    }
    else if (!child) {
        /* Still case 1, but this time the child is node->rb_left */
        pc = (uintptr_t)(node->__rb_parent_color);
        tmp->__rb_parent_color = (cutest_map_node_t*)pc;
        parent = __rb_parent(pc);
        __rb_change_child(node, tmp, parent, root);
        rebalance = NULL;
        tmp = parent;
    }
    else {
        cutest_map_node_t* successor = child, * child2;
        tmp = child->rb_left;
        if (!tmp) {
            /*
            * Case 2: node's successor is its right child
            *
            *    (n)          (s)
            *    / \          / \
            *  (x) (s)  ->  (x) (c)
            *        \
            *        (c)
            */
            parent = successor;
            child2 = successor->rb_right;
        }
        else {
            /*
            * Case 3: node's successor is leftmost under
            * node's right child subtree
            *
            *    (n)          (s)
            *    / \          / \
            *  (x) (y)  ->  (x) (y)
            *      /            /
            *    (p)          (p)
            *    /            /
            *  (s)          (c)
            *    \
            *    (c)
            */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->rb_left;
            } while (tmp);
            parent->rb_left = child2 = successor->rb_right;
            successor->rb_right = child;
            rb_set_parent(child, successor);
        }

        successor->rb_left = tmp = node->rb_left;
        rb_set_parent(tmp, successor);

        pc = (uintptr_t)(node->__rb_parent_color);
        tmp = __rb_parent(pc);
        __rb_change_child(node, successor, tmp, root);
        if (child2) {
            successor->__rb_parent_color = (cutest_map_node_t*)pc;
            rb_set_parent_color(child2, parent, RB_BLACK);
            rebalance = NULL;
        }
        else {
            uintptr_t pc2 = (uintptr_t)(successor->__rb_parent_color);
            successor->__rb_parent_color = (cutest_map_node_t*)pc;
            rebalance = __rb_is_black(pc2) ? parent : NULL;
        }
        tmp = successor;
    }

    return rebalance;
}

static void ____rb_erase_color(cutest_map_node_t* parent, cutest_map_t* root)
{
    cutest_map_node_t* node = NULL, * sibling, * tmp1, * tmp2;

    for (;;) {
        /*
        * Loop invariants:
        * - node is black (or NULL on first iteration)
        * - node is not the root (parent is not NULL)
        * - All leaf paths going through parent and node have a
        *   black node count that is 1 lower than other leaf paths.
        */
        sibling = parent->rb_right;
        if (node != sibling) {  /* node == parent->rb_left */
            if (rb_is_red(sibling)) {
                /*
                * Case 1 - left rotate at parent
                *
                *     P               S
                *    / \             / \
                *   N   s    -->    p   Sr
                *      / \         / \
                *     Sl  Sr      N   Sl
                */
                parent->rb_right = tmp1 = sibling->rb_left;
                sibling->rb_left = parent;
                rb_set_parent_color(tmp1, parent, RB_BLACK);
                __rb_rotate_set_parents(parent, sibling, root,
                    RB_RED);
                sibling = tmp1;
            }
            tmp1 = sibling->rb_right;
            if (!tmp1 || rb_is_black(tmp1)) {
                tmp2 = sibling->rb_left;
                if (!tmp2 || rb_is_black(tmp2)) {
                    /*
                    * Case 2 - sibling color flip
                    * (p could be either color here)
                    *
                    *    (p)           (p)
                    *    / \           / \
                    *   N   S    -->  N   s
                    *      / \           / \
                    *     Sl  Sr        Sl  Sr
                    *
                    * This leaves us violating 5) which
                    * can be fixed by flipping p to black
                    * if it was red, or by recursing at p.
                    * p is red when coming from Case 1.
                    */
                    rb_set_parent_color(sibling, parent,
                        RB_RED);
                    if (rb_is_red(parent))
                        rb_set_black(parent);
                    else {
                        node = parent;
                        parent = rb_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /*
                * Case 3 - right rotate at sibling
                * (p could be either color here)
                *
                *   (p)           (p)
                *   / \           / \
                *  N   S    -->  N   Sl
                *     / \             \
                *    sl  Sr            s
                *                       \
                *                        Sr
                */
                sibling->rb_left = tmp1 = tmp2->rb_right;
                tmp2->rb_right = sibling;
                parent->rb_right = tmp2;
                if (tmp1)
                    rb_set_parent_color(tmp1, sibling,
                        RB_BLACK);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /*
            * Case 4 - left rotate at parent + color flips
            * (p and sl could be either color here.
            *  After rotation, p becomes black, s acquires
            *  p's color, and sl keeps its color)
            *
            *      (p)             (s)
            *      / \             / \
            *     N   S     -->   P   Sr
            *        / \         / \
            *      (sl) sr      N  (sl)
            */
            parent->rb_right = tmp2 = sibling->rb_left;
            sibling->rb_left = parent;
            rb_set_parent_color(tmp1, sibling, RB_BLACK);
            if (tmp2)
                rb_set_parent(tmp2, parent);
            __rb_rotate_set_parents(parent, sibling, root,
                RB_BLACK);
            break;
        }
        else {
            sibling = parent->rb_left;
            if (rb_is_red(sibling)) {
                /* Case 1 - right rotate at parent */
                parent->rb_left = tmp1 = sibling->rb_right;
                sibling->rb_right = parent;
                rb_set_parent_color(tmp1, parent, RB_BLACK);
                __rb_rotate_set_parents(parent, sibling, root,
                    RB_RED);
                sibling = tmp1;
            }
            tmp1 = sibling->rb_left;
            if (!tmp1 || rb_is_black(tmp1)) {
                tmp2 = sibling->rb_right;
                if (!tmp2 || rb_is_black(tmp2)) {
                    /* Case 2 - sibling color flip */
                    rb_set_parent_color(sibling, parent,
                        RB_RED);
                    if (rb_is_red(parent))
                        rb_set_black(parent);
                    else {
                        node = parent;
                        parent = rb_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /* Case 3 - right rotate at sibling */
                sibling->rb_right = tmp1 = tmp2->rb_left;
                tmp2->rb_left = sibling;
                parent->rb_left = tmp2;
                if (tmp1)
                    rb_set_parent_color(tmp1, sibling,
                        RB_BLACK);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /* Case 4 - left rotate at parent + color flips */
            parent->rb_left = tmp2 = sibling->rb_right;
            sibling->rb_right = parent;
            rb_set_parent_color(tmp1, sibling, RB_BLACK);
            if (tmp2)
                rb_set_parent(tmp2, parent);
            __rb_rotate_set_parents(parent, sibling, root,
                RB_BLACK);
            break;
        }
    }
}

static void ev_map_low_erase(cutest_map_t* root, cutest_map_node_t* node)
{
    cutest_map_node_t* rebalance;
    rebalance = __rb_erase_augmented(node, root);
    if (rebalance)
        ____rb_erase_color(rebalance, root);
}

static void cutest_map_erase(cutest_map_t* handler, cutest_map_node_t* node)
{
    handler->size--;
    ev_map_low_erase(handler, node);
}

static cutest_map_node_t* cutest_map_find(const cutest_map_t* handler, const cutest_map_node_t* key)
{
    cutest_map_node_t* node = handler->rb_root;

    while (node)
    {
        int result = handler->cmp.cmp(key, node, handler->cmp.arg);

        if (result < 0)
        {
            node = node->rb_left;
        }
        else if (result > 0)
        {
            node = node->rb_right;
        }
        else
        {
            return node;
        }
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// C String
///////////////////////////////////////////////////////////////////////////////

typedef struct test_str
{
    char*                       ptr;            /**< String address. */
    unsigned long               len;            /**< String length, not including NULL terminator. */
} test_str_t;

/**
 * @brief Construct a C string.
 * @param[in] str   Constanst C string.
 * @return          C string.
 */
static test_str_t _cutest_str(const char* str)
{
    test_str_t tmp;
    tmp.ptr = (char*)str;
    tmp.len = cutest_porting_strlen(str);
    return tmp;
}

/**
 * @brief Split \p str into \p k and \p v by needle \p s, start at \p offset.
 * @note When failure, \p k and \p v remain untouched.
 * @return 0 if success, otherwise failure.
 */
static int _cutest_str_split(const test_str_t* str, test_str_t* k,
    test_str_t* v, const char* s, unsigned long offset)
{
    unsigned long i;
    unsigned long s_len = cutest_porting_strlen(s);
    if (str->len < s_len)
    {
        goto error;
    }

    for (i = offset; i < str->len - s_len + 1; i++)
    {
        if (cutest_porting_memcmp(str->ptr + i, s, s_len) == 0)
        {
            if (k != NULL)
            {
                k->ptr = str->ptr;
                k->len = i;
            }

            if (v != NULL)
            {
                v->ptr = str->ptr + i + s_len;
                v->len = str->len - i - s_len;
            }

            return 0;
        }
    }

error:
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
// Timestamp
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Compare timestamp
 * @param [in] t1       timestamp t1
 * @param [in] t2       timestamp t2
 * @param [in] dif      diff
 * @return              -1 if t1 < t2; 1 if t1 > t2; 0 if t1 == t2
 */
static int cutest_timestamp_dif(const cutest_porting_timespec_t* t1,
    const cutest_porting_timespec_t* t2, cutest_porting_timespec_t* dif)
{
    cutest_porting_timespec_t tmp_dif;
    const cutest_porting_timespec_t* large_t = (t1->tv_sec > t2->tv_sec) ?
        t1 :
        (t1->tv_sec < t2->tv_sec ?
            t2 :
            (t1->tv_nsec > t2->tv_nsec ? t1 : t2));
    const cutest_porting_timespec_t* little_t = large_t == t1 ? t2 : t1;

    tmp_dif.tv_sec = large_t->tv_sec - little_t->tv_sec;
    if (large_t->tv_nsec < little_t->tv_nsec)
    {
        tmp_dif.tv_nsec = little_t->tv_nsec - large_t->tv_nsec;
        tmp_dif.tv_sec--;
    }
    else
    {
        tmp_dif.tv_nsec = large_t->tv_nsec - little_t->tv_nsec;
    }

    if (dif != NULL)
    {
        *dif = tmp_dif;
    }

    if (tmp_dif.tv_sec == 0 && tmp_dif.tv_nsec == 0)
    {
        return 0;
    }
    return t1 == little_t ? -1 : 1;
}

/************************************************************************/
/* test                                                                 */
/************************************************************************/

#define COLOR_GREEN(str)                    "@G" str "@D"
#define COLOR_RED(str)                      "@R" str "@D"
#define COLOR_YELLO(str)                    "@Y" str "@D"

#define MASK_FAILURE                        (0x01 << 0x00)
#define MASK_SKIPPED                        (0x01 << 0x01)
#define SET_MASK(val, mask)                 do { (val) |= (mask); } while (0)
#define HAS_MASK(val, mask)                 ((val) & (mask))

#define MAX_RAND                            99999

/**
 * @brief microseconds in one second
 */
#define USEC_IN_SEC                         (1 * 1000 * 1000)

#define CONTAINER_OF(ptr, TYPE, member) \
    ((TYPE*)((char*)(ptr) - (char*)&((TYPE*)0)->member))

/**
 * @brief Create compare context for \p TYPE.
 * @param[in] TYPE  Data type.
 */
#define TEST_GENERATE_NATIVE_COMPARE(NAME, TYPE)  \
    TEST_GENERATE_TEMPLATE_COMPARE(NAME, TYPE, _cutest_smart_print_int)

#define TEST_GENERATE_TEMPLATE_COMPARE(NAME, TYPE, PRINT)   \
    static int _test_cmp_##NAME(TYPE* addr1, TYPE* addr2) {\
        TYPE v1 = *addr1;\
        TYPE v2 = *addr2;\
        if (v1 == v2) {\
            return 0;\
        }\
        return v1 < v2 ? -1 : 1;\
    }\
    static int _test_print_##NAME(FILE* file, TYPE* addr) {\
        int is_signed = ((TYPE)(-1)) < ((TYPE)1);\
        return PRINT(file, addr, sizeof(TYPE), is_signed);\
    }\
    static cutest_type_info_t NAME = {\
        { NULL, NULL, NULL }, #TYPE,\
        (cutest_custom_type_cmp_fn)_test_cmp_##NAME,\
        (cutest_custom_type_dump_fn)_test_print_##NAME,\
    }

typedef struct test_case_info
{
    char                        fmt_name[256];  /**< Formatted name. */
    unsigned long               fmt_name_sz;    /**< The length of formatted name, not include NULL termainator. */

    cutest_case_t*              test_case;      /**< Test case. */

    cutest_porting_timespec_t   tv_case_beg;    /**< Start time. */
    cutest_porting_timespec_t   tv_case_end;    /**< End time. */
} test_case_info_t;

typedef struct fixture_run_helper
{
    test_case_info_t*           info;
    int                         ret;
} fixture_run_helper_t;

typedef struct test_case_helper
{
    test_case_info_t*           info;
    int                         ret;
} test_case_helper_t;

typedef struct test_run_parameterized_helper
{
    test_case_info_t*           info;
} test_run_parameterized_helper_t;

typedef struct test_ctx
{
    cutest_map_t                    case_table;                     /**< Cases in map */
    cutest_map_t                    type_table;                     /**< Type table. */

    struct
    {
        void*                       tid;                            /**< Thread ID */
        cutest_case_t*              cur_node;                       /**< Current running test case node. */
    } runtime;

    struct
    {
        struct
        {
            unsigned                total;                          /**< The number of total running cases */
            unsigned                disabled;                       /**< The number of disabled cases */
            unsigned                success;                        /**< The number of success cases */
            unsigned                skipped;                        /**< The number of skipped cases */
            unsigned                failed;                         /**< The number of failed cases */
        } result;

        struct
        {
            unsigned long           repeat;                         /**< How many times need to repeat */
            unsigned long           repeated;                       /**< How many times already repeated */
        } repeat;
    } counter;

    struct
    {
        test_str_t                  pattern;                         /**< `--test_filter` */
    } filter;

    struct
    {
        unsigned                    break_on_failure : 1;           /**< DebugBreak when failure */
        unsigned                    no_print_time : 1;              /**< Whether to print execution cost time */
        unsigned                    also_run_disabled_tests : 1;    /**< Also run disabled tests */
        unsigned                    shuffle : 1;                    /**< Randomize running cases */
    } mask;

    struct
    {
        cutest_porting_jmpbuf_t*    addr;                           /**< Jump address. */
        cutest_porting_longjmp_fn   func;                           /**< Long jump function. */
    } jmp;

    FILE*                           out;
    const cutest_hook_t*            hook;
} test_ctx_t;

static int _cutest_on_cmp_case(const cutest_map_node_t* key1, const cutest_map_node_t* key2, void* arg)
{
    (void)arg;
    int ret;

    cutest_case_t* t1 = CONTAINER_OF(key1, cutest_case_t, node);
    cutest_case_t* t2 = CONTAINER_OF(key2, cutest_case_t, node);

    /* randkey always go first. */
    if (t1->data.randkey < t2->data.randkey)
    {
        return -1;
    }
    if (t1->data.randkey > t2->data.randkey)
    {
        return 1;
    }

    if ((ret = cutest_porting_strcmp(t1->info.fixture_name, t2->info.fixture_name)) != 0)
    {
        return ret;
    }

    if ((ret = cutest_porting_strcmp(t1->info.case_name, t2->info.case_name)) != 0)
    {
        return ret;
    }

    if (t1->parameterized.param_idx == t2->parameterized.param_idx)
    {
        return 0;
    }
    return t1->parameterized.param_idx < t2->parameterized.param_idx ? -1 : 1;
}

static int _cutest_on_cmp_type(const cutest_map_node_t* key1, const cutest_map_node_t* key2, void* arg)
{
    (void)arg;
    cutest_type_info_t* t1 = CONTAINER_OF(key1, cutest_type_info_t, node);
    cutest_type_info_t* t2 = CONTAINER_OF(key2, cutest_type_info_t, node);

    return cutest_porting_strcmp(t1->type_name, t2->type_name);
}

static test_ctx_t g_test_ctx = {
    CUTEST_MAP_INIT(_cutest_on_cmp_case, NULL),                         /* .case_table */
    CUTEST_MAP_INIT(_cutest_on_cmp_type, NULL),                         /* .type_table */
    { NULL, NULL },                                                     /* .runtime */
    { { 0, 0, 0, 0, 0 }, { 0, 0 } },                                    /* .counter */
    { { NULL, 0 } },                                                    /* .filter */
    { 0, 0, 0, 0 },                                                     /* .mask */
    { NULL, NULL },                                                     /* .jmp */
    NULL,                                                               /* .out */
    NULL,                                                               /* .hook */
};

static const char* s_test_help_encoded =
"This program contains tests written using cutest. You can use the\n"
"following command line flags to control its behavior:\n"
"\n"
"Test Selection:\n"
"  " COLOR_GREEN("--test_list_tests") "\n"
"      List the names of all tests instead of running them. The name of\n"
"      TEST(Foo, Bar) is \"Foo.Bar\".\n"
"  " COLOR_GREEN("--test_list_types") "\n"
"      List the names of all support types.\n"
"  " COLOR_GREEN("--test_filter=") COLOR_YELLO("POSTIVE_PATTERNS[") COLOR_GREEN("-") COLOR_YELLO("NEGATIVE_PATTERNS]") "\n"
"      Run only the tests whose name matches one of the positive patterns but\n"
"      none of the negative patterns. '?' matches any single character; '*'\n"
"      matches any substring; ':' separates two patterns.\n"
"  " COLOR_GREEN("--test_also_run_disabled_tests") "\n"
"      Run all disabled tests too.\n"
"\n"
"Test Execution:\n"
"  " COLOR_GREEN("--test_repeat=") COLOR_YELLO("[COUNT]") "\n"
"      Run the tests repeatedly; use a negative count to repeat forever.\n"
"  " COLOR_GREEN("--test_shuffle") "\n"
"      Randomize tests' orders on every iteration.\n"
"  " COLOR_GREEN("--test_random_seed=") COLOR_YELLO("[NUMBER]") "\n"
"      Random number seed to use for shuffling test orders (between 0 and\n"
"      " TEST_STRINGIFY(MAX_RAND) ". By default a seed based on the current time is used for shuffle).\n"
"\n"
"Test Output:\n"
"  " COLOR_GREEN("--test_print_time=") COLOR_YELLO("(") COLOR_GREEN("0") COLOR_YELLO("|") COLOR_GREEN("1") COLOR_YELLO(")") "\n"
"      Don't print the elapsed time of each test.\n"
"\n"
"Assertion Behavior:\n"
"  " COLOR_GREEN("--test_break_on_failure") "\n"
"      Turn assertion failures into debugger break-points.\n"
;

/**
 * @brief Check if `str` match `pat`
 * @return bool
 */
static int _cutest_pattern_match(const char* pat, unsigned long pat_sz,
    const char* str, unsigned long str_sz)
{
    const char* name = str;
    const char* const name_begin = name;
    const char* const name_end = name + str_sz;

    const char* pattern = pat;
    const char* pattern_end = pat + pat_sz;
    const char* pattern_next = pattern;
    const char* name_next = name;

    while (pattern < pattern_end || name < name_end)
    {
        if (pattern < pattern_end)
        {
            switch (*pattern)
            {
            default:  // Match an ordinary character.
                if (name < name_end && *name == *pattern)
                {
                    ++pattern;
                    ++name;
                    continue;
                }
                break;
            case '?':  // Match any single character.
                if (name < name_end)
                {
                    ++pattern;
                    ++name;
                    continue;
                }
                break;
            case '*':
                // Match zero or more characters. Start by skipping over the wildcard
                // and matching zero characters from name. If that fails, restart and
                // match one more character than the last attempt.
                pattern_next = pattern;
                name_next = name + 1;
                ++pattern;
                continue;
            }
        }
        // Failed to match a character. Restart if possible.
        if (name_begin < name_next && name_next <= name_end)
        {
            pattern = pattern_next;
            name = name_next;
            continue;
        }
        return 0;
    }
    return 1;
}

/**
 * @return true if this test will run, false if not.
 */
static int _cutest_check_pattern(const char* str, unsigned long str_sz)
{
    test_str_t k, v = g_test_ctx.filter.pattern;

    /* If no pattern, run this test. */
    if (v.ptr == NULL)
    {
        return 1;
    }

    unsigned long cnt_positive_patterns = 0;
    int flag_match_positive_pattern = 0;

    /* Split patterns by `:` */
    while (_cutest_str_split(&v, &k, &v, ":", 0) == 0)
    {
        /* If it is a positive pattern. */
        if (k.ptr[0] != '-')
        {
            cnt_positive_patterns++;

            /* Record if it match any of positive pattern */
            if (_cutest_pattern_match(k.ptr, k.len, str, str_sz))
            {
                flag_match_positive_pattern = 1;
            }
        }
        else if (_cutest_pattern_match(k.ptr + 1, k.len - 1, str, str_sz))
        {/* If match negative pattern, don't run. */
            return 0;
        }
    }
    if (v.ptr[0] != '-')
    {
        cnt_positive_patterns++;
        if (_cutest_pattern_match(v.ptr, v.len, str, str_sz))
        {
            flag_match_positive_pattern = 1;
        }
    }
    else if (_cutest_pattern_match(v.ptr + 1, v.len - 1, str, str_sz))
    {
        return 0;
    }

    return cnt_positive_patterns ? flag_match_positive_pattern : 1;
}

/**
 * @return bool
 */
static int _cutest_check_disable(const char* name)
{
    return !g_test_ctx.mask.also_run_disabled_tests &&
        (cutest_porting_strncmp("DISABLED_", name, 9) == 0);
}

static int _cutest_print_encoded(const char* str)
{
    int ret = 0;
    cutest_porting_color_t color = CUTEST_COLOR_DEFAULT;

    for (;;)
    {
        const char* p = cutest_porting_strchr(str, '@');
        if (p == NULL)
        {
            ret += cutest_porting_cfprintf(g_test_ctx.out, color, "%s", str);
            return ret;
        }

        ret += cutest_porting_cfprintf(g_test_ctx.out, color, "%.*s",
            (int)(p - str), str);

        const char ch = p[1];
        str = p + 2;

        switch (ch)
        {
        case '@':
            ret += cutest_porting_cfprintf(g_test_ctx.out, color, "@");
            break;
        case 'D':
            color = 0;
            break;
        case 'R':
            color = CUTEST_COLOR_RED;
            break;
        case 'G':
            color = CUTEST_COLOR_GREEN;
            break;
        case 'Y':
            color = CUTEST_COLOR_YELLOW;
            break;
        default:
            --str;
            break;
        }
    }
}

static void _cutest_hook_before_fixture_setup(cutest_case_t* test_case)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_setup == NULL)
    {
        return;
    }
    g_test_ctx.hook->before_setup(test_case->info.fixture_name);
}

static void _cutest_hook_after_fixture_setup(cutest_case_t* test_case, int ret)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_setup == NULL)
    {
        return;
    }
    g_test_ctx.hook->after_setup(test_case->info.fixture_name, ret);
}

static void _cutest_run_case_set_jmp(cutest_porting_jmpbuf_t* buf,
                                     cutest_porting_longjmp_fn fn_longjmp)
{
    g_test_ctx.jmp.addr = buf;
    g_test_ctx.jmp.func = fn_longjmp;
}

static void _cutest_fixture_run_setup_jmp(cutest_porting_jmpbuf_t* buf,
    cutest_porting_longjmp_fn fn_longjmp, int val, void* data)
{
    fixture_run_helper_t* helper = data;
    test_case_info_t* info = helper->info;

    _cutest_run_case_set_jmp(buf, fn_longjmp);

    if (val != 0)
    {
        SET_MASK(info->test_case->data.mask, val);
        goto after_setup;
    }

    _cutest_hook_before_fixture_setup(info->test_case);
    info->test_case->stage.setup();

after_setup:
    _cutest_hook_after_fixture_setup(info->test_case, val);
    helper->ret = val;
}

/**
 * @brief Run setup stage.
 * @param[in] test_case The test case to run.
 * @return              0 if success, otherwise failure.
 */
static int _cutest_fixture_run_setup(test_case_info_t* info)
{
    if (info->test_case->stage.setup == NULL)
    {
        return 0;
    }

    fixture_run_helper_t helper = { info, 0 };
    cutest_porting_setjmp(_cutest_fixture_run_setup_jmp, &helper);
    return helper.ret;
}

static void _cutest_hook_before_test(test_case_info_t* info)
{
    cutest_case_t* test_case = info->test_case;
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_test == NULL)
    {
        return;
    }

    unsigned long fixture_sz = cutest_porting_strlen(test_case->info.fixture_name);
    g_test_ctx.hook->before_test(test_case->info.fixture_name, info->fmt_name + fixture_sz);
}

static void _cutest_hook_after_test(test_case_info_t* info, int ret)
{
    cutest_case_t* test_case = info->test_case;
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_test == NULL)
    {
        return;
    }

    unsigned long fixture_sz = cutest_porting_strlen(test_case->info.fixture_name);
    g_test_ctx.hook->after_test(test_case->info.fixture_name, info->fmt_name + fixture_sz, ret);
}

static void _cutest_hook_before_teardown(cutest_case_t* test_case)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_teardown == NULL)
    {
        return;
    }

    g_test_ctx.hook->before_teardown(test_case->info.fixture_name);
}

static void _cutest_hook_after_teardown(cutest_case_t* test_case, int ret)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_teardown == NULL)
    {
        return;
    }
    g_test_ctx.hook->after_teardown(test_case->info.fixture_name, ret);
}

static void _cutest_hook_before_all_test(int argc, char* argv[])
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_all_test == NULL)
    {
        return;
    }
    g_test_ctx.hook->before_all_test(argc, argv);
}

static void _cutest_hook_after_all_test(void)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_all_test == NULL)
    {
        return;
    }
    g_test_ctx.hook->after_all_test();
}

static void _cutest_fixture_run_teardown_jmp(cutest_porting_jmpbuf_t* buf,
    cutest_porting_longjmp_fn fn_longjmp, int val, void* data)
{
    test_case_info_t* info = data;

    _cutest_run_case_set_jmp(buf, fn_longjmp);

    if (val != 0)
    {
        SET_MASK(info->test_case->data.mask, val);
        goto after_teardown;
    }

    _cutest_hook_before_teardown(info->test_case);
    info->test_case->stage.teardown();

after_teardown:
    _cutest_hook_after_teardown(info->test_case, val);
}

static void _cutest_fixture_run_teardown(test_case_info_t* info)
{
    if (info->test_case->stage.teardown == NULL)
    {
        return;
    }

    cutest_porting_setjmp(_cutest_fixture_run_teardown_jmp, info);
}

static void _cutest_finishlize(test_case_info_t* info)
{
    cutest_porting_clock_gettime(&info->tv_case_end);

    cutest_porting_timespec_t tv_diff;
    cutest_timestamp_dif(&info->tv_case_beg, &info->tv_case_end, &tv_diff);

    if (HAS_MASK(info->test_case->data.mask, MASK_FAILURE))
    {
        g_test_ctx.counter.result.failed++;
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_RED, "[  FAILED  ]");
    }
    else if (HAS_MASK(info->test_case->data.mask, MASK_SKIPPED))
    {
        g_test_ctx.counter.result.skipped++;
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_YELLOW, "[   SKIP   ]");
    }
    else
    {
        g_test_ctx.counter.result.success++;
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_GREEN, "[       OK ]");
    }

    cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, " %s", info->fmt_name);
    if (!g_test_ctx.mask.no_print_time)
    {
        unsigned long take_time = (unsigned long)(tv_diff.tv_sec * 1000 + tv_diff.tv_nsec / 1000000);
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, " (%lu ms)", take_time);
    }
    cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, "\n");
}

static void _cutest_run_case_normal_body_jmp(cutest_porting_jmpbuf_t* buf,
    cutest_porting_longjmp_fn fn_longjmp, int val, void* data)
{
    test_case_helper_t* helper = data;
    test_case_info_t* info = helper->info;

    _cutest_run_case_set_jmp(buf, fn_longjmp);

    if (val != 0)
    {
        SET_MASK(info->test_case->data.mask, val);
        goto after_body;
    }

    _cutest_hook_before_test(info);
    info->test_case->stage.body(NULL, 0);

after_body:
    _cutest_hook_after_test(info, val);
    helper->ret = val;
}

static int _cutest_run_case_normal_body(test_case_info_t* info)
{
    test_case_helper_t helper = { info, 0 };
    cutest_porting_setjmp(_cutest_run_case_normal_body_jmp, &helper);
    return helper.ret;
}

/**
 * @return  1 if need to process, 0 to stop.
 */
static int _cutest_run_prepare(test_case_info_t* info)
{
    /* Check if need to run this test case */
    if (!_cutest_check_pattern(info->fmt_name, info->fmt_name_sz))
    {
        return 1;
    }
    g_test_ctx.counter.result.total++;

    /* check if this test is disabled */
    if (_cutest_check_disable(info->test_case->info.case_name))
    {
        g_test_ctx.counter.result.disabled++;
        return 1;
    }

    cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_GREEN, "[ RUN      ]");
    cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, " %s\n", info->fmt_name);

    /* record start time */
    cutest_porting_clock_gettime(&info->tv_case_beg);
    return 0;
}

static unsigned long _cutest_get_test_fmt_name_normal(char* buf, unsigned long len, cutest_case_t* test_case)
{
    unsigned long fixture_len = cutest_porting_strlen(test_case->info.fixture_name);
    unsigned long case_name_len = cutest_porting_strlen(test_case->info.case_name);

    /* Write fixture */
    if (len <= fixture_len + 1)
    {
        cutest_porting_memcpy(buf, test_case->info.fixture_name, len - 1);
        goto finish;
    }
    cutest_porting_memcpy(buf, test_case->info.fixture_name, fixture_len);

    /*  Write dot */
    buf[fixture_len] = '.';
    if (len == fixture_len + 2)
    {
        goto finish;
    }

    /* Write case_name */
    unsigned long left_size = len - fixture_len - 1;
    unsigned long copy_size = left_size < case_name_len ? left_size : case_name_len;
    cutest_porting_memcpy(buf + fixture_len + 1, test_case->info.case_name, copy_size);
    buf[fixture_len + 1 + case_name_len] = '\0';

finish:
    buf[len - 1] = '\0';
    return fixture_len + 1 + case_name_len;
}

static void _cutest_run_case_normal(cutest_case_t* test_case)
{
    test_case_info_t info;
    unsigned long ret = _cutest_get_test_fmt_name_normal(info.fmt_name, sizeof(info.fmt_name), test_case);
    if (ret >= sizeof(info.fmt_name))
    {
        cutest_abort("");
        return;
    }
    info.fmt_name_sz = ret;
    info.test_case = test_case;

    if (_cutest_run_prepare(&info) != 0)
    {
        return;
    }

    /* setup */
    if (_cutest_fixture_run_setup(&info) != 0)
    {
        goto cleanup;
    }

    _cutest_run_case_normal_body(&info);
    _cutest_fixture_run_teardown(&info);

cleanup:
    _cutest_finishlize(&info);
}

static void _cutest_run_case_parameterized_body_jmp(cutest_porting_jmpbuf_t* buf,
    cutest_porting_longjmp_fn fn_longjmp, int val, void* data)
{
    test_run_parameterized_helper_t* helper = data;
    test_case_info_t* info = helper->info;
    cutest_case_t* test_case = info->test_case;

    _cutest_run_case_set_jmp(buf, fn_longjmp);

    if (val != 0)
    {
        SET_MASK(info->test_case->data.mask, val);
        goto after_body;
    }

    info->test_case->stage.body(test_case->parameterized.param_data, test_case->parameterized.param_idx);

after_body:
    _cutest_hook_after_test(info, val);
}

static void _cutest_run_case_parameterized_body(test_case_info_t* info)
{
    _cutest_hook_before_test(info);

    test_run_parameterized_helper_t helper = { info };
    cutest_porting_setjmp(_cutest_run_case_parameterized_body_jmp, &helper);
}

static void _cutest_run_case_parameterized_idx(test_case_info_t* info)
{
    if (_cutest_run_prepare(info) != 0)
    {
        return;
    }

    /* setup */
    if (_cutest_fixture_run_setup(info) != 0)
    {
        goto cleanup;
    }

    _cutest_run_case_parameterized_body(info);
    _cutest_fixture_run_teardown(info);

cleanup:
    _cutest_finishlize(info);
}

static unsigned long _cutest_get_test_fmt_name_parameter(char* buf, unsigned long len, cutest_case_t* test_case)
{
    char num_buf[32];

    cutest_porting_ultoa(num_buf, test_case->parameterized.param_idx);
    unsigned long num_len = cutest_porting_strlen(num_buf);

    unsigned long ret = _cutest_get_test_fmt_name_normal(buf, len, test_case);
    if (ret >= len - num_len - 1)
    {
        goto finish;
    }

    buf[ret] = '/';
    cutest_porting_memcpy(buf + ret + 1, num_buf, num_len + 1);

finish:
    return ret + 1 + num_len;
}

static void _cutest_run_case_parameterized(cutest_case_t* test_case)
{
    test_case_info_t info;
    info.test_case = test_case;

    unsigned long ret = _cutest_get_test_fmt_name_parameter(info.fmt_name, sizeof(info.fmt_name), test_case);
    if (ret >= sizeof(info.fmt_name))
    {
        cutest_abort("name too long.\n");
        return;
    }
    info.fmt_name_sz = ret;

    _cutest_run_case_parameterized_idx(&info);
}

/**
 * run test case.
 * the target case was set to `g_test_ctx.runtime.cur_case`
 */
static void _cutest_run_case(cutest_case_t* test_case)
{
    test_case->data.mask = 0;

    if (test_case->parameterized.type_name != NULL)
    {
        _cutest_run_case_parameterized(test_case);
        return;
    }

    _cutest_run_case_normal(test_case);
}

static void _cutest_reset_all_test_mask(void)
{
    cutest_porting_memset(&g_test_ctx.counter.result, 0, sizeof(g_test_ctx.counter.result));

    cutest_map_node_t* it = cutest_map_begin(&g_test_ctx.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_case_t* test_case = CONTAINER_OF(it, cutest_case_t, node);
        test_case->data.mask = 0;
    }
}

static void _cutest_show_report_failed(void)
{
    char buffer[512];

    cutest_map_node_t* it = cutest_map_begin(&g_test_ctx.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_case_t* test_case = CONTAINER_OF(it, cutest_case_t, node);
        if (!HAS_MASK(test_case->data.mask, MASK_FAILURE))
        {
            continue;
        }

        if (test_case->parameterized.type_name == NULL)
        {
            _cutest_get_test_fmt_name_normal(buffer, sizeof(buffer), test_case);
        }
        else
        {
            _cutest_get_test_fmt_name_parameter(buffer, sizeof(buffer), test_case);
        }

        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_RED, "[  FAILED  ]");
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, " %s\n", buffer);
    }
}

static void _cutest_show_report(const cutest_porting_timespec_t* tv_total_start,
    const cutest_porting_timespec_t* tv_total_end)
{
    cutest_porting_timespec_t tv_diff;
    cutest_timestamp_dif(tv_total_start, tv_total_end, &tv_diff);

    cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, "[==========]");
    cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, " %u/%u test case%s ran.",
        g_test_ctx.counter.result.total,
        (unsigned)g_test_ctx.case_table.size,
        g_test_ctx.counter.result.total > 1 ? "s" : "");
    if (!g_test_ctx.mask.no_print_time)
    {
        unsigned long take_time = (unsigned long)(tv_diff.tv_sec * 1000 + tv_diff.tv_nsec / 1000000);
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, " (%lu ms total)", take_time);
    }
    cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, "\n");

    if (g_test_ctx.counter.result.disabled != 0)
    {
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_GREEN, "[ DISABLED ]");
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, " %u test%s.\n",
            g_test_ctx.counter.result.disabled,
            g_test_ctx.counter.result.disabled > 1 ? "s" : "");
    }
    if (g_test_ctx.counter.result.skipped != 0)
    {
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_YELLOW,"[ BYPASSED ]");
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, " %u test%s.\n",
            g_test_ctx.counter.result.skipped,
            g_test_ctx.counter.result.skipped > 1 ? "s" : "");
    }
    if (g_test_ctx.counter.result.success != 0)
    {
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_GREEN, "[  PASSED  ]");
        cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, " %u test%s.\n",
            g_test_ctx.counter.result.success,
            g_test_ctx.counter.result.success > 1 ? "s" : "");
    }

    /* don't show failed tests if every test was success */
    if (g_test_ctx.counter.result.failed == 0)
    {
        return;
    }

    cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_RED, "[  FAILED  ]");
    cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT,
        " %u test%s, listed below:\n", g_test_ctx.counter.result.failed, g_test_ctx.counter.result.failed > 1 ? "s" : "");
    _cutest_show_report_failed();
}

static int _cutest_setup_arg_pattern(char* user_pattern)
{
    g_test_ctx.filter.pattern = _cutest_str(user_pattern);
    return 0;
}

/**
 * @return The length of whole string, including double quotation marks.
 */
static unsigned long _cutest_parameterized_parser_peek_string(const char* str)
{
    CUTEST_PORTING_ASSERT(str[0] == '"');

    unsigned long pos = 1;
    int flag_escape = 0;

    /* Find valid double quotation marks. */
    for (; str[pos] != 0; pos++)
    {
        char c = str[pos];
        if (flag_escape)
        {
            flag_escape = 0;
            continue;
        }

        switch (c)
        {
        case '\\':
            flag_escape = 1;
            break;

        case '"':
            return pos + 1;
        }
    }

    CUTEST_PORTING_ASSERT(!"string not end with quote");
    return 0;
}

static unsigned long _cutest_parameterized_parser_peek_struct(const char* str)
{
    CUTEST_PORTING_ASSERT(str[0] == '{');

    unsigned long pos = 1;
    unsigned long left_bracket = 1;

    for (; str[pos] != '\0'; pos++)
    {
        char c = str[pos];
        switch (c)
        {
        case '"':
            pos += _cutest_parameterized_parser_peek_string(str + pos) - 1;
            break;

        case '{':
            left_bracket++;
            break;

        case '}':
            left_bracket--;
            if (left_bracket == 0)
            {
                return pos + 1;
            }

        default:
            break;
        }
    }

    CUTEST_PORTING_ASSERT_P(str[pos] != '\0', "unbalanced struct: %s", str);
    return 0;
}

/**
 * @brief Parser static initialize code into sequence of tokens.
 * @param[in] code  Initialize code.
 * @param[in] idx   Index of element we want.
 * @param[out] len  The length of element code in bytes.
 * @return          The position of element code.
 */
static const char* _cutest_parameterized_parser(const char* code, unsigned long idx, int* len)
{
    const char* str = code;
    unsigned long pos = 0;

    for (; code[pos] != '\0'; pos++)
    {
        char c = code[pos];
        switch (c)
        {
        case '"':
            pos += _cutest_parameterized_parser_peek_string(code + pos) - 1;
            break;

        case ' ':
            break;

        case '{':
            pos += _cutest_parameterized_parser_peek_struct(code + pos) - 1;
            break;

        case ',':
            if (idx == 0)
            {
                goto finish;
            }
            else
            {
                idx--;
                str = code + pos + 1;
                while (*str == ' ')
                {
                    str++;
                }
            }
            break;

        default:
            break;
        }
    }

finish:
    *len = (int)(code + pos - str);
    return str;
}

static void _cutest_list_tests_print_name(const cutest_case_t* test_case)
{
    const char* case_name = test_case->info.case_name;

    if (test_case->parameterized.type_name == NULL)
    {
        cutest_porting_fprintf(g_test_ctx.out, "  %s\n", case_name);
        return;
    }

    const char* type_name = test_case->parameterized.type_name;
    unsigned long parameterized_idx = test_case->parameterized.param_idx;

    int len = 0;;
    const char* str = _cutest_parameterized_parser(test_case->parameterized.test_data_cstr, parameterized_idx, &len);

    cutest_porting_fprintf(g_test_ctx.out, "  %s/%u  # <%s> %.*s\n",
        case_name, (unsigned)parameterized_idx, type_name, len, str);
}

static void _cutest_list_tests(void)
{
    const char* last_class_name = "";

    cutest_map_node_t* it = cutest_map_begin(&g_test_ctx.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_case_t* test_case = CONTAINER_OF(it, cutest_case_t, node);

        /* some compiler will make same string with different address */
        if (last_class_name != test_case->info.fixture_name
            && cutest_porting_strcmp(last_class_name, test_case->info.fixture_name) != 0)
        {
            last_class_name = test_case->info.fixture_name;
            cutest_porting_fprintf(g_test_ctx.out, "%s.\n", last_class_name);
        }
        _cutest_list_tests_print_name(test_case);
    }
}

static void _cutest_list_types(void)
{
    cutest_map_node_t* it = cutest_map_begin(&g_test_ctx.type_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_type_info_t* type_info = CONTAINER_OF(it, cutest_type_info_t, node);
        cutest_porting_fprintf(g_test_ctx.out, "%s\n", type_info->type_name);
    }
}

static int _cutest_setup_arg_repeat(const char* str)
{
    unsigned long repeat;
    if (cutest_porting_atoul(str, &repeat) != 0)
    {
        return 1 << 8 | 1;
    }

    g_test_ctx.counter.repeat.repeat = repeat;
    return 0;
}

static int _cutest_setup_arg_print_time(const char* str)
{
    unsigned long val = 1;
    if (cutest_porting_atoul(str, &val) != 0)
    {
        return 1 << 8 | 1;
    }

    g_test_ctx.mask.no_print_time = !val;
    return 0;
}

static void _cutest_srand(unsigned long s)
{
    s = s % (MAX_RAND + 1);
    cutest_porting_srand(s);
}

static int _cutest_setup_arg_random_seed(const char* str)
{
    unsigned long val = 0;
    if (cutest_porting_atoul(str, &val) != 0)
    {
        return 1 << 8 | 1;
    }

    if (val > MAX_RAND)
    {
        return 1 << 8 | 1;
    }

    _cutest_srand(val);
    return 0;
}

static void _cutest_shuffle_cases(void)
{
    cutest_map_node_t* it;
    while ((it = cutest_map_begin(&g_test_ctx.case_table)) != NULL)
    {
        cutest_case_t* tc = CONTAINER_OF(it, cutest_case_t, node);
        if (tc->data.randkey != 0)
        {
            break;
        }

        cutest_map_erase(&g_test_ctx.case_table, it);
        tc->data.randkey = cutest_porting_rand(MAX_RAND + 1);
        cutest_map_insert(&g_test_ctx.case_table, it);
    }
}

static void _cutest_undo_shuffle_cases(void)
{
    cutest_map_node_t* it;

    while ((it = cutest_map_end(&g_test_ctx.case_table)) != NULL)
    {
        cutest_case_t* tc = CONTAINER_OF(it, cutest_case_t, node);
        if (tc->data.randkey == 0)
        {
            break;
        }

        cutest_map_erase(&g_test_ctx.case_table, it);
        tc->data.randkey = 0;
        cutest_map_insert(&g_test_ctx.case_table, it);
    }
}

static int _cutest_smart_print_int(FILE* stream, const void* addr,
    unsigned width, int is_signed)
{
    if (is_signed)
    {
        if (width == sizeof(char))
        {
            int val = *(char*)addr;
            return cutest_porting_fprintf(stream, "%d", val);
        }

        if (width == sizeof(short))
        {
            int val = *(short*)addr;
            return cutest_porting_fprintf(stream, "%d", val);
        }

        if (width == sizeof(int))
        {
            return cutest_porting_fprintf(stream, "%d", *(int*)addr);
        }

        if (width == sizeof(long))
        {
            return cutest_porting_fprintf(stream, "%ld", *(long*)addr);
        }

#if !defined(CUTEST_NO_C99_SUPPORT)
        if (width == sizeof(long long))
        {
            return cutest_porting_fprintf(stream, "%lld", *(long long*)addr);
        }
#endif
    }
    else
    {
        if (width == sizeof(unsigned char))
        {
            unsigned val = *(unsigned char*)addr;
            return cutest_porting_fprintf(stream, "%u", val);
        }

        if (width == sizeof(unsigned short))
        {
            unsigned val = *(unsigned short*)addr;
            return cutest_porting_fprintf(stream, "%u", val);
        }

        if (width == sizeof(unsigned int))
        {
            return cutest_porting_fprintf(stream, "%u", *(unsigned*)addr);
        }

        if (width == sizeof(unsigned long))
        {
            return cutest_porting_fprintf(stream, "%lu", *(unsigned long*)addr);
        }

#if !defined(CUTEST_NO_C99_SUPPORT)
        if (width == sizeof(unsigned long long))
        {
            return cutest_porting_fprintf(stream, "%llu", *(unsigned long long*)addr);
        }
#endif
    }

    CUTEST_PORTING_ASSERT_P(!"error", "width of %u does not match any native size", width);
    return 0;
}

static int _cutest_print_char(FILE* stream, const void* addr,
    unsigned width, int is_signed)
{
    (void)width; (void)is_signed;
    return cutest_porting_fprintf(stream, "%c", *(char*)addr);
}

TEST_GENERATE_TEMPLATE_COMPARE(s_type_info_char, char, _cutest_print_char);
TEST_GENERATE_TEMPLATE_COMPARE(s_type_info_signed_char, signed char, _cutest_print_char);
TEST_GENERATE_TEMPLATE_COMPARE(s_type_info_unsigned_char, unsigned char, _cutest_print_char);
TEST_GENERATE_NATIVE_COMPARE(s_type_info_short, short);
TEST_GENERATE_NATIVE_COMPARE(s_type_info_unsigned_short, unsigned short);
TEST_GENERATE_NATIVE_COMPARE(s_type_info_int, int);
TEST_GENERATE_NATIVE_COMPARE(s_type_info_unsigned_int, unsigned int);
TEST_GENERATE_NATIVE_COMPARE(s_type_info_long, long);
TEST_GENERATE_NATIVE_COMPARE(s_type_info_unsigned_long, unsigned long);

#if !defined(CUTEST_NO_C99_SUPPORT)
#include <stdint.h>
#include <stddef.h>
#if !defined(CUTEST_NO_LONGLONG_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_long_long, long long);
#endif
#if !defined(CUTEST_NO_ULONGLONG_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_unsigned_long_long, unsigned long long);
#endif
#if !defined(CUTEST_NO_INT8_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_int8_t, int8_t);
#endif
#if !defined(CUTEST_NO_UINT8_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_uint8_t, uint8_t);
#endif
#if !defined(CUTEST_NO_INT16_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_int16_t, int16_t);
#endif
#if !defined(CUTEST_NO_UINT16_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_uint16_t, uint16_t);
#endif
#if !defined(CUTEST_NO_INT32_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_int32_t, int32_t);
#endif
#if !defined(CUTEST_NO_UINT32_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_uint32_t, uint32_t);
#endif
#if !defined(CUTEST_NO_INT64_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_int64_t, int64_t);
#endif
#if !defined(CUTEST_NO_UINT64_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_uint64_t, uint64_t);
#endif
#if !defined(CUTEST_NO_SIZE_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_size_t, size_t);
#endif
#if !defined(CUTEST_NO_PTRDIFF_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_ptrdiff_t, ptrdiff_t);
#endif
#if !defined(CUTEST_NO_INTPTR_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_intptr_t, intptr_t);
#endif
#if !defined(CUTEST_NO_UINTPTR_SUPPORT)
TEST_GENERATE_NATIVE_COMPARE(s_type_info_uintptr_t, uintptr_t);
#endif
#endif

static int _cutest_cmp_ptr(const void** addr1, const void** addr2)
{
    const void* t_addr1 = *addr1;
    const void* t_addr2 = *addr2;

    const char* v1 = (const char*)t_addr1;
    const char* v2 = (const char*)t_addr2;

    if (v1 == v2)
    {
        return 0;
    }
    return v1 < v2 ? -1 : 1;
}

static int _cutest_print_ptr(FILE* file, const void** addr)
{
    return cutest_porting_fprintf(file, "%p", *addr);
}

static cutest_type_info_t s_type_info_ptr = {
    { NULL, NULL, NULL }, "const void*",
    (cutest_custom_type_cmp_fn)_cutest_cmp_ptr,
    (cutest_custom_type_dump_fn)_cutest_print_ptr,
};

static int _cutest_cmp_str(const char** addr1, const char** addr2)
{
    const char* v1 = *addr1;
    const char* v2 = *addr2;
    return cutest_porting_strcmp(v1, v2);
}

static int _cutest_print_str(FILE* file, const char** addr)
{
    const char* v = *addr;
    return cutest_porting_fprintf(file, "%s", v);
}

static cutest_type_info_t s_type_info_str = {
    { NULL, NULL, NULL }, "const char*",
    (cutest_custom_type_cmp_fn)_cutest_cmp_str,
    (cutest_custom_type_dump_fn)_cutest_print_str,
};

static int _cutest_cmp_float(const float* addr1, const float* addr2)
{
    return cutest_porting_compare_floating_number(0, addr1, addr2);
}

static int _cutest_dump_float(FILE* file, const float* addr)
{
    return cutest_porting_fprintf(file, "%f", *addr);
}

static cutest_type_info_t s_type_info_float = {
    { NULL, NULL, NULL }, "float",
    (cutest_custom_type_cmp_fn)_cutest_cmp_float,
    (cutest_custom_type_dump_fn)_cutest_dump_float,
};

static int _cutest_cmp_double(const double* addr1, const double* addr2)
{
    return cutest_porting_compare_floating_number(1, addr1, addr2);
}

static int _cutest_dump_double(FILE* file, const double* addr)
{
    return cutest_porting_fprintf(file, "%f", *addr);
}

static cutest_type_info_t s_type_info_double = {
    { NULL, NULL, NULL }, "double",
    (cutest_custom_type_cmp_fn)_cutest_cmp_double,
    (cutest_custom_type_dump_fn)_cutest_dump_double,
};

static void _cutest_setup_type(void)
{
    /* C89 */
    cutest_internal_register_type(&s_type_info_char);
    cutest_internal_register_type(&s_type_info_signed_char);
    cutest_internal_register_type(&s_type_info_unsigned_char);
    cutest_internal_register_type(&s_type_info_short);
    cutest_internal_register_type(&s_type_info_unsigned_short);
    cutest_internal_register_type(&s_type_info_int);
    cutest_internal_register_type(&s_type_info_unsigned_int);
    cutest_internal_register_type(&s_type_info_long);
    cutest_internal_register_type(&s_type_info_unsigned_long);
    cutest_internal_register_type(&s_type_info_ptr);
    cutest_internal_register_type(&s_type_info_str);
    cutest_internal_register_type(&s_type_info_float);
    cutest_internal_register_type(&s_type_info_double);

    /* C99 */
#if !defined(CUTEST_NO_C99_SUPPORT)
#if !defined(CUTEST_NO_LONGLONG_SUPPORT)
    cutest_internal_register_type(&s_type_info_long_long);
#endif
#if !defined(CUTEST_NO_ULONGLONG_SUPPORT)
    cutest_internal_register_type(&s_type_info_unsigned_long_long);
#endif
#if !defined(CUTEST_NO_INT8_SUPPORT)
    cutest_internal_register_type(&s_type_info_int8_t);
#endif
#if !defined(CUTEST_NO_UINT8_SUPPORT)
    cutest_internal_register_type(&s_type_info_uint8_t);
#endif
#if !defined(CUTEST_NO_INT16_SUPPORT)
    cutest_internal_register_type(&s_type_info_int16_t);
#endif
#if !defined(CUTEST_NO_UINT16_SUPPORT)
    cutest_internal_register_type(&s_type_info_uint16_t);
#endif
#if !defined(CUTEST_NO_INT32_SUPPORT)
    cutest_internal_register_type(&s_type_info_int32_t);
#endif
#if !defined(CUTEST_NO_UINT32_SUPPORT)
    cutest_internal_register_type(&s_type_info_uint32_t);
#endif
#if !defined(CUTEST_NO_INT64_SUPPORT)
    cutest_internal_register_type(&s_type_info_int64_t);
#endif
#if !defined(CUTEST_NO_UINT64_SUPPORT)
    cutest_internal_register_type(&s_type_info_uint64_t);
#endif
#if !defined(CUTEST_NO_SIZE_SUPPORT)
    cutest_internal_register_type(&s_type_info_size_t);
#endif
#if !defined(CUTEST_NO_PTRDIFF_SUPPORT)
    cutest_internal_register_type(&s_type_info_ptrdiff_t);
#endif
#if !defined(CUTEST_NO_INTPTR_SUPPORT)
    cutest_internal_register_type(&s_type_info_intptr_t);
#endif
#if !defined(CUTEST_NO_UINTPTR_SUPPORT)
    cutest_internal_register_type(&s_type_info_uintptr_t);
#endif
#endif
}

/**
 * @brief Setup resources that will not change.
 */
static void _cutest_setup_once(void)
{
    static int token = 0;
    if (token == 0)
    {
        token = 1;
        _cutest_setup_type();
    }
}

static void _cutest_prepare(void)
{
    cutest_porting_timespec_t seed;
    cutest_porting_clock_gettime(&seed);
    _cutest_srand((unsigned long)seed.tv_sec);

    g_test_ctx.runtime.tid = cutest_porting_gettid();
    g_test_ctx.counter.repeat.repeat = 1;
}

static int _cutest_setup_arg_help(void)
{
    _cutest_print_encoded(s_test_help_encoded);
    return 1 << 8 | 0;
}

static int _cutest_setup_arg_list_tests(void)
{
    _cutest_list_tests();
    return 1 << 8 | 0;
}

static int _cutest_setup_arg_list_types(void)
{
    _cutest_list_types();
    return 1 << 8 | 0;
}

static int _cutest_setup_arg_also_run_disabled_tests(void)
{
    g_test_ctx.mask.also_run_disabled_tests = 1;
    return 0;
}

static int _cutest_setup_arg_shuffle(void)
{
    g_test_ctx.mask.shuffle = 1;
    return 0;
}

static int _cutest_setup_arg_break_on_failure(void)
{
    g_test_ctx.mask.break_on_failure = 1;
    return 0;
}

static void _cutest_cleanup(void)
{
    /* Reset all data. */
    {
        cutest_map_t case_table = g_test_ctx.case_table;
        cutest_map_t type_table = g_test_ctx.type_table;
        cutest_porting_memset(&g_test_ctx, 0, sizeof(g_test_ctx));
        g_test_ctx.case_table = case_table;
        g_test_ctx.type_table = type_table;
    }
}

/**
 * @brief Setup test context
 * @param[in] argc      The number of command line argument.
 * @param[in] argv      Command line argument list.
 * @param[in] hook      Global test hook.
 * @param[out] b_exit   Whether need to exit.
 * @return              0 if success, otherwise failure. The lower 8 bit is the actual exit code.
 */
static int _cutest_setup(int argc, char* argv[], FILE* out, const cutest_hook_t* hook)
{
#define PARSER_LONGOPT_WITH_VALUE(OPT, FUNC)   \
    do {\
        int ret = -1; const char* opt = OPT;\
        unsigned optlen = cutest_porting_strlen(opt);\
        if (cutest_porting_strncmp(argv[i], opt, optlen) == 0) {\
            if (argv[i][optlen] == '=') {\
                ret = FUNC(argv[i] + optlen + 1);\
            } else if (i < argc - 1) {\
                ret = FUNC(argv[i + 1]); i++;\
            }\
            if (ret != 0) {\
                cutest_porting_fprintf(g_test_ctx.out, "Invalid argument to `%s'\n", opt);\
                return ret;\
            }\
            continue;\
        }\
    } while (0)

#define PARSER_LONGOPT_NO_VALUE(OPT, FUNC)   \
    do {\
        int ret = 0; const char* opt = OPT;\
        if (cutest_porting_strcmp(argv[i], opt) == 0) {\
            if ((ret = FUNC()) != 0) {\
                return ret;\
            }\
        }\
        continue;\
    } while (0)

    _cutest_setup_once();

    _cutest_cleanup();
    _cutest_prepare();

    g_test_ctx.out = out;
    g_test_ctx.hook = hook;

    int i;
    for (i = 0; i < argc; i++)
    {
        PARSER_LONGOPT_NO_VALUE("-h",                               _cutest_setup_arg_help);
        PARSER_LONGOPT_NO_VALUE("--help",                           _cutest_setup_arg_help);
        PARSER_LONGOPT_NO_VALUE("--test_list_tests",                _cutest_setup_arg_list_tests);
        PARSER_LONGOPT_NO_VALUE("--test_list_types",                _cutest_setup_arg_list_types);
        PARSER_LONGOPT_NO_VALUE("--test_also_run_disabled_tests",   _cutest_setup_arg_also_run_disabled_tests);
        PARSER_LONGOPT_NO_VALUE("--test_shuffle",                   _cutest_setup_arg_shuffle);
        PARSER_LONGOPT_NO_VALUE("--test_break_on_failure",          _cutest_setup_arg_break_on_failure);

        PARSER_LONGOPT_WITH_VALUE("--test_filter",                  _cutest_setup_arg_pattern);
        PARSER_LONGOPT_WITH_VALUE("--test_repeat",                  _cutest_setup_arg_repeat);
        PARSER_LONGOPT_WITH_VALUE("--test_random_seed",             _cutest_setup_arg_random_seed);
        PARSER_LONGOPT_WITH_VALUE("--test_print_time",              _cutest_setup_arg_print_time);
    }

    return 0;

#undef PARSER_LONGOPT_NO_VALUE
#undef PARSER_LONGOPT_WITH_VALUE
}

static void _cutest_run_all_test_once(void)
{
    _cutest_reset_all_test_mask();

    cutest_porting_timespec_t tv_total_start, tv_total_end;
    cutest_porting_clock_gettime(&tv_total_start);

    cutest_map_node_t* it = cutest_map_begin(&g_test_ctx.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        g_test_ctx.runtime.cur_node = CONTAINER_OF(it, cutest_case_t, node);
        _cutest_run_case(g_test_ctx.runtime.cur_node);
    }

    cutest_porting_clock_gettime(&tv_total_end);

    _cutest_show_report(&tv_total_start, &tv_total_end);
}

static void _cutest_show_information(void)
{
#if CUTEST_VERSION_PREREL
    cutest_porting_fprintf(g_test_ctx.out,
        "[ $VERSION ] %d.%d.%d-dev%d\n", CUTEST_VERSION_MAJOR, CUTEST_VERSION_MINOR, CUTEST_VERSION_PATCH, CUTEST_VERSION_PREREL);
#else
    cutest_porting_fprintf(g_test_ctx.out,
        "[ $VERSION ] %d.%d.%d\n", CUTEST_VERSION_MAJOR, CUTEST_VERSION_MINOR, CUTEST_VERSION_PATCH);
#endif
    cutest_porting_fprintf(g_test_ctx.out,
        "[ $PARAME. ] --test_shuffle=%d\n", (int)g_test_ctx.mask.shuffle);
    cutest_porting_fprintf(g_test_ctx.out,
        "[ $PARAME. ] --test_random_seed=%lu\n", cutest_porting_grand());
    cutest_porting_fprintf(g_test_ctx.out,
        "[ $PARAME. ] --test_also_run_disabled_tests=%d\n", (int)g_test_ctx.mask.also_run_disabled_tests);
    cutest_porting_fprintf(g_test_ctx.out,
        "[ $PARAME. ] --test_filter=%s\n", g_test_ctx.filter.pattern.ptr != NULL ? g_test_ctx.filter.pattern.ptr : "");
    cutest_porting_fprintf(g_test_ctx.out,
        "[ $PARAME. ] --test_repeat=%lu\n", g_test_ctx.counter.repeat.repeat);
    cutest_porting_fprintf(g_test_ctx.out,
        "[ $PARAME. ] --test_break_on_failure=%d\n", (int)g_test_ctx.mask.break_on_failure);
    cutest_porting_fprintf(g_test_ctx.out,
        "[ $PARAME. ] --test_print_time=%d\n", (int)!g_test_ctx.mask.no_print_time);
    cutest_porting_fprintf(g_test_ctx.out,
        "[==========] total %u test%s registered.\n",
        (unsigned)g_test_ctx.case_table.size,
        g_test_ctx.case_table.size > 1 ? "s" : "");
}

static void _cutest_run_all_tests(void)
{
    _cutest_show_information();

    for (g_test_ctx.counter.repeat.repeated = 0;
        g_test_ctx.counter.repeat.repeated < g_test_ctx.counter.repeat.repeat;
        g_test_ctx.counter.repeat.repeated++)
    {
        if (g_test_ctx.counter.repeat.repeat > 1)
        {
            cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_YELLOW, "[==========]");
            cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT,
                " start loop: %u/%u\n",
                (unsigned)(g_test_ctx.counter.repeat.repeated + 1),
                (unsigned)g_test_ctx.counter.repeat.repeat);
        }

        /* shuffle if necessary */
        if (g_test_ctx.mask.shuffle)
        {
            _cutest_shuffle_cases();
        }

        _cutest_run_all_test_once();

        /* Undo shuffle. */
        _cutest_undo_shuffle_cases();

        if (g_test_ctx.counter.repeat.repeat > 1)
        {
            cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_YELLOW, "[==========]");
            cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT,
                " end loop (%u/%u)\n",
                (unsigned)(g_test_ctx.counter.repeat.repeated + 1),
                (unsigned)g_test_ctx.counter.repeat.repeat);
            if (g_test_ctx.counter.repeat.repeated < g_test_ctx.counter.repeat.repeat - 1)
            {
                cutest_porting_cfprintf(g_test_ctx.out, CUTEST_COLOR_DEFAULT, "\n");
            }
        }
    }
}

void cutest_register_case(cutest_case_t* tc)
{
    CUTEST_PORTING_ASSERT(cutest_map_insert(&g_test_ctx.case_table, &tc->node) == 0);
}

void cutest_unregister_case(cutest_case_t* tc)
{
    cutest_map_erase(&g_test_ctx.case_table, &tc->node);
}

void cutest_case_init(cutest_case_t* tc, const char* fixture_name, const char* case_name,
    cutest_test_case_setup_fn setup, cutest_test_case_teardown_fn teardown, cutest_test_case_body_fn body)
{
    const cutest_case_t s_empty_tc = {
        { NULL, NULL, NULL },       /* .node */
        { NULL, NULL },             /* .info */
        { NULL, NULL, NULL },       /* .stage */
        { 0,0 },                    /* .data */
        { NULL, NULL, NULL, 0 },    /* .parameterized */
    };
    *tc = s_empty_tc;

    tc->info.fixture_name = fixture_name;
    tc->info.case_name = case_name;
    tc->stage.setup = setup;
    tc->stage.teardown = teardown;
    tc->stage.body = body;
}

void cutest_case_convert_parameterized(cutest_case_t* tc, const char* type,
    const char* commit, void* data, unsigned long size)
{
    tc->parameterized.type_name = type;
    tc->parameterized.test_data_cstr = commit;
    tc->parameterized.param_data = data;
    tc->parameterized.param_idx = size;
}

int cutest_run_tests(int argc, char* argv[], FILE* out, const cutest_hook_t* hook)
{
    int ret = 0;
    CUTEST_PORTING_ASSERT(out != NULL);

    /* Parser parameter */
    if ((ret = _cutest_setup(argc, argv, out, hook)) != 0)
    {
        goto fin;
    }

    _cutest_hook_before_all_test(argc, argv);
    _cutest_run_all_tests();
    ret = (int)g_test_ctx.counter.result.failed;
    _cutest_hook_after_all_test();

fin:
    _cutest_cleanup();
    return ret & 0xFF;
}

const char* cutest_get_current_fixture(void)
{
    if (g_test_ctx.runtime.cur_node == NULL)
    {
        return NULL;
    }
    return g_test_ctx.runtime.cur_node->info.fixture_name;
}

const char* cutest_get_current_test(void)
{
    if (g_test_ctx.runtime.cur_node == NULL)
    {
        return NULL;
    }
    return g_test_ctx.runtime.cur_node->info.case_name;
}

void cutest_internal_assert_failure(void)
{
    if (g_test_ctx.runtime.tid != cutest_porting_gettid())
    {
        /**
         * If current thread is NOT the main thread, it is dangerous to jump back
         * to caller stack, so we just abort the program.
         */
        cutest_abort("");
    }
    else
    {
        g_test_ctx.jmp.func(g_test_ctx.jmp.addr, MASK_FAILURE);
    }
}

void cutest_skip_test(void)
{
    SET_MASK(g_test_ctx.runtime.cur_node->data.mask, MASK_SKIPPED);
}

int cutest_internal_break_on_failure(void)
{
    return g_test_ctx.mask.break_on_failure;
}

void cutest_internal_register_type(cutest_type_info_t* info)
{
    info->node.__rb_parent_color = NULL;
    info->node.rb_left = NULL;
    info->node.rb_right = NULL;

    if (cutest_map_insert(&g_test_ctx.type_table, &info->node) < 0)
    {
        cutest_abort("Duplicate type `%s'.\n", info->type_name);
    }
}

static cutest_type_info_t* _cutest_get_type_info(const char* type_name)
{
    cutest_type_info_t tmp;
    tmp.type_name = type_name;

    cutest_map_node_t* it = cutest_map_find(&g_test_ctx.type_table, &tmp.node);
    if (it == NULL)
    {
        return NULL;
    }

    return CONTAINER_OF(it, cutest_type_info_t, node);
}

int cutest_internal_compare(const char* type_name, const void* addr1, const void* addr2)
{
    cutest_type_info_t* type_info = _cutest_get_type_info(type_name);
    CUTEST_PORTING_ASSERT_P(type_info != NULL, "%s not registered", type_name);

    return type_info->cmp(addr1, addr2);
}

void cutest_internal_dump(const char* file, int line, const char* type_name,
    const char* op, const char* op_l, const char* op_r,
    const void* addr1, const void* addr2)
{
    cutest_type_info_t* type_info = _cutest_get_type_info(type_name);
    if (type_info == NULL)
    {
        cutest_abort("%s not registered.\n", type_name);
        return;
    }

    cutest_porting_fprintf(g_test_ctx.out,
        "%s:%d:failure:\n"
        "            expected: `%s' %s `%s'\n"
        "              actual: ",
        file, line, op_l, op, op_r);
    type_info->dump(g_test_ctx.out, addr1);
    cutest_porting_fprintf(g_test_ctx.out, " vs ");
    type_info->dump(g_test_ctx.out, addr2);
    cutest_porting_fprintf(g_test_ctx.out, "\n");
}

void cutest_internal_printf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    cutest_porting_vfprintf(g_test_ctx.out, fmt, ap);
    cutest_porting_fprintf(g_test_ctx.out, "\n");
    va_end(ap);
}
