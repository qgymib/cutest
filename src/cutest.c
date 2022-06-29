#ifndef _WIN32_WINNT
#   define _WIN32_WINNT   0x0600
#endif

#include "runtime.h"
#include "print.h"
#include "list.h"
#include "map.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#if defined(_MSC_VER)
#   include <windows.h>
#   include <io.h>
#   define snprintf(str, size, fmt, ...)    _snprintf_s(str, size, _TRUNCATE, fmt, ##__VA_ARGS__)
#   ifndef strdup
#       define strdup(str)                  _strdup(str)
#   endif
#   define strncasecmp(s1, s2, n)           _strnicmp(s1, s2, n)
#else
#   include <unistd.h>
#   include <pthread.h>
#endif

/*
 * Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
 * https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
 */
#if defined(_MSC_VER) && _MSC_VER < 1900
#   pragma warning(disable : 4127)
#endif

/************************************************************************/
/* test                                                                 */
/************************************************************************/

#define SET_MASK(val, mask)                 do { (val) |= (mask); } while (0)
#define HAS_MASK(val, mask)                 ((val) & (mask))

/**
 * @brief microseconds in one second
 */
#define USEC_IN_SEC                         (1 * 1000 * 1000)

typedef struct test_ctx2
{
    char                        strbuf[128];                    /**< String buffer */
}test_ctx2_t;

static test_ctx2_t          g_test_ctx2;                                // no need to initialize

/**
 * @brief Check if `str` match `pat`
 * @return bool
 */
static int _test_pattern_matches_string(const char* pat, const char* str)
{
    switch (*pat)
    {
    case '\0':
        return *str == '\0';
    case '?':
        return *str != '\0' && _test_pattern_matches_string(pat + 1, str + 1);
    case '*':
        return (*str != '\0' && _test_pattern_matches_string(pat, str + 1)) || _test_pattern_matches_string(pat + 1, str);
    default:
        return *pat == *str && _test_pattern_matches_string(pat + 1, str + 1);
    }
}

/**
 * @return bool
 */
static int _test_check_pattern(const char* str)
{
    size_t i;
    for (i = 0; i < cutest_runtime.filter.n_negative; i++)
    {
        if (_test_pattern_matches_string(cutest_runtime.filter.negative_patterns[i], str))
        {
            return 0;
        }
    }

    if (cutest_runtime.filter.n_postive == 0)
    {
        return 1;
    }

    for (i = 0; i < cutest_runtime.filter.n_postive; i++)
    {
        if (_test_pattern_matches_string(cutest_runtime.filter.positive_patterns[i], str))
        {
            return 1;
        }
    }

    return 0;
}

/**
 * @return bool
 */
static int _test_check_disable(const char* name)
{
    return !cutest_runtime.mask.also_run_disabled_tests && (strncmp("DISABLED_", name, 9) == 0);
}

static void _test_hook_before_fixture_setup(cutest_case_t* test_case)
{
    if (cutest_runtime.hook == NULL || cutest_runtime.hook->before_fixture_setup == NULL)
    {
        return;
    }
    cutest_runtime.hook->before_fixture_setup(test_case->info.suit_name);
}

static void _test_hook_after_fixture_setup(cutest_case_t* test_case, int ret)
{
    if (cutest_runtime.hook == NULL || cutest_runtime.hook->after_fixture_setup == NULL)
    {
        return;
    }
    cutest_runtime.hook->after_fixture_setup(test_case->info.suit_name, ret);
}

static void _test_fixture_run_setup(void)
{
    if (cutest_runtime.runtime.cur_case->stage.setup == NULL)
    {
        return;
    }

    _test_hook_before_fixture_setup(cutest_runtime.runtime.cur_case);
    cutest_runtime.runtime.cur_case->stage.setup();
    _test_hook_after_fixture_setup(cutest_runtime.runtime.cur_case, 0);
}

