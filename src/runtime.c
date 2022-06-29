#include "runtime.h"
#include "print.h"
#include "list.h"
#include "map.h"
#include "optparser.h"
#include <string.h>
#include <assert.h>
#include <float.h>
#include <time.h>
#include <stdlib.h>

/**
 * `XX` have following parameters:
 *   +[0]: type enum
 *   +[1]: print pattern
 *   +[2]: data type
 */
#define TEST_PARAMETERIZED_MAP(XX)  \
    XX(TEST_PARAMETERIZED_TYPE_D8,      "%" PRId8,  int8_t)\
    XX(TEST_PARAMETERIZED_TYPE_U8,      "%" PRIu8,  uint8_t)\
    XX(TEST_PARAMETERIZED_TYPE_D16,     "%" PRId16, int16_t)\
    XX(TEST_PARAMETERIZED_TYPE_U16,     "%" PRIu16, uint16_t)\
    XX(TEST_PARAMETERIZED_TYPE_D32,     "%" PRId32, int32_t)\
    XX(TEST_PARAMETERIZED_TYPE_U32,     "%" PRIu32, uint32_t)\
    XX(TEST_PARAMETERIZED_TYPE_D64,     "%" PRId64, int64_t)\
    XX(TEST_PARAMETERIZED_TYPE_U64,     "%" PRIu64, uint64_t)\
    XX(TEST_PARAMETERIZED_TYPE_PTR,     "%p",       void*)\
    XX(TEST_PARAMETERIZED_TYPE_CHAR,    "%c",       char)\
    XX(TEST_PARAMETERIZED_TYPE_FLOAT,   "%f",       float)\
    XX(TEST_PARAMETERIZED_TYPE_DOUBLE,  "%f",       double)\
    XX(TEST_PARAMETERIZED_TYPE_STRING,  "%s",       const char*)

#define COLOR_GREEN(str)                    "@G" str "@D"
#define COLOR_RED(str)                      "@R" str "@D"
#define COLOR_YELLO(str)                    "@Y" str "@D"

 /**
  * @brief Print information with error code.
  */
