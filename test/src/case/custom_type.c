#include "test.h"

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

typedef struct custom_type_s
{
	int a;
} custom_type_t;

static int _on_cmp_custom(const void* addr1, const void* addr2)
{
	custom_type_t* v1 = (custom_type_t*)addr1;
	custom_type_t* v2 = (custom_type_t*)addr2;
	return v1->a - v2->a;
}

static int _on_print_custom(FILE* file, const void* addr)
{
	custom_type_t* v = (custom_type_t*)addr;
	return fprintf(file, "{ a:%d }", v->a);
}

TEST_REGISTER_TYPE(custom_type_t, _on_cmp_custom, _on_print_custom)

#define ASSERT_EQ_CUSTOM(a, b, ...)	ASSERT_TEMPLATE_EXT(custom_type_t, ==, a, b, __VA_ARGS__)

TEST(custom_type, 0)
{
	custom_type_t v1 = { 0 };
	custom_type_t v2 = { 0 };

	ASSERT_EQ_CUSTOM(v1, v2);
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

DEFINE_TEST(custom_type, 0)
{
	test_print_file(stderr, _TEST.hook.out);
}