static void _test_hook_before_normal_test(cutest_case_t* test_case)
{
    switch (test_case->info.type)
    {
    case CUTEST_CASE_TYPE_SIMPLE:
        if (cutest_runtime.hook != NULL && cutest_runtime.hook->before_simple_test != NULL)
        {
            cutest_runtime.hook->before_simple_test(test_case->info.suit_name, test_case->info.case_name);
        }
        break;

    case CUTEST_CASE_TYPE_FIXTURE:
        if (cutest_runtime.hook != NULL && cutest_runtime.hook->before_fixture_test != NULL)
        {
            cutest_runtime.hook->before_fixture_test(test_case->info.suit_name, test_case->info.case_name);
        }
        break;

    default:
        ASSERT(0);
        break;
    }
}

static void _test_hook_after_normal_test(cutest_case_t* test_case, int ret)
{
    switch (test_case->info.type)
    {
    case CUTEST_CASE_TYPE_SIMPLE:
        if (cutest_runtime.hook != NULL && cutest_runtime.hook->after_simple_test != NULL)
        {
            cutest_runtime.hook->after_simple_test(test_case->info.suit_name, test_case->info.case_name, ret);
        }
        break;

    case CUTEST_CASE_TYPE_FIXTURE:
        if (cutest_runtime.hook != NULL && cutest_runtime.hook->after_fixture_test != NULL)
        {
            cutest_runtime.hook->after_fixture_test(test_case->info.suit_name, test_case->info.case_name, ret);
        }
        break;

    default:
        ASSERT(0);
        break;
    }
}

static void _test_hook_before_parameterized_test(cutest_case_t* test_case, unsigned index)
{
    if (cutest_runtime.hook == NULL || cutest_runtime.hook->before_parameterized_test == NULL)
    {
        return;
    }
    cutest_runtime.hook->before_parameterized_test(test_case->info.suit_name,
        test_case->info.case_name, index, (unsigned)test_case->parameterized.n_dat);
}

static void _test_hook_after_parameterized_test(cutest_case_t* test_case, unsigned index, int ret)
{
    if (cutest_runtime.hook == NULL || cutest_runtime.hook->after_parameterized_test == NULL)
    {
        return;
    }
    cutest_runtime.hook->after_parameterized_test(test_case->info.suit_name,
        test_case->info.case_name, index, (unsigned)test_case->parameterized.n_dat, ret);
}

static void _test_fixture_run_body(void)
{
    if (cutest_runtime.runtime.cur_case->info.type != CUTEST_CASE_TYPE_PARAMETERIZED)
    {
        _test_hook_before_normal_test(cutest_runtime.runtime.cur_case);
        ((cutest_procedure_fn)cutest_runtime.runtime.cur_case->stage.body)();
        _test_hook_after_normal_test(cutest_runtime.runtime.cur_case, 0);
    }
    else
    {
        for (cutest_runtime.runtime.cur_parameterized_idx = 0;
            cutest_runtime.runtime.cur_parameterized_idx < cutest_runtime.runtime.cur_case->parameterized.n_dat;
            cutest_runtime.runtime.cur_parameterized_idx++)
        {
            _test_hook_before_parameterized_test(cutest_runtime.runtime.cur_case, cutest_runtime.runtime.cur_parameterized_idx);
            ((cutest_parameterized_fn)cutest_runtime.runtime.cur_case->stage.body)(cutest_runtime.runtime.cur_case->parameterized.p_dat);
            _test_hook_after_parameterized_test(cutest_runtime.runtime.cur_case, cutest_runtime.runtime.cur_parameterized_idx, -1);
        }
    }
}

static void _test_hook_before_fixture_teardown(cutest_case_t* test_case)
{
    if (cutest_runtime.hook == NULL || cutest_runtime.hook->before_fixture_teardown == NULL)
    {
        return;
    }

    cutest_runtime.hook->before_fixture_teardown(test_case->info.suit_name);
}

static void _test_hook_after_fixture_teardown(cutest_case_t* test_case, int ret)
{
    if (cutest_runtime.hook == NULL || cutest_runtime.hook->after_fixture_teardown == NULL)
    {
        return;
    }
    cutest_runtime.hook->after_fixture_teardown(test_case->info.suit_name, ret);
}

static void _test_hook_before_all_test(int argc, char* argv[])
{
    if (cutest_runtime.hook == NULL || cutest_runtime.hook->before_all_test == NULL)
    {
        return;
    }
    cutest_runtime.hook->before_all_test(argc, argv);
}