#define PERR(errcode, fmt, ...)    \
    do {\
        char buffer[1024];\
        int err = errcode;\
        strerror_r(err, buffer, sizeof(buffer));\
        fprintf(stderr, fmt ": %s(%d).\n", ##__VA_ARGS__, buffer, err);\
    } while (0)

#if defined(_WIN32)

#include <windows.h>
#define strerror_r(errnum, buf, buflen)  strerror_s(buf, buflen, errnum)
#define sscanf(str, fmt, ...)            sscanf_s(str, fmt, ##__VA_ARGS__)

static char* strndup(const char* s, size_t n)
{
    size_t l = strnlen(s, n);
    char* d = malloc(l + 1);
    if (!d) return NULL;
    memcpy(d, s, l);
    d[l] = 0;
    return d;
}

#else

#include <pthread.h>
#include <errno.h>

static int fopen_s(FILE** pFile, const char* filename, const char* mode)
{
    if ((*pFile = fopen(filename, mode)) == NULL)
    {
        return errno;
    }
    return 0;
}

#endif

enum test_opt
{
    test_list_tests = 1,
    test_filter,
    test_also_run_disabled_tests,
    test_repeat,
    test_shuffle,
    test_random_seed,
    test_print_time,
    test_break_on_failure,
    test_logfile,
    help,
};

typedef enum test_parameterized_type
{
#define EXPAND_TEST_PARAMETERIZED_AS_ENUM(type, ign0, ign1)    type,
    TEST_PARAMETERIZED_MAP(EXPAND_TEST_PARAMETERIZED_AS_ENUM)
#undef EXPAND_TEST_PARAMETERIZED_AS_ENUM
    TEST_PARAMETERIZED_TYPE_UNKNOWN,
}test_parameterized_type_t;

static int _test_on_cmp_case(const cutest_map_node_t* key1, const cutest_map_node_t* key2, void* arg)
{
    (void)arg;
    const cutest_case_t* t_case_1 = CONTAINER_OF(key1, cutest_case_t, node.table);
    const cutest_case_t* t_case_2 = CONTAINER_OF(key2, cutest_case_t, node.table);

    return strcmp(t_case_1->info.full_name, t_case_2->info.full_name);
}

cutest_runtime_t cutest_runtime;
cutest_nature_t cutest_nature = {
    CUTEST_MAP_INIT(_test_on_cmp_case, NULL),                           /* .case_table */
    { 0, { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },                  /* .precision */
};

static cutest_optparse_long_opt_t longopts[] = {
    { "test_list_tests",                test_list_tests,                CUTEST_OPTPARSE_NONE },
    { "test_filter",                    test_filter,                    CUTEST_OPTPARSE_REQUIRED },
    { "test_also_run_disabled_tests",   test_also_run_disabled_tests,   CUTEST_OPTPARSE_NONE },
    { "test_repeat",                    test_repeat,                    CUTEST_OPTPARSE_REQUIRED },
    { "test_shuffle",                   test_shuffle,                   CUTEST_OPTPARSE_NONE },
    { "test_random_seed",               test_random_seed,               CUTEST_OPTPARSE_REQUIRED },
    { "test_print_time",                test_print_time,                CUTEST_OPTPARSE_REQUIRED },
    { "test_break_on_failure",          test_break_on_failure,          CUTEST_OPTPARSE_NONE },
    { "test_logfile",                   test_logfile,                   CUTEST_OPTPARSE_REQUIRED },
    { "help",                           help,                           CUTEST_OPTPARSE_NONE },
    { 0,                                0,                              0 },
};

static const char* s_test_help_encoded =
"This program contains tests written using cutest. You can use the\n"
"following command line flags to control its behavior:\n"
"\n"
"Test Selection:\n"
"  " COLOR_GREEN("--test_list_tests") "\n"
"      List the names of all tests instead of running them. The name of\n"
"      TEST(Foo, Bar) is \"Foo.Bar\".\n"
"  " COLOR_GREEN("--test_filter=") COLOR_YELLO("POSTIVE_PATTERNS[") COLOR_GREEN("-") COLOR_YELLO("NEGATIVE_PATTERNS]") "\n"
"      Run only the tests whose name matches one of the positive patterns but\n"
"      none of the negative patterns. '?' matches any single character; '*'\n"
"      matches any substring; ':' separates two patterns.\n"
"  " COLOR_GREEN("--test_also_run_disabled_tests") "\n"
"      Run all disabled tests too.\n"
"\n"
"Test Execution:\n"
"  " COLOR_GREEN("--test_repeat=") COLOR_YELLO("[COUNT]") "\n"
"      Run the tests repeatedly; use a negative count to repeat forever.\n"
"  " COLOR_GREEN("--test_shuffle") "\n"
"      Randomize tests' orders on every iteration.\n"
"  " COLOR_GREEN("--test_random_seed=") COLOR_YELLO("[NUMBER]") "\n"
"      Random number seed to use for shuffling test orders (between 0 and\n"
"      99999. By default a seed based on the current time is used for shuffle).\n"
"\n"
"Test Output:\n"
"  " COLOR_GREEN("--test_print_time=") COLOR_YELLO("(") COLOR_GREEN("0") COLOR_YELLO("|") COLOR_GREEN("1") COLOR_YELLO(")") "\n"
"      Don't print the elapsed time of each test.\n"
"  " COLOR_GREEN("--test_logfile=") COLOR_YELLO("[PATH]") "\n"
"      Redirect console output to file. The file will be truncate to zero first.\n"
"\n"
"Assertion Behavior:\n"
"  " COLOR_GREEN("--test_break_on_failure") "\n"
"      Turn assertion failures into debugger break-points.\n"
;

static void _test_srand(unsigned long long s)
{
    cutest_runtime.runtime.seed = s - 1;
}

static void _test_setup_precision(void)
{
    /* Ensure size match */
    assert(sizeof(((double_point_t*)NULL)->bits_) == sizeof(((double_point_t*)NULL)->value_));
    assert(sizeof(((float_point_t*)NULL)->bits_) == sizeof(((float_point_t*)NULL)->value_));

    cutest_nature.precision.kMaxUlps = 4;
    // double
    {
        cutest_nature.precision._double.kBitCount_64 = 8 * sizeof(((double_point_t*)NULL)->value_);
        cutest_nature.precision._double.kSignBitMask_64 = (uint64_t)1 << (cutest_nature.precision._double.kBitCount_64 - 1);
        cutest_nature.precision._double.kFractionBitCount_64 = DBL_MANT_DIG - 1;
        cutest_nature.precision._double.kExponentBitCount_64 = cutest_nature.precision._double.kBitCount_64 - 1 - cutest_nature.precision._double.kFractionBitCount_64;
        cutest_nature.precision._double.kFractionBitMask_64 = (~(uint64_t)0) >> (cutest_nature.precision._double.kExponentBitCount_64 + 1);
        cutest_nature.precision._double.kExponentBitMask_64 = ~(cutest_nature.precision._double.kSignBitMask_64 | cutest_nature.precision._double.kFractionBitMask_64);
    }

    // float
    {
        cutest_nature.precision._float.kBitCount_32 = 8 * sizeof(((float_point_t*)NULL)->value_);
        cutest_nature.precision._float.kSignBitMask_32 = (uint32_t)1 << (cutest_nature.precision._float.kBitCount_32 - 1);
        cutest_nature.precision._float.kFractionBitCount_32 = FLT_MANT_DIG - 1;
        cutest_nature.precision._float.kExponentBitCount_32 = cutest_nature.precision._float.kBitCount_32 - 1 - cutest_nature.precision._float.kFractionBitCount_32;
        cutest_nature.precision._float.kFractionBitMask_32 = (~(uint32_t)0) >> (cutest_nature.precision._float.kExponentBitCount_32 + 1);
        cutest_nature.precision._float.kExponentBitMask_32 = ~(cutest_nature.precision._float.kSignBitMask_32 | cutest_nature.precision._float.kFractionBitMask_32);
    }
}

#if defined(_WIN32)
static BOOL WINAPI _cutest_setup_once(PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context)
{
    (void)InitOnce; (void)Parameter; (void)Context;
#else
static void _cutest_setup_once(void)
{
#endif
    _test_srand(time(NULL));
    _test_setup_precision();

#if defined(_WIN32)
    return TRUE;
}
#else
}
#endif

/**
 * @brief Setup resources.
 */
static void _test_setup_once(void)
{
#if defined(_WIN32)
    static INIT_ONCE token = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce(&token, _cutest_setup_once, NULL, NULL);
#else
    static pthread_once_t once_control = PTHREAD_ONCE_INIT;
    pthread_once(&once_control, _cutest_setup_once);
#endif
}

static void _test_prepare(void)
{
    cutest_runtime.runtime.tid = cutest_gettid();
    cutest_runtime.counter.repeat.repeat = 1;

    cutest_map_node_t* it = cutest_map_begin(&cutest_nature.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_case_t* p_case = CONTAINER_OF(it, cutest_case_t, node.table);
        cutest_list_push_back(&cutest_runtime.info.case_list, &p_case->node.queue);
    }
}

static char _test_log_ascii_to_char(char c)
{
    if (c >= 32 && c <= 126)
    {
        return c;
    }
    return '.';
}

static void _test_list_tests_fix_escape(char* buffer, size_t size)
{
    size_t idx;
    for (idx = 0; idx < size; idx++)
    {
        buffer[idx] = _test_log_ascii_to_char(buffer[idx]);
    }
}

static void _test_list_tests_gen_param_info(char* buffer, size_t size,
    test_parameterized_type_t param_type, const cutest_case_t* case_data, size_t idx)
{
#define EXPAND_TEST_PARAMETERIZED_AS_SWITCH(type_enum, print_type, TYPE)  \
    case type_enum: {\
        TYPE* val = (TYPE*)((uintptr_t)case_data->parameterized.p_dat + case_data->parameterized.fn_get_type_size() * idx);\
        int ret = snprintf(buffer, size, "<%s> " print_type,\
            case_data->parameterized.fn_get_user_type_name(), *val);\
        size_t fix_len = ret >= (int)size ? size - 1 : (size_t)ret;\
        _test_list_tests_fix_escape(buffer, fix_len);\
    }\
    break;\

    switch (param_type)
    {
        TEST_PARAMETERIZED_MAP(EXPAND_TEST_PARAMETERIZED_AS_SWITCH)
    default:
        snprintf(buffer, size, "<%s>", case_data->parameterized.fn_get_user_type_name());
        break;
    }

#undef EXPAND_TEST_PARAMETERIZED_AS_SWITCH
}

static test_parameterized_type_t _test_get_well_known_type(const char* type_name)
{
#define CHECK_FIXED_SIZE_TYPE(RET, ...)   \
    do {\
        const char* pat[] = { __VA_ARGS__ };\
        size_t i;\
        for (i = 0; i < ARRAY_SIZE(pat); i++) {\
            if (strcmp(pat[i], type_name) == 0) {\
                return RET;\
            }\
        }\
    } while (0)
#define CHECK_BUILTIN_TYPE_WITH_WIDTH(TYPE, width) \
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_D##width, #TYPE, "signed " #TYPE " int", #TYPE " int");\
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_U##width, "unsigned " #TYPE " int", "unsigned " #TYPE " int");\
    break;
#define CHECK_BUILTIN_TYPE(TYPE)    \
    switch(sizeof(TYPE)) {\
    case 2: CHECK_BUILTIN_TYPE_WITH_WIDTH(TYPE, 16)\
    case 4: CHECK_BUILTIN_TYPE_WITH_WIDTH(TYPE, 32)\
    case 8: CHECK_BUILTIN_TYPE_WITH_WIDTH(TYPE, 64)\
    default: break;\
    }

    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_D8, "signed char", "int8_t");
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_U8, "unsigned char", "uint8_t");
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_D16, "int16_t");
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_U16, "uint16_t");
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_D32, "int32_t");
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_U32, "uint32_t");
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_D64, "int64_t");
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_U64, "uint64_t");
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_CHAR, "char");
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_FLOAT, "float");
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_DOUBLE, "double");
    CHECK_FIXED_SIZE_TYPE(TEST_PARAMETERIZED_TYPE_STRING, "const char*", "const char *", "char*", "char *");

    CHECK_BUILTIN_TYPE(short);
    CHECK_BUILTIN_TYPE(int);
    CHECK_BUILTIN_TYPE(long);
    CHECK_BUILTIN_TYPE(long long);

    if (type_name[strlen(type_name) - 1] == '*')
    {
        return TEST_PARAMETERIZED_TYPE_PTR;
    }

    return TEST_PARAMETERIZED_TYPE_UNKNOWN;

