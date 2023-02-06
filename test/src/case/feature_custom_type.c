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

#define ASSERT_EQ_CUSTOM(a, b, ...)	ASSERT_TEMPLATE_EXT(const custom_type_t*, ==, a, b, __VA_ARGS__)

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

DEFINE_TEST(custom_type, 0)
{
	test_print_file(stderr, _TEST.out);
}