static void _test_hook_after_all_test(void)
{
    if (cutest_runtime.hook == NULL || cutest_runtime.hook->after_all_test == NULL)
    {
        return;
    }
    cutest_runtime.hook->after_all_test();
}

static void _test_fixture_run_teardown(void)
{
    if (cutest_runtime.runtime.cur_case->stage.teardown == NULL)
    {
        return;
    }

    _test_hook_before_fixture_teardown(cutest_runtime.runtime.cur_case);
    cutest_runtime.runtime.cur_case->stage.teardown();
    _test_hook_after_fixture_teardown(cutest_runtime.runtime.cur_case, 0);
}

/**
 * run test case.
 * the target case was set to `g_test_ctx.runtime.cur_case`
 */
static void _test_run_case(void)
{
    /* reset resource */
    cutest_runtime.runtime.cur_parameterized_idx = 0;
    cutest_runtime.runtime.cur_case->info.mask = 0;

    snprintf(g_test_ctx2.strbuf, sizeof(g_test_ctx2.strbuf), "%s.%s",
        cutest_runtime.runtime.cur_case->info.suit_name,
        cutest_runtime.runtime.cur_case->info.case_name);

    /* Check if need to run this test case */
    if (cutest_runtime.filter.positive_patterns != NULL && !_test_check_pattern(g_test_ctx2.strbuf))
    {
        return;
    }
    cutest_runtime.counter.result.total++;

    /* check if this test is disabled */
    if (_test_check_disable(cutest_runtime.runtime.cur_case->info.case_name))
    {
        cutest_runtime.counter.result.disabled++;
        return;
    }

    cutest_color_printf(CUTEST_PRINT_COLOR_GREEN, "[ RUN      ]");
    cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %s\n", g_test_ctx2.strbuf);

    int ret;
    if ((ret = setjmp(cutest_runtime.jmpbuf)) != 0)
    {
        SET_MASK(cutest_runtime.runtime.cur_case->info.mask, ret);
        goto procedure_teardown;
    }

    /* setup */
    cutest_runtime.runtime.cur_stage = STAGE_SETUP;
    _test_fixture_run_setup();

    /* record start time */
    ASSERT(cutest_timestamp_get(&cutest_runtime.timestamp.tv_case_start) == 0);

    /* run test case */
    cutest_runtime.runtime.cur_stage = STAGE_RUN;
    _test_fixture_run_body();

procedure_teardown:
    switch (cutest_runtime.runtime.cur_stage)
    {
    case STAGE_SETUP:
        _test_hook_after_fixture_setup(cutest_runtime.runtime.cur_case, -1);
        break;

    case STAGE_RUN:
        if (cutest_runtime.runtime.cur_case->info.type != CUTEST_CASE_TYPE_PARAMETERIZED)
        {
            _test_hook_after_normal_test(cutest_runtime.runtime.cur_case, -1);
        }
        else
        {
            _test_hook_after_parameterized_test(cutest_runtime.runtime.cur_case, cutest_runtime.runtime.cur_parameterized_idx, -1);
        }
        break;

    case STAGE_TEARDOWN:
        _test_hook_after_fixture_teardown(cutest_runtime.runtime.cur_case, -1);
        goto procedure_teardown_fin;

    default:
        ASSERT(0);
    }

    /* record end time */
    ASSERT(cutest_timestamp_get(&cutest_runtime.timestamp.tv_case_end) == 0);

    /* teardown */
    cutest_runtime.runtime.cur_stage = STAGE_TEARDOWN;
    _test_fixture_run_teardown();

procedure_teardown_fin:
    cutest_timestamp_dif(&cutest_runtime.timestamp.tv_case_start, &cutest_runtime.timestamp.tv_case_end, &cutest_runtime.timestamp.tv_diff);

    if (HAS_MASK(cutest_runtime.runtime.cur_case->info.mask, MASK_FAILURE))
    {
        cutest_runtime.counter.result.failed++;
        cutest_color_printf(CUTEST_PRINT_COLOR_RED, "[  FAILED  ]");
    }
    else if (HAS_MASK(cutest_runtime.runtime.cur_case->info.mask, MASK_SKIPPED))
    {
        cutest_runtime.counter.result.skipped++;
        cutest_color_printf(CUTEST_PRINT_COLOR_YELLOW, "[   SKIP   ]");
    }
    else
    {
        cutest_runtime.counter.result.success++;
        cutest_color_printf(CUTEST_PRINT_COLOR_GREEN, "[       OK ]");
    }

    cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %s", g_test_ctx2.strbuf);
    if (!cutest_runtime.mask.no_print_time)
    {
        unsigned long take_time = (unsigned long)(cutest_runtime.timestamp.tv_diff.sec * 1000
            + cutest_runtime.timestamp.tv_diff.usec / 1000);
        cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " (%lu ms)", take_time);
    }
    cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "\n");
}

