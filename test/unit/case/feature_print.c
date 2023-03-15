#include "test.h"
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////
// Watchpoint
///////////////////////////////////////////////////////////////////////////////

TEST(print, char)
{
	ASSERT_EQ_CHAR('a', 'b');
}

TEST(print, dchar)
{
	ASSERT_EQ_DCHAR('a', 'b');
}

TEST(print, uchar)
{
	ASSERT_EQ_UCHAR('a', 'b');
}

TEST(print, short)
{
	ASSERT_EQ_SHORT(0, 1);
}

TEST(print, ushort)
{
	ASSERT_EQ_USHORT(0, 1);
}

TEST(print, int)
{
	ASSERT_EQ_INT(0, 1);
}

TEST(print, uint)
{
	ASSERT_EQ_UINT(0, 1);
}

TEST(print, long)
{
	ASSERT_EQ_LONG(0, 1);
}

TEST(print, ulong)
{
	ASSERT_EQ_ULONG(0, 1);
}

TEST(print, longlong)
{
	ASSERT_EQ_LONGLONG(0, 1);
}

TEST(print, ulonglong)
{
	ASSERT_EQ_ULONGLONG(0, 1);
}

TEST(print, int8)
{
	ASSERT_EQ_INT8(0, 1);
}

TEST(print, uint8)
{
	ASSERT_EQ_UINT8(0, 1);
}

TEST(print, int16)
{
	ASSERT_EQ_INT16(0, 1);
}

TEST(print, uint16)
{
	ASSERT_EQ_UINT16(0, 1);
}

TEST(print, int32)
{
	ASSERT_EQ_INT32(0, 1);
}

TEST(print, uint32)
{
	ASSERT_EQ_UINT32(0, 1);
}

TEST(print, int64)
{
	ASSERT_EQ_INT64(0, 1);
}

TEST(print, uint64)
{
	ASSERT_EQ_UINT64(0, 1);
}

TEST(print, size)
{
	ASSERT_EQ_SIZE(0, 1);
}

TEST(print, ptrdiff)
{
	ASSERT_EQ_PTRDIFF(0, 1);
}

TEST(print, intptr)
{
	ASSERT_EQ_INTPTR(0, 1);
}

TEST(print, uintptr)
{
	ASSERT_EQ_UINTPTR(0, 1);
}

///////////////////////////////////////////////////////////////////////////////
// Verify
///////////////////////////////////////////////////////////////////////////////

DEFINE_TEST(print, char, "--test_filter=print.*char")
{
	string_matrix_t* matrix = string_matrix_create_from_file(_TEST.out, "\n");
	TEST_PORTING_ASSERT(matrix != NULL);

	const char* line = string_matrix_access(matrix, 12, 0);
	ASSERT_NE_PTR(strstr(line, "              actual: a vs b"), NULL);

	string_matrix_destroy(matrix);
}

DEFINE_TEST(print, not_char, "--test_filter=-print.*char")
{
	string_matrix_t* matrix = string_matrix_create_from_file(_TEST.out, "\n");
	TEST_PORTING_ASSERT(matrix != NULL);

	const char* line = string_matrix_access(matrix, 12, 0);
	ASSERT_NE_PTR(strstr(line, "              actual: 0 vs 1"), NULL);

	string_matrix_destroy(matrix);
}
