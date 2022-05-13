#ifndef __TEST_ASSERTION_TEMPLATE_H__
#define __TEST_ASSERTION_TEMPLATE_H__
#ifdef __cplusplus
extern "C" {
#endif

#define TEST_ASSERTION      \
    ASSERT_EQ_D32(0, 0);    \
    ASSERT_NE_D32(0, 1);    \
    ASSERT_LT_D32(0, 1);    \
    ASSERT_LE_D32(0, 1);    \
    ASSERT_GT_D32(1, 0);    \
    ASSERT_GE_D32(1, 0);    \
                            \
    ASSERT_EQ_U32(0, 0);    \
    ASSERT_NE_U32(0, 1);    \
    ASSERT_LT_U32(0, 1);    \
    ASSERT_LE_U32(0, 1);    \
    ASSERT_GT_U32(1, 0);    \
    ASSERT_GE_U32(1, 0);    \
                            \
    ASSERT_EQ_D64(0, 0);    \
    ASSERT_NE_D64(0, 1);    \
    ASSERT_LT_D64(0, 1);    \
    ASSERT_LE_D64(0, 1);    \
    ASSERT_GT_D64(1, 0);    \
    ASSERT_GE_D64(1, 0);    \
                            \
    ASSERT_EQ_U64(0, 0);    \
    ASSERT_NE_U64(0, 1);    \
    ASSERT_LT_U64(0, 1);    \
    ASSERT_LE_U64(0, 1);    \
    ASSERT_GT_U64(1, 0);    \
    ASSERT_GE_U64(1, 0);    \
                            \
    ASSERT_EQ_SIZE(0, 0);   \
    ASSERT_NE_SIZE(0, 1);   \
    ASSERT_LT_SIZE(0, 1);   \
    ASSERT_LE_SIZE(0, 1);   \
    ASSERT_GT_SIZE(1, 0);   \
    ASSERT_GE_SIZE(1, 0);   \
                            \
    ASSERT_EQ_PTR(0, 0);    \
    ASSERT_NE_PTR(0, 1);    \
    ASSERT_LT_PTR(0, 1);    \
    ASSERT_LE_PTR(0, 1);    \
    ASSERT_GT_PTR(1, 0);    \
    ASSERT_GE_PTR(1, 0);    \
                            \
    ASSERT_EQ_FLOAT(0, 0);  \
    ASSERT_NE_FLOAT(0, 1);  \
    ASSERT_LT_FLOAT(0, 1);  \
    ASSERT_LE_FLOAT(0, 1);  \
    ASSERT_GT_FLOAT(1, 0);  \
    ASSERT_GE_FLOAT(1, 0);  \
                            \
    ASSERT_EQ_DOUBLE(0, 0); \
    ASSERT_NE_DOUBLE(0, 1); \
    ASSERT_LT_DOUBLE(0, 1); \
    ASSERT_LE_DOUBLE(0, 1); \
    ASSERT_GT_DOUBLE(1, 0); \
    ASSERT_GE_DOUBLE(1, 0); \
                            \
    ASSERT_EQ_STR("a", "a");\
    ASSERT_NE_STR("a", "")

#ifdef __cplusplus
}
#endif
#endif