#undef CHECK_BUILTIN_TYPE
#undef CHECK_BUILTIN_TYPE_WITH_WIDTH
#undef CHECK_FIXED_SIZE_TYPE
}

static void _test_list_tests_print_name(const cutest_case_t* case_data)
{
    char buffer[64];
    if (case_data->info.type != CUTEST_CASE_TYPE_PARAMETERIZED)
    {
        cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "  %s\n", case_data->info.case_name);
        return;
    }

    const char* type_name = case_data->parameterized.fn_get_builtin_type_name();
    if (type_name == NULL)
    {
        type_name = case_data->parameterized.fn_get_user_type_name();
    }

    test_parameterized_type_t param_type = _test_get_well_known_type(type_name);

    size_t i;
    for (i = 0; i < case_data->parameterized.n_dat; i++)
    {
        _test_list_tests_gen_param_info(buffer, sizeof(buffer), param_type, case_data, i);

        cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "  %s/%" TEST_PRIsize "  # TEST_GET_PARAM() = %s\n",
            case_data->info.case_name, i, buffer);
    }
}

static void _test_list_tests(void)
{
    const char* last_class_name = "";

    cutest_map_node_t* it = cutest_map_begin(&cutest_nature.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_case_t* case_data = CONTAINER_OF(it, cutest_case_t, node.table);
        /* some compiler will make same string with different address */
        if (last_class_name != case_data->info.suit_name
            && strcmp(last_class_name, case_data->info.suit_name) != 0)
        {
            last_class_name = case_data->info.suit_name;
            cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "%s.\n", last_class_name);
        }
        _test_list_tests_print_name(case_data);
    }
}

