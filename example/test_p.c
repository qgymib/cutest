#include "cutest.h"

TEST_FIXTURE_SETUP(example)
{
}

TEST_FIXTURE_TEAREDOWN(example)
{
}

///////////////////////////////////////////////////////////////////////////////
// example.test_p_simple
///////////////////////////////////////////////////////////////////////////////

//! [ADD_SIMPLE_PARAMETERIZED_DEFINE]
/*
 * Define parameterized test data for `example.test_p_simple`
 * The test data is typeof `int` and is defined as { 1, 2, 3 }
 */
TEST_PARAMETERIZED_DEFINE(example, test_p_simple, int, 1, 2, 3);
//! [ADD_SIMPLE_PARAMETERIZED_DEFINE]

/*
 * `TEST_P` declare a parameterized test.
 *
 * The parameterized data is defined by `TEST_PARAMETERIZED_DEFINE()`, and you
 * can get the data by `TEST_GET_PARAM()`. The parameterized test will be called
 * many times with a specific given data.
 *
 * For example, if `TEST_PARAMETERIZED_DEFINE()` define test data as { 1, 2, 3 },
 * this parameterized test will be run three round. The first round
 * `TEST_GET_PARAM()` return value of `1`, the second round `TEST_GET_PARAM()`
 * return value of `2`, and so on.
 *
 * It does not mater how many times you call `TEST_GET_PARAM()`, as it will
 * always return the same resule.
 */
TEST_P(example, test_p_simple)
{
	/* We can get parameterized data by `TEST_GET_PARAM()`. */
	int data = TEST_GET_PARAM();

	/* You will always get the same result from `TEST_GET_PARAM()` */
	ASSERT_EQ_D32(data, TEST_GET_PARAM());
}

///////////////////////////////////////////////////////////////////////////////
// example.test_p_structure
///////////////////////////////////////////////////////////////////////////////

typedef struct test_p_2_data
{
	int	a;
	int b;
	int	c;
} test_p_2_data_t;

//! [ADD_COMPLEX_PARAMETERIZED_DEFINE]
/*
 * Let's try more complex code.
 *
 * The `TEST_PARAMETERIZED_DEFINE()` macro support custom data structure like
 * `struct` or `enum`, you can define any type you want.
 */
TEST_PARAMETERIZED_DEFINE(example, test_p_structure, test_p_2_data_t, { 1, 2, 3 }, { 2, 3, 5 });
//! [ADD_COMPLEX_PARAMETERIZED_DEFINE]

//! [GET_PARAMETERIZED_DATA]
TEST_P(example, test_p_structure)
{
	/*
	 * The `TEST_GET_PARAM()` is strong typed, it returns the same type as you
	 * define.
	 */
	test_p_2_data_t data = TEST_GET_PARAM();

	/* Let's do summation for test data. */
	ASSERT_EQ_D32(data.a + data.b, data.c);
}
//! [GET_PARAMETERIZED_DATA]

///////////////////////////////////////////////////////////////////////////////
// example.test_p_repeat
///////////////////////////////////////////////////////////////////////////////

/*
 * Of course you may want to use parameterized test just to repeat your test
 * code.
 *
 * In such condition just write some random thing in `TEST_PARAMETERIZED_DEFINE()`,
 * just ensure the number of parameterized test data meet your need.
 */
TEST_PARAMETERIZED_DEFINE(example, test_p_repeat, int, 0, 0, 0);

//! [SUPPRESS_UNUSED_PARAMETERIZED_DATA]
TEST_P(example, test_p_repeat)
{
	/*
	 * We don't call `TEST_GET_PARAM()`, so there might be some warnings during
	 * code compile. Use `TEST_PARAMETERIZED_SUPPRESS_UNUSED` to suppress it.
	 */
	TEST_PARAMETERIZED_SUPPRESS_UNUSED;
}
//! [SUPPRESS_UNUSED_PARAMETERIZED_DATA]
