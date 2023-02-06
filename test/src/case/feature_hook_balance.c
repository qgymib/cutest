#include "test.h"

typedef struct test_ctx
{
	unsigned cutest_setup_cnt;
	unsigned cutest_teardown_cnt;

	unsigned hook_before_all_test_cnt;
	unsigned hook_after_all_test_cnt;

	unsigned hook_before_setup_cnt;
	unsigned hook_after_setup_cnt;

	unsigned hook_before_teardown_cnt;
	unsigned hook_after_teardown_cnt;

	unsigned hook_before_test_cnt;
	unsigned hook_after_test_cnt;
} test_ctx_t;

static test_ctx_t s_test_ctx;

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

TEST_FIXTURE_SETUP(hook)
{
	s_test_ctx.cutest_setup_cnt++;
}

TEST_FIXTURE_TEAREDOWN(hook)
{
	s_test_ctx.cutest_teardown_cnt++;
}

TEST(hook, balance_0)
{
}

TEST_F(hook, balance_1)
{
}

TEST_PARAMETERIZED_DEFINE(hook, balance_2, int, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
TEST_P(hook, balance_2)
{
	TEST_PARAMETERIZED_SUPPRESS_UNUSED;
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

static void _on_before_setup(const char* fixture)
{
	(void)fixture;
	s_test_ctx.hook_before_setup_cnt++;
}

static void _on_after_setup(const char* fixture, int ret)
{
	(void)fixture; (void)ret;
	s_test_ctx.hook_after_setup_cnt++;
}

static void _on_before_teardown(const char* fixture)
{
	(void)fixture;
	s_test_ctx.hook_before_teardown_cnt++;
}

static void _on_after_teardown(const char* fixture, int ret)
{
	(void)fixture; (void)ret;
	s_test_ctx.hook_after_teardown_cnt++;
}

static void _on_before_all_test(int argc, char* argv[])
{
	(void)argc; (void)argv;
	s_test_ctx.hook_before_all_test_cnt++;
}

static void _on_after_all_test(void)
{
	s_test_ctx.hook_after_all_test_cnt++;
}

static void _on_before_test(const char* fixture, const char* test_name)
{
	(void)fixture; (void)test_name;
	s_test_ctx.hook_before_test_cnt++;
}

static void _on_after_test(const char* fixture, const char* test_name, int ret)
{
	(void)fixture; (void)test_name; (void)ret;
	s_test_ctx.hook_after_test_cnt++;
}

DEFINE_TEST_SETUP(hook)
{
	_TEST.hook.before_all_test = _on_before_all_test;
	_TEST.hook.after_all_test = _on_after_all_test;
	_TEST.hook.before_setup = _on_before_setup;
	_TEST.hook.after_setup = _on_after_setup;
	_TEST.hook.before_teardown = _on_before_teardown;
	_TEST.hook.after_teardown = _on_after_teardown;
	_TEST.hook.before_test = _on_before_test;
	_TEST.hook.after_test = _on_after_test;

	memset(&s_test_ctx, 0, sizeof(s_test_ctx));
}

DEFINE_TEST_TEARDOWN(hook)
{
}

DEFINE_TEST_F(hook, callback_balance)
{
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_all_test_cnt != 0);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_setup_cnt != 0);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_teardown_cnt != 0);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_test_cnt != 0);

	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_all_test_cnt == s_test_ctx.hook_after_all_test_cnt);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_setup_cnt == s_test_ctx.hook_after_setup_cnt);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_teardown_cnt == s_test_ctx.hook_after_teardown_cnt);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_test_cnt == s_test_ctx.hook_after_test_cnt);
}

DEFINE_TEST_F(hook, callback_balance_help, "--help")
{
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_all_test_cnt == 0);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_setup_cnt == 0);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_teardown_cnt == 0);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_test_cnt == 0);

	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_all_test_cnt == s_test_ctx.hook_after_all_test_cnt);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_setup_cnt == s_test_ctx.hook_after_setup_cnt);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_teardown_cnt == s_test_ctx.hook_after_teardown_cnt);
	CUTEST_PORTING_ASSERT(s_test_ctx.hook_before_test_cnt == s_test_ctx.hook_after_test_cnt);
}