static void _test_setup_arg_print_time(const char* str)
{
    int val = 1;
    sscanf(str, "%d", &val);
    cutest_runtime.mask.no_print_time = !val;
}

static void _test_setup_arg_random_seed(const char* str)
{
    long long val = time(NULL);
    sscanf(str, "%lld", &val);

    _test_srand(val);
}

static void _test_setup_arg_pattern(const char* user_pattern)
{
    int flag_allow_negative = 1;
    size_t number_of_patterns = 1;
    size_t number_of_negative = 0;
    size_t user_pattern_size = 0;
    while (user_pattern[user_pattern_size] != '\0')
    {
        if (user_pattern[user_pattern_size] == '-' && flag_allow_negative)
        {/* If no `:` before `-`, it is not a negative item */
            flag_allow_negative = 0;
            number_of_negative++;
        }
        else if (user_pattern[user_pattern_size] == ':')
        {
            flag_allow_negative = 1;
            number_of_patterns++;
        }
        user_pattern_size++;
    }
    user_pattern_size++;

    size_t prefix_size = sizeof(void*) * number_of_patterns;
    size_t malloc_size = prefix_size + user_pattern_size;
    cutest_runtime.filter.positive_patterns = malloc(malloc_size);
    if (cutest_runtime.filter.positive_patterns == NULL)
    {
        return;
    }
    cutest_runtime.filter.negative_patterns = cutest_runtime.filter.positive_patterns + (number_of_patterns - number_of_negative);
    memcpy((char*)cutest_runtime.filter.positive_patterns + prefix_size, user_pattern, user_pattern_size);

    char* str_it = (char*)cutest_runtime.filter.positive_patterns + prefix_size;
    do
    {
        while (*str_it == ':')
        {
            *str_it = '\0';
            str_it++;
        }

        if (*str_it == '\0')
        {
            return;
        }

        if (*str_it == '-')
        {
            *str_it = '\0';
            str_it++;

            cutest_runtime.filter.negative_patterns[cutest_runtime.filter.n_negative] = str_it;
            cutest_runtime.filter.n_negative++;
            continue;
        }

        cutest_runtime.filter.positive_patterns[cutest_runtime.filter.n_postive] = str_it;
        cutest_runtime.filter.n_postive++;
    } while ((str_it = strchr(str_it + 1, ':')) != NULL);
}