static void _test_reset_all_test(void)
{
    memset(&cutest_runtime.counter.result, 0, sizeof(cutest_runtime.counter.result));
    memset(&cutest_runtime.timestamp, 0, sizeof(cutest_runtime.timestamp));

    cutest_list_node_t* it = cutest_list_begin(&cutest_runtime.info.case_list);
    for (; it != NULL; it = cutest_list_next(it))
    {
        cutest_case_t* case_data = CONTAINER_OF(it, cutest_case_t, node.queue);
        case_data->info.mask = 0;
    }
}

static void _test_show_report_failed(void)
{
    cutest_list_node_t* it = cutest_list_begin(&cutest_runtime.info.case_list);
    for (; it != NULL; it = cutest_list_next(it))
    {
        cutest_case_t* case_data = CONTAINER_OF(it, cutest_case_t, node.queue);
        if (!HAS_MASK(case_data->info.mask, MASK_FAILURE))
        {
            continue;
        }

        snprintf(g_test_ctx2.strbuf, sizeof(g_test_ctx2.strbuf), "%s.%s",
            case_data->info.suit_name, case_data->info.case_name);

        cutest_color_printf(CUTEST_PRINT_COLOR_RED, "[  FAILED  ]");
        cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %s\n", g_test_ctx2.strbuf);
    }
}

static void _test_show_report(void)
{
    cutest_timestamp_dif(&cutest_runtime.timestamp.tv_total_start,
        &cutest_runtime.timestamp.tv_total_end, &cutest_runtime.timestamp.tv_diff);

    cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "[==========]");
    cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %u/%u test case%s ran.",
        cutest_runtime.counter.result.total,
        (unsigned)cutest_list_size(&cutest_runtime.info.case_list),
        cutest_runtime.counter.result.total > 1 ? "s" : "");
    if (!cutest_runtime.mask.no_print_time)
    {
        unsigned long take_time = (unsigned long)(cutest_runtime.timestamp.tv_diff.sec * 1000
            + cutest_runtime.timestamp.tv_diff.usec / 1000);
        cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " (%lu ms total)", take_time);
    }
    cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "\n");

    if (cutest_runtime.counter.result.disabled != 0)
    {
        cutest_color_printf(CUTEST_PRINT_COLOR_GREEN, "[ DISABLED ]");
        cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %u test%s.\n",
            cutest_runtime.counter.result.disabled,
            cutest_runtime.counter.result.disabled > 1 ? "s" : "");
    }
    if (cutest_runtime.counter.result.skipped != 0)
    {
        cutest_color_printf(CUTEST_PRINT_COLOR_YELLOW,"[ BYPASSED ]");
        cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %u test%s.\n",
            cutest_runtime.counter.result.skipped,
            cutest_runtime.counter.result.skipped > 1 ? "s" : "");
    }
    if (cutest_runtime.counter.result.success != 0)
    {
        cutest_color_printf(CUTEST_PRINT_COLOR_GREEN, "[  PASSED  ]");
        cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %u test%s.\n",
            cutest_runtime.counter.result.success,
            cutest_runtime.counter.result.success > 1 ? "s" : "");
    }

    /* don't show failed tests if every test was success */
    if (cutest_runtime.counter.result.failed == 0)
    {
        return;
    }

    cutest_color_printf(CUTEST_PRINT_COLOR_RED, "[  FAILED  ]");
    cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT,
        " %u test%s, listed below:\n", cutest_runtime.counter.result.failed, cutest_runtime.counter.result.failed > 1 ? "s" : "");
    _test_show_report_failed();
}

