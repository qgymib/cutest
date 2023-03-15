#include "test.h"

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

typedef struct custom_type_s
{
    int a;
} custom_type_t;

static int _on_cmp_custom(const custom_type_t** addr1, const custom_type_t** addr2)
{
    return (*addr1)->a - (*addr2)->a;
}

static int _on_print_custom(FILE* file, const custom_type_t** addr)
{
    return fprintf(file, "{ a:%d }", (*addr)->a);
}

#define ASSERT_EQ_CUSTOM(a, b, ...) ASSERT_TEMPLATE(const custom_type_t*, ==, a, b, __VA_ARGS__)

TEST(custom_type, 0)
{
    TEST_REGISTER_TYPE_ONCE(const custom_type_t*, _on_cmp_custom, _on_print_custom);

    custom_type_t v1 = { 0 };
    custom_type_t v2 = { 0 };

    ASSERT_EQ_CUSTOM(&v1, &v2);
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

typedef struct test_custom_type_ctx
{
    int failure_count;
} test_custom_type_ctx_t;

static test_custom_type_ctx_t g_test_custom_type_ctx;

static void after_test(const char* fixture, const char* test_name, int ret)
{
    (void)fixture; (void)test_name;
    if (ret != 0)
    {
        g_test_custom_type_ctx.failure_count++;
    }
}

DEFINE_TEST_SETUP(custom_type)
{
    _TEST.hook.after_test = after_test;
    memset(&g_test_custom_type_ctx, 0, sizeof(g_test_custom_type_ctx));
}

DEFINE_TEST_TEARDOWN(custom_type)
{

}

DEFINE_TEST_F(custom_type, 0)
{
    TEST_PORTING_ASSERT(g_test_custom_type_ctx.failure_count == 0);
}
