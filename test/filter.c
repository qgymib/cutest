#undef NDEBUG
#include "cutest.h"
#include <string.h>
#include <assert.h>

typedef struct test_filter
{
	size_t cnt_p1;
	size_t cnt_p2;
	size_t cnt_p3;
} test_filter_t;

static test_filter_t s_test_filter;

TEST_FIXTURE_SETUP(filter) {}
TEST_FIXTURE_TEAREDOWN(filter) {}

TEST(filter, p1)
{
	s_test_filter.cnt_p1++;
}

TEST_F(filter, p2)
{
	s_test_filter.cnt_p2++;
}

TEST_PARAMETERIZED_DEFINE(filter, p3, size_t, 1, 2, 3);
TEST_P(filter, p3)
{
	s_test_filter.cnt_p3 += TEST_GET_PARAM();
}

static void _run_with_filter(const char* argv0, const char* pattern)
{
	/* Wrap command line arguments */
	char* argv[] = {
		(char*)argv0,
		(char*)pattern,
		NULL,
	};
	int argc = sizeof(argv) / sizeof(argv[0]) - 1;

	/* Cleanup */
	memset(&s_test_filter, 0, sizeof(s_test_filter));

	/* Run tests */
	assert(cutest_run_tests(argc, argv, NULL) == 0);
}

int main(int argc, char* argv[])
{
	(void)argc;

	/*
	 * filter:
	 * + filter.p1
	 * + filter.p2
	 * + filter.p3/0
	 * + filter.p3/1
	 * + filter.p3/2
	 */
	_run_with_filter(argv[0], "--test_filter=*");
	assert(s_test_filter.cnt_p1 == 1);
	assert(s_test_filter.cnt_p2 == 1);
	assert(s_test_filter.cnt_p3 == 6);

	/*
	 * filter:
	 */
	_run_with_filter(argv[0], "--test_filter=asdf");
	assert(s_test_filter.cnt_p1 == 0);
	assert(s_test_filter.cnt_p2 == 0);
	assert(s_test_filter.cnt_p3 == 0);

	/*
	 * filter:
	 * + filter.p1
	 * + filter.p2
	 * + filter.p3/0
	 * + filter.p3/1
	 * + filter.p3/2
	 */
	_run_with_filter(argv[0], "--test_filter=filter.*");
	assert(s_test_filter.cnt_p1 == 1);
	assert(s_test_filter.cnt_p2 == 1);
	assert(s_test_filter.cnt_p3 == 6);

	/*
	 * filter:
	 * + filter.p1
	 * + filter.p2
	 */
	_run_with_filter(argv[0], "--test_filter=filter.p?");
	assert(s_test_filter.cnt_p1 == 1);
	assert(s_test_filter.cnt_p2 == 1);
	assert(s_test_filter.cnt_p3 == 0);

	/*
	 * filter:
	 * + filter.p3/0
	 * + filter.p3/1
	 * + filter.p3/2
	 */
	_run_with_filter(argv[0], "--test_filter=*/*");
	assert(s_test_filter.cnt_p1 == 0);
	assert(s_test_filter.cnt_p2 == 0);
	assert(s_test_filter.cnt_p3 == 6);
}
