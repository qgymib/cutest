#include "runtime.h"
#include <string.h>
#include <stdlib.h>

static uint32_t _test_float_point_exponent_bits(const float_point_t* p)
{
    return cutest_nature.precision._float.kExponentBitMask_32 & p->bits_;
}

static uint32_t _test_float_point_fraction_bits(const float_point_t* p)
{
    return cutest_nature.precision._float.kFractionBitMask_32 & p->bits_;
}

static int _test_float_point_is_nan(const float_point_t* p)
{
    return (_test_float_point_exponent_bits(p) == cutest_nature.precision._float.kExponentBitMask_32)
        && (_test_float_point_fraction_bits(p) != 0);
}

static uint32_t _test_float_point_sign_and_magnitude_to_biased(const uint32_t sam)
{
    if (cutest_nature.precision._float.kSignBitMask_32 & sam)
    {
        // sam represents a negative number.
        return ~sam + 1;
    }

    // sam represents a positive number.
    return cutest_nature.precision._float.kSignBitMask_32 | sam;
}

static uint32_t _test_float_point_distance_between_sign_and_magnitude_numbers(uint32_t sam1, uint32_t sam2)
{
    const uint32_t biased1 = _test_float_point_sign_and_magnitude_to_biased(sam1);
    const uint32_t biased2 = _test_float_point_sign_and_magnitude_to_biased(sam2);

    return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
}

static uint64_t _test_double_point_sign_and_magnitude_to_biased(const uint64_t sam)
{
    if (cutest_nature.precision._double.kSignBitMask_64 & sam)
    {
        // sam represents a negative number.
        return ~sam + 1;
    }

    // sam represents a positive number.
    return cutest_nature.precision._double.kSignBitMask_64 | sam;
}

static uint64_t _test_double_point_distance_between_sign_and_magnitude_numbers(uint64_t sam1, uint64_t sam2)
{
    const uint64_t biased1 = _test_double_point_sign_and_magnitude_to_biased(sam1);
    const uint64_t biased2 = _test_double_point_sign_and_magnitude_to_biased(sam2);

    return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
}

static uint64_t _test_double_point_exponent_bits(const double_point_t* p)
{
    return cutest_nature.precision._double.kExponentBitMask_64 & p->bits_;
}

static uint64_t _test_double_point_fraction_bits(const double_point_t* p)
{
    return cutest_nature.precision._double.kFractionBitMask_64 & p->bits_;
}

static int _test_double_point_is_nan(const double_point_t* p)
{
    return (_test_double_point_exponent_bits(p) == cutest_nature.precision._double.kExponentBitMask_64)
           && (_test_double_point_fraction_bits(p) != 0);
}

int cutest_internal_assert_helper_float_eq(float a, float b)
{
    float_point_t v_a; v_a.value_ = a;
    float_point_t v_b; v_b.value_ = b;

    if (_test_float_point_is_nan(&v_a) || _test_float_point_is_nan(&v_b))
    {
        return 0;
    }

    return _test_float_point_distance_between_sign_and_magnitude_numbers(v_a.bits_, v_b.bits_)
        <= cutest_nature.precision.kMaxUlps;
}

int cutest_internal_assert_helper_double_eq(double a, double b)
{
    double_point_t v_a; v_a.value_ = a;
    double_point_t v_b; v_b.value_ = b;

    if (_test_double_point_is_nan(&v_a) || _test_double_point_is_nan(&v_b))
    {
        return 0;
    }

    return _test_double_point_distance_between_sign_and_magnitude_numbers(v_a.bits_, v_b.bits_)
        <= cutest_nature.precision.kMaxUlps;
}

int cutest_internal_assert_helper_double_le(double a, double b)
{
    return (a < b) || cutest_internal_assert_helper_double_eq(a, b);
}

int cutest_internal_assert_helper_double_ge(double a, double b)
{
    return (a > b) || cutest_internal_assert_helper_double_eq(a, b);
}

int cutest_internal_assert_helper_str_eq(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}

int cutest_internal_assert_helper_float_le(float a, float b)
{
    return (a < b) || cutest_internal_assert_helper_float_eq(a, b);
}

int cutest_internal_assert_helper_float_ge(float a, float b)
{
    return (a > b) || cutest_internal_assert_helper_float_eq(a, b);
}

void cutest_unwrap_assert_fail(const char* expr, const char* file, int line, const char* func)
{
    fprintf(stderr, "Assertion failed: %s (%s: %s: %d)\n", expr, file, func, line);
    fflush(NULL);
    abort();
}

void cutest_internal_assert_failure(void)
{
    if (cutest_runtime.runtime.tid != cutest_gettid())
    {
        /**
         * If current thread is NOT the main thread, it is dangerous to jump back
         * to caller stack, so we just abort the program.
         */
        abort();
    }
    else
    {
        ASSERT(cutest_runtime.runtime.cur_stage != STAGE_TEARDOWN);
        longjmp(cutest_runtime.jmpbuf, MASK_FAILURE);
    }
}