static void _test_setup_arg_repeat(const char* str)
{
    int repeat = 1;
    sscanf(str, "%d", &repeat);
    cutest_runtime.counter.repeat.repeat = (unsigned)repeat;
}

static unsigned long _test_rand(void)
{
    cutest_runtime.runtime.seed = 6364136223846793005ULL * cutest_runtime.runtime.seed + 1;
    return cutest_runtime.runtime.seed >> 33;
}

static void _test_shuffle_cases(void)
{
    cutest_list_t copy_case_list = TEST_LIST_INITIALIZER;

    while (cutest_list_size(&cutest_runtime.info.case_list) != 0)
    {
        unsigned idx = _test_rand() % cutest_list_size(&cutest_runtime.info.case_list);

        unsigned i = 0;
        cutest_list_node_t* it = cutest_list_begin(&cutest_runtime.info.case_list);
        for (; i < idx; i++, it = cutest_list_next(it))
        {
        }

        cutest_list_erase(&cutest_runtime.info.case_list, it);
        cutest_list_push_back(&copy_case_list, it);
    }

    cutest_runtime.info.case_list = copy_case_list;
}

static int _print_encoded(const char* str)
{
    char* str_tmp;
    int ret = 0;
    cutest_print_color_t color = CUTEST_PRINT_COLOR_DEFAULT;

    for (;;)
    {
        const char* p = strchr(str, '@');
        if (p == NULL)
        {
            ret += cutest_color_printf(color, "%s", str);
            return ret;
        }

        str_tmp = strndup(str, p - str);
        ret += cutest_color_printf(color, "%s", str_tmp);
        free(str_tmp);

        const char ch = p[1];
        str = p + 2;

        switch (ch)
        {
        case '@':
            ret += cutest_color_printf(color, "@");
            break;
        case 'D':
            color = 0;
            break;
        case 'R':
            color = CUTEST_PRINT_COLOR_RED;
            break;
        case 'G':
            color = CUTEST_PRINT_COLOR_GREEN;
            break;
        case 'Y':
            color = CUTEST_PRINT_COLOR_YELLOW;
            break;
        default:
            --str;
            break;
        }
    }
}

