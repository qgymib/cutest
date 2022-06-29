#ifndef __CUTEST_RUNTIME_H__
#define __CUTEST_RUNTIME_H__

#include "cutest.h"
#include <setjmp.h>

#define ARRAY_SIZE(arr)                     (sizeof(arr) / sizeof(arr[0]))

#define CONTAINER_OF(ptr, TYPE, member) \
    ((TYPE*)((char*)(ptr) - (size_t)&((TYPE*)0)->member))

#define MASK_FAILURE                        (0x01 << 0x00)
#define MASK_SKIPPED                        (0x01 << 0x01)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum test_case_stage
{
    STAGE_SETUP = 0,
    STAGE_RUN,
    STAGE_TEARDOWN,
}test_case_stage_t;

typedef union float_point
{
    float                   value_;
    uint32_t                bits_;
}float_point_t;

typedef union double_point
{
    double                  value_;
    uint64_t                bits_;
}double_point_t;

typedef struct cutest_runtime_s
{
    struct
    {
        cutest_list_t       case_list;                      /**< Cases in list */
    } info;

    struct
    {
        unsigned long       tid;                            /**< Thread ID */
        unsigned long long  seed;                           /**< Random seed */
        cutest_list_node_t* cur_it;                         /**< Current cursor position */
        cutest_case_t*      cur_case;                       /**< Current running test case */
        unsigned            cur_parameterized_idx;          /**< Current parameterized index */
        test_case_stage_t   cur_stage;                      /**< Current running stage */
    } runtime;

    struct
    {
        cutest_timestamp_t  tv_case_start;                  /**< Test start time */
        cutest_timestamp_t  tv_case_end;                    /**< Test end time */

        cutest_timestamp_t  tv_total_start;                 /**< The start time of whole test */
        cutest_timestamp_t  tv_total_end;                   /**< The end time of whole test */

        cutest_timestamp_t  tv_diff;                        /**< Time diff */
    } timestamp;

    struct
    {
        struct
        {
            unsigned        total;                          /**< The number of total running cases */
            unsigned        disabled;                       /**< The number of disabled cases */
            unsigned        success;                        /**< The number of successed cases */
            unsigned        skipped;                        /**< The number of skipped cases */
            unsigned        failed;                         /**< The number of failed cases */
        } result;

        struct
        {
            unsigned        repeat;                         /**< How many times need to repeat */
            unsigned        repeated;                       /**< How many times alread repeated */
        } repeat;
    } counter;

    struct
    {
        unsigned            break_on_failure : 1;           /**< DebugBreak when failure */
        unsigned            no_print_time : 1;              /**< Whether to print execution cost time */
        unsigned            also_run_disabled_tests : 1;    /**< Also run disabled tests */
        unsigned            shuffle : 1;                    /**< Randomize running cases */
    } mask;

    struct
    {
        char**              positive_patterns;              /**< positive patterns for filter */
        char**              negative_patterns;              /**< negative patterns for filter */
        size_t              n_negative;                     /**< The number of negative patterns */
        size_t              n_postive;                      /**< The number of positive patterns */
    } filter;                                               /**< Context for float/double compare */

    struct
    {
        FILE*               f_out;                          /**< Output file */
        int                 need_close;                     /**< Need close */
    } io;

    jmp_buf                 jmpbuf;                         /**< Jump buffer */
    const cutest_hook_t*    hook;                           /**< User hook */

} cutest_runtime_t;

typedef struct cutest_nature_s
{
    cutest_map_t            case_table;                     /**< Cases in map */

    struct
    {
        size_t              kMaxUlps;
        struct
        {
            size_t          kBitCount_64;
            size_t          kFractionBitCount_64;
            size_t          kExponentBitCount_64;
            uint64_t        kSignBitMask_64;
            uint64_t        kFractionBitMask_64;
            uint64_t        kExponentBitMask_64;
        }_double;
        struct
        {
            size_t          kBitCount_32;
            size_t          kFractionBitCount_32;
            size_t          kExponentBitCount_32;
            uint32_t        kSignBitMask_32;
            uint32_t        kFractionBitMask_32;
            uint32_t        kExponentBitMask_32;
        }_float;
    }precision;
} cutest_nature_t;

/**
 * @brief Test Runtime.
 * 
 * This is a structure that will change during test execution.
 */
extern cutest_runtime_t     cutest_runtime;

/**
 * @brief Test nature.
 * 
 * This is a structure that will not change during test execution.
 */
extern cutest_nature_t      cutest_nature;

/**
 * @brief Setup runtime.
 */
int cutest_setup(int argc, char* argv[], const cutest_hook_t* hook);

/**
 * @brief Cleanup runtime.
 */
void cutest_cleanup(void);

/**
 * @brief Get current thread ID.
 * @return      Thread ID.
 */
unsigned long cutest_gettid(void);

#ifdef __cplusplus
}
#endif

#endif