static void _test_run_test_loop(void)
{
    _test_reset_all_test();

    cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "[==========]");
    cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " total %u test%s registered.\n",
        (unsigned)cutest_list_size(&cutest_runtime.info.case_list),
        cutest_list_size(&cutest_runtime.info.case_list) > 1 ? "s" : "");

    cutest_timestamp_get(&cutest_runtime.timestamp.tv_total_start);

    cutest_runtime.runtime.cur_it = cutest_list_begin(&cutest_runtime.info.case_list);
    for (; cutest_runtime.runtime.cur_it != NULL;
        cutest_runtime.runtime.cur_it = cutest_list_next(cutest_runtime.runtime.cur_it))
    {
        cutest_runtime.runtime.cur_case = CONTAINER_OF(cutest_runtime.runtime.cur_it, cutest_case_t, node.queue);
        _test_run_case();
    }

    cutest_timestamp_get(&cutest_runtime.timestamp.tv_total_end);

    _test_show_report();
}

static void _test_run_tests_condition(void)
{
    for (cutest_runtime.counter.repeat.repeated = 0;
        cutest_runtime.counter.repeat.repeated < cutest_runtime.counter.repeat.repeat;
        cutest_runtime.counter.repeat.repeated++)
    {
        if (cutest_runtime.counter.repeat.repeat > 1)
        {
            cutest_color_printf(CUTEST_PRINT_COLOR_YELLOW, "[==========]");
            cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " start loop: %u/%u\n",
                cutest_runtime.counter.repeat.repeated + 1, cutest_runtime.counter.repeat.repeat);
        }

        _test_run_test_loop();

        if (cutest_runtime.counter.repeat.repeat > 1)
        {
            cutest_color_printf(CUTEST_PRINT_COLOR_YELLOW, "[==========]");
            cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " end loop (%u/%u)\n",
                cutest_runtime.counter.repeat.repeated + 1, cutest_runtime.counter.repeat.repeat);
            if (cutest_runtime.counter.repeat.repeated < cutest_runtime.counter.repeat.repeat - 1)
            {
                cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "\n");
            }
        }
    }
}

int cutest_register_case(cutest_case_t* data)
{
    /**
     * Theoretically this operation will not fail. If two tests have duplicated
     * name, it will fail at program link stage.
     */
    return cutest_map_insert(&cutest_nature.case_table, &data->node.table);
}

int cutest_run_tests(int argc, char* argv[], const cutest_hook_t* hook)
{
    int ret;

    /* Parser parameter */
    if (cutest_setup(argc, argv, hook) < 0)
    {
        ret = 0;
        goto fin;
    }

    _test_hook_before_all_test(argc, argv);
    _test_run_tests_condition();

    ret = (int)cutest_runtime.counter.result.failed;
fin:
    _test_hook_after_all_test();
    cutest_cleanup();

    return ret;
}

const char* cutest_get_current_suit_name(void)
{
    if (cutest_runtime.runtime.cur_case == NULL)
    {
        return NULL;
    }
    return cutest_runtime.runtime.cur_case->info.suit_name;
}

const char* cutest_get_current_case_name(void)
{
    if (cutest_runtime.runtime.cur_case == NULL)
    {
        return NULL;
    }
    return cutest_runtime.runtime.cur_case->info.case_name;
}

unsigned cutest_internal_parameterized_index(void)
{
    return cutest_runtime.runtime.cur_parameterized_idx;
}

void cutest_skip_test(void)
{
    ASSERT(cutest_runtime.runtime.cur_stage == STAGE_SETUP);
    SET_MASK(cutest_runtime.runtime.cur_case->info.mask, MASK_SKIPPED);
}

void cutest_internal_flush(void)
{
    fflush(NULL);
}

int cutest_internal_break_on_failure(void)
{
    return cutest_runtime.mask.break_on_failure;
}