static void _test_close_logfile(void)
{
    if (cutest_runtime.io.need_close)
    {
        fclose(cutest_runtime.io.f_out);
    }
    cutest_runtime.io.f_out = NULL;
    cutest_runtime.io.need_close = 0;
}

static int _test_setup_arg_logfile(const char* path)
{
    _test_close_logfile();

    int errcode = fopen_s(&cutest_runtime.io.f_out, path, "w");
    if (errcode != 0)
    {
        PERR(errcode, "open file `%s` failed", path);
        return -1;
    }

    cutest_runtime.io.need_close = 1;
    return 0;
}

int cutest_setup(int argc, char* argv[], const cutest_hook_t* hook)
{
    (void)argc;

    memset(&cutest_runtime, 0, sizeof(cutest_runtime));
    _test_setup_once();
    _test_prepare();

    cutest_optparse_t options;
    cutest_optparse_init(&options, argv);

    int option;
    while ((option = cutest_optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
        case test_list_tests:
            _test_list_tests();
            return -1;
        case test_filter:
            _test_setup_arg_pattern(options.optarg);
            break;
        case test_also_run_disabled_tests:
            cutest_runtime.mask.also_run_disabled_tests = 1;
            break;
        case test_repeat:
            _test_setup_arg_repeat(options.optarg);
            break;
        case test_shuffle:
            cutest_runtime.mask.shuffle = 1;
            break;
        case test_random_seed:
            _test_setup_arg_random_seed(options.optarg);
            break;
        case test_print_time:
            _test_setup_arg_print_time(options.optarg);
            break;
        case test_break_on_failure:
            cutest_runtime.mask.break_on_failure = 1;
            break;
        case test_logfile:
            _test_setup_arg_logfile(options.optarg);
            break;
        case help:
            _print_encoded(s_test_help_encoded);
            return -1;
        default:
            break;
        }
    }

    /* shuffle if necessary */
    if (cutest_runtime.mask.shuffle)
    {
        _test_shuffle_cases();
    }
    cutest_runtime.hook = hook;

    return 0;
}

void cutest_cleanup(void)
{
    memset(&cutest_runtime.runtime, 0, sizeof(cutest_runtime.runtime));
    memset(&cutest_runtime.timestamp, 0, sizeof(cutest_runtime.timestamp));
    memset(&cutest_runtime.counter, 0, sizeof(cutest_runtime.counter));
    memset(&cutest_runtime.mask, 0, sizeof(cutest_runtime.mask));

    if (cutest_runtime.filter.positive_patterns != NULL)
    {
        free(cutest_runtime.filter.positive_patterns);
        memset(&cutest_runtime.filter, 0, sizeof(cutest_runtime.filter));
    }

    _test_close_logfile();
    cutest_runtime.hook = NULL;
}

unsigned long cutest_gettid(void)
{
#if defined(_WIN32)
    return ((unsigned long)GetCurrentThreadId());
#elif defined(__linux__)
    return ((unsigned long)pthread_self());
#else
    return 0;
#endif
}
