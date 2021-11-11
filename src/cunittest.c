#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <float.h>
#include "cunittest.h"

/*
 * Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
 * https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
 */
#if defined(_MSC_VER) && _MSC_VER < 1900
#   pragma warning(disable : 4127)
#endif

/************************************************************************/
/* map                                                                  */
/************************************************************************/

#define RB_RED              0
#define RB_BLACK            1
#define __rb_color(pc)      ((uintptr_t)(pc) & 1)
#define __rb_is_black(pc)   __rb_color(pc)
#define __rb_is_red(pc)     (!__rb_color(pc))
#define __rb_parent(pc)     ((cunittest_map_node_t*)(pc & ~3))
#define rb_color(rb)        __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb)       __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb)     __rb_is_black((rb)->__rb_parent_color)
#define rb_parent(r)        ((cunittest_map_node_t*)((uintptr_t)((r)->__rb_parent_color) & ~3))

/* 'empty' nodes are nodes that are known not to be inserted in an rbtree */
#define RB_EMPTY_NODE(node)  \
    ((node)->__rb_parent_color == (cunittest_map_node_t*)(node))

static void _etest_map_link_node(cunittest_map_node_t* node, cunittest_map_node_t* parent, cunittest_map_node_t** rb_link)
{
    node->__rb_parent_color = parent;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
    return;
}

static cunittest_map_node_t* rb_red_parent(cunittest_map_node_t *red)
{
    return red->__rb_parent_color;
}

static void rb_set_parent_color(cunittest_map_node_t *rb, cunittest_map_node_t *p, int color)
{
    rb->__rb_parent_color = (cunittest_map_node_t*)((uintptr_t)p | color);
}

static void __rb_change_child(cunittest_map_node_t* old_node, cunittest_map_node_t* new_node,
    cunittest_map_node_t* parent, cunittest_map_t* root)
{
    if (parent)
    {
        if (parent->rb_left == old_node)
        {
            parent->rb_left = new_node;
        }
        else
        {
            parent->rb_right = new_node;
        }
    }
    else
    {
        root->rb_root = new_node;
    }
}

/*
* Helper function for rotations:
* - old's parent and color get assigned to new
* - old gets assigned new as a parent and 'color' as a color.
*/
static void __rb_rotate_set_parents(cunittest_map_node_t* old, cunittest_map_node_t* new_node,
    cunittest_map_t* root, int color)
{
    cunittest_map_node_t* parent = rb_parent(old);
    new_node->__rb_parent_color = old->__rb_parent_color;
    rb_set_parent_color(old, new_node, color);
    __rb_change_child(old, new_node, parent, root);
}

static void _etest_map_insert(cunittest_map_node_t* node, cunittest_map_t* root)
{
    cunittest_map_node_t* parent = rb_red_parent(node), *gparent, *tmp;

    while (1) {
        /*
        * Loop invariant: node is red
        *
        * If there is a black parent, we are done.
        * Otherwise, take some corrective action as we don't
        * want a red root or two consecutive red nodes.
        */
        if (!parent) {
            rb_set_parent_color(node, NULL, RB_BLACK);
            break;
        }
        else if (rb_is_black(parent))
            break;

        gparent = rb_red_parent(parent);

        tmp = gparent->rb_right;
        if (parent != tmp) {    /* parent == gparent->rb_left */
            if (tmp && rb_is_red(tmp)) {
                /*
                * Case 1 - color flips
                *
                *       G            g
                *      / \          / \
                *     p   u  -->   P   U
                *    /            /
                *   n            n
                *
                * However, since g's parent might be red, and
                * 4) does not allow this, we need to recurse
                * at g.
                */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->rb_right;
            if (node == tmp) {
                /*
                * Case 2 - left rotate at parent
                *
                *      G             G
                *     / \           / \
                *    p   U  -->    n   U
                *     \           /
                *      n         p
                *
                * This still leaves us in violation of 4), the
                * continuation into Case 3 will fix that.
                */
                parent->rb_right = tmp = node->rb_left;
                node->rb_left = parent;
                if (tmp)
                    rb_set_parent_color(tmp, parent,
                    RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                parent = node;
                tmp = node->rb_right;
            }

            /*
            * Case 3 - right rotate at gparent
            *
            *        G           P
            *       / \         / \
            *      p   U  -->  n   g
            *     /                 \
            *    n                   U
            */
            gparent->rb_left = tmp;  /* == parent->rb_right */
            parent->rb_right = gparent;
            if (tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, root, RB_RED);
            break;
        }
        else {
            tmp = gparent->rb_left;
            if (tmp && rb_is_red(tmp)) {
                /* Case 1 - color flips */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->rb_left;
            if (node == tmp) {
                /* Case 2 - right rotate at parent */
                parent->rb_left = tmp = node->rb_right;
                node->rb_right = parent;
                if (tmp)
                    rb_set_parent_color(tmp, parent,
                    RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                parent = node;
                tmp = node->rb_left;
            }

            /* Case 3 - left rotate at gparent */
            gparent->rb_right = tmp;  /* == parent->rb_left */
            parent->rb_left = gparent;
            if (tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, root, RB_RED);
            break;
        }
    }
}

static void _etest_map_insert_color(cunittest_map_node_t* node, cunittest_map_t* root)
{
    _etest_map_insert(node, root);
}

int cunittest_map_insert(cunittest_map_t* handler, cunittest_map_node_t* node)
{
    cunittest_map_node_t **new_node = &(handler->rb_root), *parent = NULL;

    /* Figure out where to put new node */
    while (*new_node)
    {
        int result = handler->cmp.cmp(node, *new_node, handler->cmp.arg);

        parent = *new_node;
        if (result < 0)
        {
            new_node = &((*new_node)->rb_left);
        }
        else if (result > 0)
        {
            new_node = &((*new_node)->rb_right);
        }
        else
        {
            return -1;
        }
    }

    handler->size++;
    _etest_map_link_node(node, parent, new_node);
    _etest_map_insert_color(node, handler);

    return 0;
}

cunittest_map_node_t* cunittest_map_begin(const cunittest_map_t* handler)
{
    cunittest_map_node_t* n = handler->rb_root;

    if (!n)
        return NULL;
    while (n->rb_left)
        n = n->rb_left;
    return n;
}

cunittest_map_node_t* cunittest_map_next(const cunittest_map_node_t* node)
{
    cunittest_map_node_t* parent;

    if (RB_EMPTY_NODE(node))
        return NULL;

    /*
    * If we have a right-hand child, go down and then left as far
    * as we can.
    */
    if (node->rb_right) {
        node = node->rb_right;
        while (node->rb_left)
            node = node->rb_left;
        return (cunittest_map_node_t *)node;
    }

    /*
    * No right-hand children. Everything down and left is smaller than us,
    * so any 'next' node must be in the general direction of our parent.
    * Go up the tree; any time the ancestor is a right-hand child of its
    * parent, keep going up. First time it's a left-hand child of its
    * parent, said parent is our 'next' node.
    */
    while ((parent = rb_parent(node)) != NULL && node == parent->rb_right)
        node = parent;

    return parent;
}

size_t cunittest_map_size(const cunittest_map_t* handler)
{
    return handler->size;
}

/************************************************************************/
/* argument parser                                                      */
/************************************************************************/

#define OPTPARSE_MSG_INVALID "invalid option"
#define OPTPARSE_MSG_MISSING "option requires an argument"
#define OPTPARSE_MSG_TOOMANY "option takes no arguments"

typedef enum test_optparse_argtype {
    OPTPARSE_NONE,
    OPTPARSE_REQUIRED,
    OPTPARSE_OPTIONAL
}test_optparse_argtype_t;

typedef struct test_optparse_long_opt {
    const char*             longname;
    int                     shortname;
    test_optparse_argtype_t argtype;
}test_optparse_long_opt_t;

typedef struct test_optparse {
    char**                  argv;
    int                     permute;
    int                     optind;
    int                     optopt;
    char*                   optarg;
    char                    errmsg[64];
    size_t                  subopt;
}test_optparse_t;

static int _test_optparse_is_dashdash(const char *arg)
{
    return arg != 0 && arg[0] == '-' && arg[1] == '-' && arg[2] == '\0';
}

static int _test_optparse_is_shortopt(const char *arg)
{
    return arg != 0 && arg[0] == '-' && arg[1] != '-' && arg[1] != '\0';
}

static int _test_optparse_is_longopt(const char *arg)
{
    return arg != 0 && arg[0] == '-' && arg[1] == '-' && arg[2] != '\0';
}

static int _test_optparse_longopts_end(const test_optparse_long_opt_t *longopts, int i)
{
    return !longopts[i].longname && !longopts[i].shortname;
}

static int _test_optparse_longopts_match(const char *longname, const char *option)
{
    const char *a = option, *n = longname;
    if (longname == 0)
        return 0;
    for (; *a && *n && *a != '='; a++, n++)
        if (*a != *n)
            return 0;
    return *n == '\0' && (*a == '\0' || *a == '=');
}

static char* _test_optparse_longopts_arg(char *option)
{
    for (; *option && *option != '='; option++);
    if (*option == '=')
        return option + 1;
    else
        return 0;
}

static void _test_optparse_from_long(const test_optparse_long_opt_t *longopts, char *optstring)
{
    char *p = optstring;
    int i;
    for (i = 0; !_test_optparse_longopts_end(longopts, i); i++) {
        if (longopts[i].shortname) {
            int a;
            *p++ = (char)(longopts[i].shortname);
            for (a = 0; a < (int)longopts[i].argtype; a++)
                *p++ = ':';
        }
    }
    *p = '\0';
}

static void _test_optparse_permute(test_optparse_t *options, int index)
{
    char *nonoption = options->argv[index];
    int i;
    for (i = index; i < options->optind - 1; i++)
        options->argv[i] = options->argv[i + 1];
    options->argv[options->optind - 1] = nonoption;
}

static int _test_optparse_argtype(const char *optstring, char c)
{
    int count = OPTPARSE_NONE;
    if (c == ':')
        return -1;
    for (; *optstring && c != *optstring; optstring++);
    if (!*optstring)
        return -1;
    if (optstring[1] == ':')
        count += optstring[2] == ':' ? 2 : 1;
    return count;
}

static int _test_optparse_error(test_optparse_t *options, const char *msg, const char *data)
{
    unsigned p = 0;
    const char *sep = " -- '";
    while (*msg)
        options->errmsg[p++] = *msg++;
    while (*sep)
        options->errmsg[p++] = *sep++;
    while (p < sizeof(options->errmsg) - 2 && *data)
        options->errmsg[p++] = *data++;
    options->errmsg[p++] = '\'';
    options->errmsg[p++] = '\0';
    return '?';
}

static int _test_optparse(test_optparse_t *options, const char *optstring)
{
    int type;
    char *next;
    char *option = options->argv[options->optind];
    options->errmsg[0] = '\0';
    options->optopt = 0;
    options->optarg = 0;
    if (option == 0) {
        return -1;
    }
    else if (_test_optparse_is_dashdash(option)) {
        options->optind++; /* consume "--" */
        return -1;
    }
    else if (!_test_optparse_is_shortopt(option)) {
        if (options->permute) {
            int index = options->optind++;
            int r = _test_optparse(options, optstring);
            _test_optparse_permute(options, index);
            options->optind--;
            return r;
        }
        else {
            return -1;
        }
    }
    option += (options->subopt + 1);
    options->optopt = option[0];
    type = _test_optparse_argtype(optstring, option[0]);
    next = options->argv[options->optind + 1];
    switch (type) {
    case -1: {
        char str[2] = { 0, 0 };
        str[0] = option[0];
        options->optind++;
        return _test_optparse_error(options, OPTPARSE_MSG_INVALID, str);
    }
    case OPTPARSE_NONE:
        if (option[1]) {
            options->subopt++;
        }
        else {
            options->subopt = 0;
            options->optind++;
        }
        return option[0];
    case OPTPARSE_REQUIRED:
        options->subopt = 0;
        options->optind++;
        if (option[1]) {
            options->optarg = option + 1;
        }
        else if (next != 0) {
            options->optarg = next;
            options->optind++;
        }
        else {
            char str[2] = { 0, 0 };
            str[0] = option[0];
            options->optarg = 0;
            return _test_optparse_error(options, OPTPARSE_MSG_MISSING, str);
        }
        return option[0];
    case OPTPARSE_OPTIONAL:
        options->subopt = 0;
        options->optind++;
        if (option[1])
            options->optarg = option + 1;
        else
            options->optarg = 0;
        return option[0];
    }
    return 0;
}

static int _test_optparse_long_fallback(test_optparse_t *options, const test_optparse_long_opt_t *longopts, int *longindex)
{
    int result;
    char optstring[96 * 3 + 1]; /* 96 ASCII printable characters */
    _test_optparse_from_long(longopts, optstring);
    result = _test_optparse(options, optstring);
    if (longindex != 0) {
        *longindex = -1;
        if (result != -1) {
            int i;
            for (i = 0; !_test_optparse_longopts_end(longopts, i); i++)
                if (longopts[i].shortname == options->optopt)
                    *longindex = i;
        }
    }
    return result;
}

static int test_optparse_long(test_optparse_t *options, const test_optparse_long_opt_t *longopts, int *longindex)
{
    int i;
    char *option = options->argv[options->optind];
    if (option == 0) {
        return -1;
    }
    else if (_test_optparse_is_dashdash(option)) {
        options->optind++; /* consume "--" */
        return -1;
    }
    else if (_test_optparse_is_shortopt(option)) {
        return _test_optparse_long_fallback(options, longopts, longindex);
    }
    else if (!_test_optparse_is_longopt(option)) {
        if (options->permute) {
            int index = options->optind++;
            int r = test_optparse_long(options, longopts, longindex);
            _test_optparse_permute(options, index);
            options->optind--;
            return r;
        }
        else {
            return -1;
        }
    }

    /* Parse as long option. */
    options->errmsg[0] = '\0';
    options->optopt = 0;
    options->optarg = 0;
    option += 2; /* skip "--" */
    options->optind++;
    for (i = 0; !_test_optparse_longopts_end(longopts, i); i++) {
        const char *name = longopts[i].longname;
        if (_test_optparse_longopts_match(name, option)) {
            char *arg;
            if (longindex)
                *longindex = i;
            options->optopt = longopts[i].shortname;
            arg = _test_optparse_longopts_arg(option);
            if (longopts[i].argtype == OPTPARSE_NONE && arg != 0) {
                return _test_optparse_error(options, OPTPARSE_MSG_TOOMANY, name);
            } if (arg != 0) {
                options->optarg = arg;
            }
            else if (longopts[i].argtype == OPTPARSE_REQUIRED) {
                options->optarg = options->argv[options->optind];
                if (options->optarg == 0)
                    return _test_optparse_error(options, OPTPARSE_MSG_MISSING, name);
                else
                    options->optind++;
            }
            return options->optopt;
        }
    }
    return _test_optparse_error(options, OPTPARSE_MSG_INVALID, option);
}

static void test_optparse_init(test_optparse_t *options, char **argv)
{
    options->argv = argv;
    options->permute = 1;
    options->optind = 1;
    options->subopt = 0;
    options->optarg = 0;
    options->errmsg[0] = '\0';
}

/************************************************************************/
/* list                                                                 */
/************************************************************************/

static void _test_list_set_once(cunittest_list_t* handler, cunittest_list_node_t* node)
{
    handler->head = node;
    handler->tail = node;
    node->p_after = NULL;
    node->p_before = NULL;
    handler->size = 1;
}

void cunittest_list_push_back(cunittest_list_t* handler, cunittest_list_node_t* node)
{
    if (handler->head == NULL)
    {
        _test_list_set_once(handler, node);
        return;
    }

    node->p_after = NULL;
    node->p_before = handler->tail;
    handler->tail->p_after = node;
    handler->tail = node;
    handler->size++;
}

cunittest_list_node_t* cunittest_list_begin(const cunittest_list_t* handler)
{
    return handler->head;
}

cunittest_list_node_t* cunittest_list_next(const cunittest_list_node_t* node)
{
    return node->p_after;
}

size_t cunittest_list_size(const cunittest_list_t* handler)
{
    return handler->size;
}

void cunittest_list_erase(cunittest_list_t* handler, cunittest_list_node_t* node)
{
    handler->size--;

    if (handler->head == node && handler->tail == node)
    {
        handler->head = NULL;
        handler->tail = NULL;
        return;
    }

    if (handler->head == node)
    {
        node->p_after->p_before = NULL;
        handler->head = node->p_after;
        return;
    }

    if (handler->tail == node)
    {
        node->p_before->p_after = NULL;
        handler->tail = node->p_before;
        return;
    }

    node->p_before->p_after = node->p_after;
    node->p_after->p_before = node->p_before;
    return;
}

/************************************************************************/
/* test                                                                 */
/************************************************************************/

#if defined(_MSC_VER)
#   include <windows.h>
#   include <time.h>
#   define GET_TID()                        ((unsigned long)GetCurrentThreadId())
#   define snprintf(str, size, fmt, ...)    _snprintf_s(str, size, _TRUNCATE, fmt, ##__VA_ARGS__)
#   ifndef strdup
#       define strdup(str)                  _strdup(str)
#   endif
#   define strncasecmp(s1, s2, n)           _strnicmp(s1, s2, n)
#   define sscanf(str, fmt, ...)            sscanf_s(str, fmt, ##__VA_ARGS__)
#   define COLOR_GREEN(str)                 str
#   define COLOR_RED(str)                   str
#   define COLOR_YELLO(str)                 str
#elif defined(__linux__)
#   include <sys/time.h>
#   include <pthread.h>
#   define GET_TID()                        ((unsigned long)pthread_self())
#   define COLOR_GREEN(str)                 "\033[32m" str "\033[0m"
#   define COLOR_RED(str)                   "\033[31m" str "\033[0m"
#   define COLOR_YELLO(str)                 "\033[33m" str "\033[0m"
#else
#   define GET_TID()                        0
#   define COLOR_GREEN(str)                 str
#   define COLOR_RED(str)                   str
#   define COLOR_YELLO(str)                 str
#endif

#define MASK_FAILURE                        (0x01 << 0x00)
#define MASK_SKIPPED                        (0x01 << 0x01)
#define SET_MASK(val, mask)                 do { (val) |= (mask); } while (0)
#define HAS_MASK(val, mask)                 ((val) & (mask))

#define ARRAY_SIZE(arr)                     (sizeof(arr) / sizeof(arr[0]))

/**
 * @brief microseconds in one second
 */
#define USEC_IN_SEC                         (1 * 1000 * 1000)

#define CONTAINER_OF(ptr, TYPE, member) \
    ((TYPE*)((char*)(ptr) - (size_t)&((TYPE*)0)->member))

typedef int test_bool;
#define test_true       1
#define test_false      0

#define MAX_FIXTURE_SIZE    31

typedef enum print_color
{
    print_default,
    print_red,
    print_green,
    print_yellow,
}print_color_t;

typedef union double_point
{
    double                  value_;
    uint64_t                bits_;
}double_point_t;

typedef union float_point
{
    float                   value_;
    uint32_t                bits_;
}float_point_t;

typedef enum test_case_stage
{
    stage_setup,
    stage_run,
    stage_teardown,
}test_case_stage_t;

typedef struct test_ctx
{
    struct
    {
        cunittest_list_t        case_list;                      /**< Cases in list */
        cunittest_map_t         case_table;                     /**< Cases in map */
        unsigned long           tid;                            /**< Thread ID */
    }info;

    struct
    {
        unsigned long long      seed;                           /**< Random seed */
        cunittest_list_node_t*  cur_it;                         /**< Current cursor position */
        cunittest_case_t*       cur_case;                       /**< Current running test case */
        unsigned                cur_parameterized_idx;          /**< Current parameterized index */
        test_case_stage_t       cur_stage;                      /**< Current running stage */
    }runtime;

    struct
    {
        cunittest_timestamp_t   tv_case_start;                  /**< Test start time */
        cunittest_timestamp_t   tv_case_end;                    /**< Test end time */

        cunittest_timestamp_t   tv_total_start;                 /**< The start time of whole test */
        cunittest_timestamp_t   tv_total_end;                   /**< The end time of whole test */

        cunittest_timestamp_t   tv_diff;                        /**< Time diff */
    }timestamp;

    struct
    {
        struct
        {
            unsigned            total;                          /**< The number of total running cases */
            unsigned            disabled;                       /**< The number of disabled cases */
            unsigned            success;                        /**< The number of successed cases */
            unsigned            skipped;                        /**< The number of skipped cases */
            unsigned            failed;                         /**< The number of failed cases */
        }result;

        struct
        {
            unsigned            repeat;                         /**< How many times need to repeat */
            unsigned            repeated;                       /**< How many times alread repeated */
        }repeat;
    }counter;

    struct
    {
        unsigned                break_on_failure : 1;           /**< DebugBreak when failure */
        unsigned                print_time : 1;                 /**< Whether to print execution cost time */
        unsigned                also_run_disabled_tests : 1;    /**< Also run disabled tests */
        unsigned                shuffle : 1;                    /**< Randomize running cases */
    }mask;

    struct
    {
        char**                  positive_patterns;              /**< positive patterns for filter */
        char**                  negative_patterns;              /**< negative patterns for filter */
        size_t                  n_negative;                     /**< The number of negative patterns */
        size_t                  n_postive;                      /**< The number of positive patterns */
    }filter;

    struct
    {
        size_t                  kMaxUlps;
        struct
        {
            size_t              kBitCount_64;
            size_t              kFractionBitCount_64;
            size_t              kExponentBitCount_64;
            uint64_t            kSignBitMask_64;
            uint64_t            kFractionBitMask_64;
            uint64_t            kExponentBitMask_64;
        }_double;
        struct
        {
            size_t              kBitCount_32;
            size_t              kFractionBitCount_32;
            size_t              kExponentBitCount_32;
            uint32_t            kSignBitMask_32;
            uint32_t            kFractionBitMask_32;
            uint32_t            kExponentBitMask_32;
        }_float;
    }precision;                                                 /**< Context for float/double compare */

    const cunittest_hook_t*     hook;
}test_ctx_t;

typedef struct test_ctx2
{
    char                        strbuf[128];                    /**< String buffer */
    jmp_buf                     jmpbuf;                         /**< Jump buffer */
}test_ctx2_t;

static int _test_on_cmp_case(const cunittest_map_node_t* key1, const cunittest_map_node_t* key2, void* arg);
static test_ctx2_t          g_test_ctx2;                                // no need to initialize
static test_ctx_t           g_test_ctx = {
    { { NULL, NULL, 0 }, { NULL, { _test_on_cmp_case, NULL }, 0 }, 0 }, // .info
    { 0, NULL, NULL, 0, stage_setup },                                  // .runtime
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },               // .timestamp
    { { 0, 0, 0, 0, 0 }, { 1, 0 } },                                    // .counter
    { 0, 1, 0, 0 },                                                     // .mask
    { NULL, NULL, 0, 0 },                                               // .filter
    { 4, { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },                  // .precision
    NULL,                                                               // .hook
};

#if defined(_MSC_VER)

static LARGE_INTEGER _test_get_file_time_offset(void)
{
    SYSTEMTIME s;
    FILETIME f;
    LARGE_INTEGER t;

    s.wYear = 1970;
    s.wMonth = 1;
    s.wDay = 1;
    s.wHour = 0;
    s.wMinute = 0;
    s.wSecond = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime(&s, &f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;

    return t;
}

// Returns the character attribute for the given color.
static WORD _test_get_color_attribute(print_color_t color)
{
    switch (color)
    {
    case print_red:
        return FOREGROUND_RED;
    case print_green:
        return FOREGROUND_GREEN;
    case print_yellow:
        return FOREGROUND_RED | FOREGROUND_GREEN;
    default:
        return 0;
    }
}

static int _test_get_bit_offset(WORD color_mask)
{
    if (color_mask == 0) return 0;

    int bitOffset = 0;
    while ((color_mask & 1) == 0)
    {
        color_mask >>= 1;
        ++bitOffset;
    }
    return bitOffset;
}

static WORD _test_get_new_color(print_color_t color, WORD old_color_attrs)
{
    // Let's reuse the BG
    static const WORD background_mask = BACKGROUND_BLUE | BACKGROUND_GREEN |
        BACKGROUND_RED | BACKGROUND_INTENSITY;
    static const WORD foreground_mask = FOREGROUND_BLUE | FOREGROUND_GREEN |
        FOREGROUND_RED | FOREGROUND_INTENSITY;
    const WORD existing_bg = old_color_attrs & background_mask;

    WORD new_color =
        _test_get_color_attribute(color) | existing_bg | FOREGROUND_INTENSITY;
    const int bg_bitOffset = _test_get_bit_offset(background_mask);
    const int fg_bitOffset = _test_get_bit_offset(foreground_mask);

    if (((new_color & background_mask) >> bg_bitOffset) ==
        ((new_color & foreground_mask) >> fg_bitOffset))
    {
        new_color ^= FOREGROUND_INTENSITY;  // invert intensity
    }
    return new_color;
}

#endif

static int _test_on_cmp_case(const cunittest_map_node_t* key1, const cunittest_map_node_t* key2, void* arg)
{
    (void)arg;
    const cunittest_case_t* t_case_1 = CONTAINER_OF(key1, cunittest_case_t, node.table);
    const cunittest_case_t* t_case_2 = CONTAINER_OF(key2, cunittest_case_t, node.table);

    int ret;
    if ((ret = strcmp(t_case_1->info.suit_name, t_case_2->info.suit_name)) != 0)
    {
        return ret;
    }
    return strcmp(t_case_1->info.case_name, t_case_2->info.case_name);
}

static void _test_srand(unsigned long long s)
{
    g_test_ctx.runtime.seed = s - 1;
}

static unsigned long _test_rand(void)
{
    g_test_ctx.runtime.seed = 6364136223846793005ULL * g_test_ctx.runtime.seed + 1;
    return g_test_ctx.runtime.seed >> 33;
}

/**
 * @brief Check if `str` match `pat`
 * @return bool
 */
static test_bool _test_pattern_matches_string(const char* pat, const char* str)
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

static test_bool _test_check_pattern(const char* str)
{
    size_t i;
    for (i = 0; i < g_test_ctx.filter.n_negative; i++)
    {
        if (_test_pattern_matches_string(g_test_ctx.filter.negative_patterns[i], str))
        {
            return test_false;
        }
    }

    if (g_test_ctx.filter.n_postive == 0)
    {
        return test_true;
    }

    for (i = 0; i < g_test_ctx.filter.n_postive; i++)
    {
        if (_test_pattern_matches_string(g_test_ctx.filter.positive_patterns[i], str))
        {
            return test_true;
        }
    }

    return test_false;
}

static test_bool _test_check_disable(const char* name)
{
    return !g_test_ctx.mask.also_run_disabled_tests && (strncmp("DISABLED_", name, 9) == 0);
}

static const char* _test_get_ansi_color_code(print_color_t color)
{
    switch (color)
    {
    case print_red:
        return "1";

    case print_green:
        return "2";

    case print_yellow:
        return "3";

    default:
        break;
    }

    return NULL;
}

static void _test_print_colorful(print_color_t color, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    if (color == print_default)
    {
        vprintf(fmt, args);
        goto fin;
    }

#if defined(_MSC_VER)
    const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    // Gets the current text color.
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
    const WORD old_color_attrs = buffer_info.wAttributes;
    const WORD new_color = _test_get_new_color(color, old_color_attrs);

    // We need to flush the stream buffers into the console before each
    // SetConsoleTextAttribute call lest it affect the text that is already
    // printed but has not yet reached the console.
    fflush(stdout);
    SetConsoleTextAttribute(stdout_handle, new_color);

    vprintf(fmt, args);

    fflush(stdout);
    // Restores the text color.
    SetConsoleTextAttribute(stdout_handle, old_color_attrs);
#else
    printf("\033[0;3%sm", _test_get_ansi_color_code(color));
    vprintf(fmt, args);
    printf("\033[m");
#endif

fin:
    va_end(args);
}

static void _test_hook_before_fixture_setup(cunittest_case_t* test_case)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_fixture_setup == NULL)
    {
        return;
    }
    g_test_ctx.hook->before_fixture_setup(test_case->info.suit_name);
}

static void _test_hook_after_fixture_setup(cunittest_case_t* test_case, int ret)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_fixture_setup == NULL)
    {
        return;
    }
    g_test_ctx.hook->after_fixture_setup(test_case->info.suit_name, ret);
}

static void _test_fixture_run_setup(void)
{
    if (g_test_ctx.runtime.cur_case->stage.setup == NULL)
    {
        return;
    }

    _test_hook_before_fixture_setup(g_test_ctx.runtime.cur_case);
    g_test_ctx.runtime.cur_case->stage.setup();
    _test_hook_after_fixture_setup(g_test_ctx.runtime.cur_case, 0);
}

static void _test_hook_before_normal_test(cunittest_case_t* test_case)
{
    switch (test_case->info.type)
    {
    case CUNITTEST_CASE_TYPE_SIMPLE:
        if (g_test_ctx.hook != NULL && g_test_ctx.hook->before_simple_test != NULL)
        {
            g_test_ctx.hook->before_simple_test(test_case->info.suit_name, test_case->info.case_name);
        }
        break;

    case CUNITTEST_CASE_TYPE_FIXTURE:
        if (g_test_ctx.hook != NULL && g_test_ctx.hook->before_fixture_test != NULL)
        {
            g_test_ctx.hook->before_fixture_test(test_case->info.suit_name, test_case->info.case_name);
        }
        break;

    default:
        ASSERT(0);
        break;
    }
}

static void _test_hook_after_normal_test(cunittest_case_t* test_case, int ret)
{
    switch (test_case->info.type)
    {
    case CUNITTEST_CASE_TYPE_SIMPLE:
        if (g_test_ctx.hook != NULL && g_test_ctx.hook->after_simple_test != NULL)
        {
            g_test_ctx.hook->after_simple_test(test_case->info.suit_name, test_case->info.case_name, ret);
        }
        break;

    case CUNITTEST_CASE_TYPE_FIXTURE:
        if (g_test_ctx.hook != NULL && g_test_ctx.hook->after_fixture_test != NULL)
        {
            g_test_ctx.hook->after_fixture_test(test_case->info.suit_name, test_case->info.case_name, ret);
        }
        break;

    default:
        ASSERT(0);
        break;
    }
}

static void _test_hook_before_parameterized_test(cunittest_case_t* test_case, unsigned index)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_parameterized_test == NULL)
    {
        return;
    }
    g_test_ctx.hook->before_parameterized_test(test_case->info.suit_name,
        test_case->info.case_name, index, (unsigned)test_case->stage.n_dat);
}

static void _test_hook_after_parameterized_test(cunittest_case_t* test_case, unsigned index, int ret)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_parameterized_test == NULL)
    {
        return;
    }
    g_test_ctx.hook->after_parameterized_test(test_case->info.suit_name,
        test_case->info.case_name, index, (unsigned)test_case->stage.n_dat, ret);
}

static void _test_fixture_run_body(void)
{
    if (g_test_ctx.runtime.cur_case->info.type != CUNITTEST_CASE_TYPE_PARAMETERIZED)
    {
        _test_hook_before_normal_test(g_test_ctx.runtime.cur_case);
        ((cunittest_procedure_fn)g_test_ctx.runtime.cur_case->stage.body)();
        _test_hook_after_normal_test(g_test_ctx.runtime.cur_case, 0);
    }
    else
    {
        for (g_test_ctx.runtime.cur_parameterized_idx = 0;
            g_test_ctx.runtime.cur_parameterized_idx < g_test_ctx.runtime.cur_case->stage.n_dat;
            g_test_ctx.runtime.cur_parameterized_idx++)
        {
            _test_hook_before_parameterized_test(g_test_ctx.runtime.cur_case, g_test_ctx.runtime.cur_parameterized_idx);
            ((cunittest_parameterized_fn)g_test_ctx.runtime.cur_case->stage.body)(g_test_ctx.runtime.cur_case->stage.p_dat);
            _test_hook_after_parameterized_test(g_test_ctx.runtime.cur_case, g_test_ctx.runtime.cur_parameterized_idx, -1);
        }
    }
}

static void _test_hook_before_fixture_teardown(cunittest_case_t* test_case)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_fixture_teardown == NULL)
    {
        return;
    }

    g_test_ctx.hook->before_fixture_teardown(test_case->info.suit_name);
}

static void _test_hook_after_fixture_teardown(cunittest_case_t* test_case, int ret)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_fixture_teardown == NULL)
    {
        return;
    }
    g_test_ctx.hook->after_fixture_teardown(test_case->info.suit_name, ret);
}

static void _test_hook_before_all_test(int argc, char* argv[])
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_all_test == NULL)
    {
        return;
    }
    g_test_ctx.hook->before_all_test(argc, argv);
}

static void _test_hook_after_all_test(void)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_all_test == NULL)
    {
        return;
    }
    g_test_ctx.hook->after_all_test();
}

static void _test_fixture_run_teardown(void)
{
    if (g_test_ctx.runtime.cur_case->stage.teardown == NULL)
    {
        return;
    }

    _test_hook_before_fixture_teardown(g_test_ctx.runtime.cur_case);
    g_test_ctx.runtime.cur_case->stage.teardown();
    _test_hook_after_fixture_teardown(g_test_ctx.runtime.cur_case, 0);
}

/**
* run test case.
* the target case was set to `g_test_ctx.runtime.cur_case`
*/
static void _test_run_case(void)
{
    /* reset resource */
    g_test_ctx.runtime.cur_parameterized_idx = 0;
    g_test_ctx.runtime.cur_case->info.mask = 0;

    snprintf(g_test_ctx2.strbuf, sizeof(g_test_ctx2.strbuf), "%s.%s",
        g_test_ctx.runtime.cur_case->info.suit_name,
        g_test_ctx.runtime.cur_case->info.case_name);

    /* Check if need to run this test case */
    if (g_test_ctx.filter.positive_patterns != NULL && !_test_check_pattern(g_test_ctx2.strbuf))
    {
        return;
    }
    g_test_ctx.counter.result.total++;

    /* check if this test is disabled */
    if (_test_check_disable(g_test_ctx.runtime.cur_case->info.case_name))
    {
        g_test_ctx.counter.result.disabled++;
        return;
    }

    _test_print_colorful(print_green, "[ RUN      ]");
    _test_print_colorful(print_default, " %s\n", g_test_ctx2.strbuf);

    int ret;
    if ((ret = setjmp(g_test_ctx2.jmpbuf)) != 0)
    {
        SET_MASK(g_test_ctx.runtime.cur_case->info.mask, ret);
        goto procedure_teardown;
    }

    /* setup */
    g_test_ctx.runtime.cur_stage = stage_setup;
    _test_fixture_run_setup();

    /* record start time */
    ASSERT(cunittest_timestamp_get(&g_test_ctx.timestamp.tv_case_start) == 0);

    /* run test case */
    g_test_ctx.runtime.cur_stage = stage_run;
    _test_fixture_run_body();

procedure_teardown:
    switch (g_test_ctx.runtime.cur_stage)
    {
    case stage_setup:
        _test_hook_after_fixture_setup(g_test_ctx.runtime.cur_case, -1);
        break;

    case stage_run:
        if (g_test_ctx.runtime.cur_case->info.type != CUNITTEST_CASE_TYPE_PARAMETERIZED)
        {
            _test_hook_after_normal_test(g_test_ctx.runtime.cur_case, -1);
        }
        else
        {
            _test_hook_after_parameterized_test(g_test_ctx.runtime.cur_case, g_test_ctx.runtime.cur_parameterized_idx, -1);
        }
        break;

    case stage_teardown:
        _test_hook_after_fixture_teardown(g_test_ctx.runtime.cur_case, -1);
        goto procedure_teardown_fin;

    default:
        ASSERT(0);
    }

    /* record end time */
    ASSERT(cunittest_timestamp_get(&g_test_ctx.timestamp.tv_case_end) == 0);

    /* teardown */
    g_test_ctx.runtime.cur_stage = stage_teardown;
    _test_fixture_run_teardown();

procedure_teardown_fin:
    cunittest_timestamp_dif(&g_test_ctx.timestamp.tv_case_start, &g_test_ctx.timestamp.tv_case_end, &g_test_ctx.timestamp.tv_diff);

    if (HAS_MASK(g_test_ctx.runtime.cur_case->info.mask, MASK_FAILURE))
    {
        g_test_ctx.counter.result.failed++;
        _test_print_colorful(print_red, "[  FAILED  ]");
    }
    else if (HAS_MASK(g_test_ctx.runtime.cur_case->info.mask, MASK_SKIPPED))
    {
        g_test_ctx.counter.result.skipped++;
        _test_print_colorful(print_yellow, "[   SKIP   ]");
    }
    else
    {
        g_test_ctx.counter.result.success++;
        _test_print_colorful(print_green, "[       OK ]");
    }

    printf(" %s", g_test_ctx2.strbuf);
    if (g_test_ctx.mask.print_time)
    {
        unsigned long take_time = (unsigned long)(g_test_ctx.timestamp.tv_diff.sec * 1000
            + g_test_ctx.timestamp.tv_diff.usec / 1000);
        printf(" (%lu ms)", take_time);
    }
    printf("\n");
}

static void _test_reset_all_test(void)
{
    memset(&g_test_ctx.counter.result, 0, sizeof(g_test_ctx.counter.result));
    memset(&g_test_ctx.timestamp, 0, sizeof(g_test_ctx.timestamp));

    cunittest_list_node_t* it = cunittest_list_begin(&g_test_ctx.info.case_list);
    for (; it != NULL; it = cunittest_list_next(it))
    {
        cunittest_case_t* case_data = CONTAINER_OF(it, cunittest_case_t, node.queue);
        case_data->info.mask = 0;
    }

    g_test_ctx.info.tid = GET_TID();
}

static void _test_show_report_failed(void)
{
    cunittest_list_node_t* it = cunittest_list_begin(&g_test_ctx.info.case_list);
    for (; it != NULL; it = cunittest_list_next(it))
    {
        cunittest_case_t* case_data = CONTAINER_OF(it, cunittest_case_t, node.queue);
        if (!HAS_MASK(case_data->info.mask, MASK_FAILURE))
        {
            continue;
        }

        snprintf(g_test_ctx2.strbuf, sizeof(g_test_ctx2.strbuf), "%s.%s",
            case_data->info.suit_name, case_data->info.case_name);

        _test_print_colorful(print_red, "[  FAILED  ]");
        printf(" %s\n", g_test_ctx2.strbuf);
    }
}

static void _test_show_report(void)
{
    cunittest_timestamp_dif(&g_test_ctx.timestamp.tv_total_start,
        &g_test_ctx.timestamp.tv_total_end, &g_test_ctx.timestamp.tv_diff);

    _test_print_colorful(print_green, "[==========]");
    printf(" %u/%u test case%s ran.",
        g_test_ctx.counter.result.total,
        (unsigned)cunittest_list_size(&g_test_ctx.info.case_list),
        g_test_ctx.counter.result.total > 1 ? "s" : "");
    if (g_test_ctx.mask.print_time)
    {
        unsigned long take_time = (unsigned long)(g_test_ctx.timestamp.tv_diff.sec * 1000
            + g_test_ctx.timestamp.tv_diff.usec / 1000);
        printf(" (%lu ms total)", take_time);
    }
    printf("\n");

    if (g_test_ctx.counter.result.disabled != 0)
    {
        _test_print_colorful(print_green, "[ DISABLED ]");
        printf(" %u test%s.\n",
            g_test_ctx.counter.result.disabled,
            g_test_ctx.counter.result.disabled > 1 ? "s" : "");
    }
    if (g_test_ctx.counter.result.skipped != 0)
    {
        _test_print_colorful(print_yellow, "[ BYPASSED ]");
        printf(" %u test%s.\n",
            g_test_ctx.counter.result.skipped,
            g_test_ctx.counter.result.skipped > 1 ? "s" : "");
    }
    if (g_test_ctx.counter.result.success != 0)
    {
        _test_print_colorful(print_green, "[  PASSED  ]");
        printf(" %u test%s.\n",
            g_test_ctx.counter.result.success,
            g_test_ctx.counter.result.success > 1 ? "s" : "");
    }

    /* don't show failed tests if every test was success */
    if (g_test_ctx.counter.result.failed == 0)
    {
        return;
    }

    _test_print_colorful(print_red, "[  FAILED  ]");
    printf(" %u test%s, listed below:\n", g_test_ctx.counter.result.failed, g_test_ctx.counter.result.failed > 1 ? "s" : "");
    _test_show_report_failed();
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
    g_test_ctx.filter.positive_patterns = malloc(malloc_size);
    if (g_test_ctx.filter.positive_patterns == NULL)
    {
        return;
    }
    g_test_ctx.filter.negative_patterns = g_test_ctx.filter.positive_patterns + (number_of_patterns - number_of_negative);
    memcpy((char*)g_test_ctx.filter.positive_patterns + prefix_size, user_pattern, user_pattern_size);

    char* str_it = (char*)g_test_ctx.filter.positive_patterns + prefix_size;
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

            g_test_ctx.filter.negative_patterns[g_test_ctx.filter.n_negative] = str_it;
            g_test_ctx.filter.n_negative++;
            continue;
        }

        g_test_ctx.filter.positive_patterns[g_test_ctx.filter.n_postive] = str_it;
        g_test_ctx.filter.n_postive++;
    } while ((str_it = strchr(str_it + 1, ':')) != NULL);
}

static unsigned _test_calculate_max_class_length(unsigned* number_of_fixture)
{
    size_t tmp_len;
    size_t max_length = 0;
    unsigned cnt_fixture = 0;
    const char* last_class_name = "";

    cunittest_map_node_t* it = cunittest_map_begin(&g_test_ctx.info.case_table);
    for (; it != NULL; it = cunittest_map_next(it))
    {
        cunittest_case_t* case_data = CONTAINER_OF(it, cunittest_case_t, node.table);
        if (last_class_name == case_data->info.suit_name
            || strcmp(last_class_name, case_data->info.suit_name) == 0)
        {
            continue;
        }

        last_class_name = case_data->info.suit_name;
        if ((tmp_len = strlen(last_class_name)) > max_length)
        {
            max_length = tmp_len;
        }
        cnt_fixture++;
    }

    *number_of_fixture = cnt_fixture;
    return (unsigned)max_length;
}

static void _test_list_tests(void)
{
    const char* last_class_name = "";
    const char* print_class_name = "";

    unsigned cnt_fixture = 0;
    int cnt_test = (int)cunittest_map_size(&g_test_ctx.info.case_table);
    int max_fixture_length = (int)_test_calculate_max_class_length(&cnt_fixture);
    if (max_fixture_length > MAX_FIXTURE_SIZE)
    {
        max_fixture_length = MAX_FIXTURE_SIZE;
    }

    /* generate fixture info */
    snprintf(g_test_ctx2.strbuf, sizeof(g_test_ctx2.strbuf), "%u fixture%s", cnt_fixture, cnt_fixture > 1 ? "s" : "");
    int fixture_length = (int)strlen(g_test_ctx2.strbuf);
    if (fixture_length > MAX_FIXTURE_SIZE)
    {
        fixture_length = MAX_FIXTURE_SIZE;
    }

    /* calculate fixture length and case length */
    if (max_fixture_length > fixture_length)
    {
        fixture_length = max_fixture_length;
    }
    unsigned item_length = 80 - fixture_length - 4;

    printf("===============================================================================\n");
    printf("%*.*s | case item\n", fixture_length, fixture_length, "fixture");
    printf("-------------------------------------------------------------------------------\n");

    cunittest_map_node_t* it = cunittest_map_begin(&g_test_ctx.info.case_table);
    for (; it != NULL; it = cunittest_map_next(it))
    {
        cunittest_case_t* case_data = CONTAINER_OF(it, cunittest_case_t, node.table);
        /* some compiler will make same string with different address */
        if (last_class_name != case_data->info.suit_name
            && strcmp(last_class_name, case_data->info.suit_name) != 0)
        {
            last_class_name = case_data->info.suit_name;
            print_class_name = last_class_name;
        }

        printf("%*.*s | %-.*s\n", fixture_length, fixture_length, print_class_name, item_length, case_data->info.case_name);
        print_class_name = "";
    }

    printf("-------------------------------------------------------------------------------\n");
    printf("%*.*s | %u case item%s\n", fixture_length, fixture_length, g_test_ctx2.strbuf,
        cnt_test, cnt_test > 1 ? "s" : "");
    printf("===============================================================================\n");
}

static void _test_setup_arg_repeat(const char* str)
{
    int repeat = 1;
    sscanf(str, "%d", &repeat);
    g_test_ctx.counter.repeat.repeat = (unsigned)repeat;
}

static void _test_setup_arg_print_time(const char* str)
{
    int val = 1;
    sscanf(str, "%d", &val);
    g_test_ctx.mask.print_time = !!val;
}

static void _test_setup_arg_random_seed(const char* str)
{
    long long val = time(NULL);
    sscanf(str, "%lld", &val);

    _test_srand(val);
}

static void _test_setup_precision(void)
{
    assert(sizeof(((double_point_t*)NULL)->bits_) == sizeof(((double_point_t*)NULL)->value_));
    assert(sizeof(((float_point_t*)NULL)->bits_) == sizeof(((float_point_t*)NULL)->value_));

    // double
    {
        g_test_ctx.precision._double.kBitCount_64 = 8 * sizeof(((double_point_t*)NULL)->value_);
        g_test_ctx.precision._double.kSignBitMask_64 = (uint64_t)1 << (g_test_ctx.precision._double.kBitCount_64 - 1);
        g_test_ctx.precision._double.kFractionBitCount_64 = DBL_MANT_DIG - 1;
        g_test_ctx.precision._double.kExponentBitCount_64 = g_test_ctx.precision._double.kBitCount_64 - 1 - g_test_ctx.precision._double.kFractionBitCount_64;
        g_test_ctx.precision._double.kFractionBitMask_64 = (~(uint64_t)0) >> (g_test_ctx.precision._double.kExponentBitCount_64 + 1);
        g_test_ctx.precision._double.kExponentBitMask_64 = ~(g_test_ctx.precision._double.kSignBitMask_64 | g_test_ctx.precision._double.kFractionBitMask_64);
    }

    // float
    {
        g_test_ctx.precision._float.kBitCount_32 = 8 * sizeof(((float_point_t*)NULL)->value_);
        g_test_ctx.precision._float.kSignBitMask_32 = (uint32_t)1 << (g_test_ctx.precision._float.kBitCount_32 - 1);
        g_test_ctx.precision._float.kFractionBitCount_32 = FLT_MANT_DIG - 1;
        g_test_ctx.precision._float.kExponentBitCount_32 = g_test_ctx.precision._float.kBitCount_32 - 1 - g_test_ctx.precision._float.kFractionBitCount_32;
        g_test_ctx.precision._float.kFractionBitMask_32 = (~(uint32_t)0) >> (g_test_ctx.precision._float.kExponentBitCount_32 + 1);
        g_test_ctx.precision._float.kExponentBitMask_32 = ~(g_test_ctx.precision._float.kSignBitMask_32 | g_test_ctx.precision._float.kFractionBitMask_32);
    }
}

static void _test_shuffle_cases(void)
{
    cunittest_list_t copy_case_list = TEST_LIST_INITIALIZER;

    while (cunittest_list_size(&g_test_ctx.info.case_list) != 0)
    {
        unsigned idx = _test_rand() % cunittest_list_size(&g_test_ctx.info.case_list);

        unsigned i = 0;
        cunittest_list_node_t* it = cunittest_list_begin(&g_test_ctx.info.case_list);
        for (; i < idx; i++, it = cunittest_list_next(it))
        {
        }

        cunittest_list_erase(&g_test_ctx.info.case_list, it);
        cunittest_list_push_back(&copy_case_list, it);
    }

    g_test_ctx.info.case_list = copy_case_list;
}

static int _test_setup(int argc, char* argv[], const cunittest_hook_t* hook)
{
    (void)argc;
    enum test_opt
    {
        ctest_list_tests = 1,
        ctest_filter,
        ctest_also_run_disabled_tests,
        ctest_repeat,
        ctest_shuffle,
        ctest_random_seed,
        ctest_print_time,
        ctest_break_on_failure,
        help,
    };

    test_optparse_long_opt_t longopts[] = {
        { "ctest_list_tests",               ctest_list_tests,               OPTPARSE_OPTIONAL },
        { "ctest_filter",                   ctest_filter,                   OPTPARSE_OPTIONAL },
        { "ctest_also_run_disabled_tests",  ctest_also_run_disabled_tests,  OPTPARSE_OPTIONAL },
        { "ctest_repeat",                   ctest_repeat,                   OPTPARSE_OPTIONAL },
        { "ctest_shuffle",                  ctest_shuffle,                  OPTPARSE_OPTIONAL },
        { "ctest_random_seed",              ctest_random_seed,              OPTPARSE_OPTIONAL },
        { "ctest_print_time",               ctest_print_time,               OPTPARSE_OPTIONAL },
        { "ctest_break_on_failure",         ctest_break_on_failure,         OPTPARSE_OPTIONAL },
        { "help",                           help,                           OPTPARSE_OPTIONAL },
        { 0,                                0,                              OPTPARSE_NONE },
    };

    test_optparse_t options;
    test_optparse_init(&options, argv);

    int option;
    while ((option = test_optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
        case ctest_list_tests:
            _test_list_tests();
            return -1;
        case ctest_filter:
            _test_setup_arg_pattern(options.optarg);
            break;
        case ctest_also_run_disabled_tests:
            g_test_ctx.mask.also_run_disabled_tests = 1;
            break;
        case ctest_repeat:
            _test_setup_arg_repeat(options.optarg);
            break;
        case ctest_shuffle:
            g_test_ctx.mask.shuffle = 1;
            break;
        case ctest_random_seed:
            _test_setup_arg_random_seed(options.optarg);
            break;
        case ctest_print_time:
            _test_setup_arg_print_time(options.optarg);
            break;
        case ctest_break_on_failure:
            g_test_ctx.mask.break_on_failure = 1;
            break;
        case help:
            printf(
                "This program contains tests written using Test. You can use the\n"
                "following command line flags to control its behavior:\n"
                "\n"
                "Test Selection:\n"
                "  "COLOR_GREEN("--ctest_list_tests")"\n"
                "      List the names of all tests instead of running them. The name of\n"
                "      TEST(Foo, Bar) is \"Foo.Bar\".\n"
                "  "COLOR_GREEN("--ctest_filter=") COLOR_YELLO("POSTIVE_PATTERNS[") COLOR_GREEN("-") COLOR_YELLO("NEGATIVE_PATTERNS]")"\n"
                "      Run only the tests whose name matches one of the positive patterns but\n"
                "      none of the negative patterns. '?' matches any single character; '*'\n"
                "      matches any substring; ':' separates two patterns.\n"
                "  "COLOR_GREEN("--ctest_also_run_disabled_tests")"\n"
                "      Run all disabled tests too.\n"
                "\n"
                "Test Execution:\n"
                "  "COLOR_GREEN("--ctest_repeat=")COLOR_YELLO("[COUNT]")"\n"
                "      Run the tests repeatedly; use a negative count to repeat forever.\n"
                "  "COLOR_GREEN("--ctest_shuffle")"\n"
                "      Randomize tests' orders on every iteration.\n"
                "  "COLOR_GREEN("--ctest_random_seed=") COLOR_YELLO("[NUMBER]") "\n"
                "      Random number seed to use for shuffling test orders (between 0 and\n"
                "      99999. By default a seed based on the current time is used for shuffle).\n"
                "\n"
                "Test Output:\n"
                "  "COLOR_GREEN("--ctest_print_time=") COLOR_YELLO("(") COLOR_GREEN("0") COLOR_YELLO("|") COLOR_GREEN("1") COLOR_YELLO(")") "\n"
                "      Don't print the elapsed time of each test.\n"
                "\n"
                "Assertion Behavior:\n"
                "  "COLOR_GREEN("--ctest_break_on_failure")"\n"
                "      Turn assertion failures into debugger break-points.\n"
                );
            return -1;
        default:
            break;
        }
    }

    _test_setup_precision();

    /* shuffle if necessary */
    if (g_test_ctx.mask.shuffle)
    {
        _test_shuffle_cases();
    }
    g_test_ctx.hook = hook;

    return 0;
}

static void _test_run_test_loop(void)
{
    _test_reset_all_test();

    _test_print_colorful(print_yellow, "[==========]");
    printf(" total %u test%s registered.\n",
        (unsigned)cunittest_list_size(&g_test_ctx.info.case_list),
        cunittest_list_size(&g_test_ctx.info.case_list) > 1 ? "s" : "");

    cunittest_timestamp_get(&g_test_ctx.timestamp.tv_total_start);

    g_test_ctx.runtime.cur_it = cunittest_list_begin(&g_test_ctx.info.case_list);
    for (; g_test_ctx.runtime.cur_it != NULL;
        g_test_ctx.runtime.cur_it = cunittest_list_next(g_test_ctx.runtime.cur_it))
    {
        g_test_ctx.runtime.cur_case = CONTAINER_OF(g_test_ctx.runtime.cur_it, cunittest_case_t, node.queue);
        _test_run_case();
    }

    cunittest_timestamp_get(&g_test_ctx.timestamp.tv_total_end);

    _test_show_report();
}

static uint32_t _test_float_point_exponent_bits(const float_point_t* p)
{
    return g_test_ctx.precision._float.kExponentBitMask_32 & p->bits_;
}

static uint64_t _test_double_point_exponent_bits(const double_point_t* p)
{
    return g_test_ctx.precision._double.kExponentBitMask_64 & p->bits_;
}

static uint32_t _test_float_point_fraction_bits(const float_point_t* p)
{
    return g_test_ctx.precision._float.kFractionBitMask_32 & p->bits_;
}

static uint64_t _test_double_point_fraction_bits(const double_point_t* p)
{
    return g_test_ctx.precision._double.kFractionBitMask_64 & p->bits_;
}

static int _test_float_point_is_nan(const float_point_t* p)
{
    return (_test_float_point_exponent_bits(p) == g_test_ctx.precision._float.kExponentBitMask_32)
        && (_test_float_point_fraction_bits(p) != 0);
}

static int _test_double_point_is_nan(const double_point_t* p)
{
    return (_test_double_point_exponent_bits(p) == g_test_ctx.precision._double.kExponentBitMask_64)
        && (_test_double_point_fraction_bits(p) != 0);
}

static uint32_t _test_float_point_sign_and_magnitude_to_biased(const uint32_t sam)
{
    if (g_test_ctx.precision._float.kSignBitMask_32 & sam)
    {
        // sam represents a negative number.
        return ~sam + 1;
    }

    // sam represents a positive number.
    return g_test_ctx.precision._float.kSignBitMask_32 | sam;
}

static uint64_t _test_double_point_sign_and_magnitude_to_biased(const uint64_t sam)
{
    if (g_test_ctx.precision._double.kSignBitMask_64 & sam)
    {
        // sam represents a negative number.
        return ~sam + 1;
    }

    // sam represents a positive number.
    return g_test_ctx.precision._double.kSignBitMask_64 | sam;
}

static uint32_t _test_float_point_distance_between_sign_and_magnitude_numbers(uint32_t sam1, uint32_t sam2)
{
    const uint32_t biased1 = _test_float_point_sign_and_magnitude_to_biased(sam1);
    const uint32_t biased2 = _test_float_point_sign_and_magnitude_to_biased(sam2);

    return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
}

static uint64_t _test_double_point_distance_between_sign_and_magnitude_numbers(uint64_t sam1, uint64_t sam2)
{
    const uint64_t biased1 = _test_double_point_sign_and_magnitude_to_biased(sam1);
    const uint64_t biased2 = _test_double_point_sign_and_magnitude_to_biased(sam2);

    return (biased1 >= biased2) ? (biased1 - biased2) : (biased2 - biased1);
}

int cunittest_timestamp_get(cunittest_timestamp_t* ts)
{
#if defined(_MSC_VER)

    LARGE_INTEGER           t;
    FILETIME            f;
    double                  microseconds;
    static LARGE_INTEGER    offset;
    static double           frequencyToMicroseconds;
    static int              initialized = 0;
    static BOOL             usePerformanceCounter = 0;

    if (!initialized)
    {
        LARGE_INTEGER performanceFrequency;
        initialized = 1;
        usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
        if (usePerformanceCounter)
        {
            QueryPerformanceCounter(&offset);
            frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
        }
        else
        {
            offset = _test_get_file_time_offset();
            frequencyToMicroseconds = 10.;
        }
    }
    if (usePerformanceCounter)
    {
        QueryPerformanceCounter(&t);
    }
    else
    {
        GetSystemTimeAsFileTime(&f);
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double)t.QuadPart / frequencyToMicroseconds;
    t.QuadPart = (LONGLONG)microseconds;
    ts->sec = t.QuadPart / 1000000;
    ts->usec = t.QuadPart % 1000000;
    return 0;

#else
    struct timespec tmp_ts;
    if (clock_gettime(CLOCK_MONOTONIC, &tmp_ts) < 0)
    {
        return -1;
    }

    ts->sec = tmp_ts.tv_sec;
    ts->usec = tmp_ts.tv_nsec / 1000;
    return 0;
#endif
}

int cunittest_timestamp_dif(const cunittest_timestamp_t* t1, const cunittest_timestamp_t* t2, cunittest_timestamp_t* dif)
{
    cunittest_timestamp_t tmp_dif;
    const cunittest_timestamp_t* large_t = t1->sec > t2->sec ? t1 : (t1->sec < t2->sec ? t2 : (t1->usec > t2->usec ? t1 : t2));
    const cunittest_timestamp_t* little_t = large_t == t1 ? t2 : t1;

    tmp_dif.sec = large_t->sec - little_t->sec;
    if (large_t->usec < little_t->usec)
    {
        tmp_dif.usec = little_t->usec - large_t->usec;
        tmp_dif.sec--;
    }
    else
    {
        tmp_dif.usec = large_t->usec - little_t->usec;
    }

    if (dif != NULL)
    {
        *dif = tmp_dif;
    }

    if (tmp_dif.sec == 0 && tmp_dif.usec == 0)
    {
        return 0;
    }
    return t1 == little_t ? -1 : 1;
}

void cunittest_register_case(cunittest_case_t* data)
{
    ASSERT(cunittest_map_insert(&g_test_ctx.info.case_table, &data->node.table) == 0);
    cunittest_list_push_back(&g_test_ctx.info.case_list, &data->node.queue);
}

int cunittest_run_tests(int argc, char* argv[], const cunittest_hook_t* hook)
{
    /* Initialize random seed */
    _test_srand(time(NULL));

    /* Parser parameter */
    if (_test_setup(argc, argv, hook) < 0)
    {
        goto fin;
    }

    _test_hook_before_all_test(argc, argv);

    for (g_test_ctx.counter.repeat.repeated = 0;
        g_test_ctx.counter.repeat.repeated < g_test_ctx.counter.repeat.repeat;
        g_test_ctx.counter.repeat.repeated++)
    {
        if (g_test_ctx.counter.repeat.repeat > 1)
        {
            _test_print_colorful(print_yellow, "[==========]");
            printf(" start loop: %u/%u\n",
                g_test_ctx.counter.repeat.repeated + 1, g_test_ctx.counter.repeat.repeat);
        }

        _test_run_test_loop();

        if (g_test_ctx.counter.repeat.repeat > 1)
        {
            _test_print_colorful(print_yellow, "[==========]");
            printf(" end loop (%u/%u)\n",
                g_test_ctx.counter.repeat.repeated + 1, g_test_ctx.counter.repeat.repeat);
            if (g_test_ctx.counter.repeat.repeated < g_test_ctx.counter.repeat.repeat - 1)
            {
                printf("\n");
            }
        }
    }

    /* Cleanup */
fin:
    if (g_test_ctx.filter.positive_patterns != NULL)
    {
        free(g_test_ctx.filter.positive_patterns);
        memset(&g_test_ctx.filter, 0, sizeof(g_test_ctx.filter));
    }

    _test_hook_after_all_test();

    return (int)g_test_ctx.counter.result.failed;
}

TEST_NORETURN
void cunittest_unwrap_assert_fail(const char *expr, const char *file, int line, const char *func)
{
    fprintf(stderr, "Assertion failed: %s (%s: %s: %d)\n", expr, file, func, line);
    fflush(NULL);
    abort();
}

const char* cunittest_get_current_suit_name(void)
{
    if (g_test_ctx.runtime.cur_case == NULL)
    {
        return NULL;
    }
    return g_test_ctx.runtime.cur_case->info.suit_name;
}

const char* cunittest_get_current_case_name(void)
{
    if (g_test_ctx.runtime.cur_case == NULL)
    {
        return NULL;
    }
    return g_test_ctx.runtime.cur_case->info.case_name;
}

int cunittest_internal_assert_helper_str_eq(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}

int cunittest_internal_assert_helper_float_eq(float a, float b)
{
    float_point_t v_a; v_a.value_ = a;
    float_point_t v_b; v_b.value_ = b;

    if (_test_float_point_is_nan(&v_a) || _test_float_point_is_nan(&v_b))
    {
        return 0;
    }

    return _test_float_point_distance_between_sign_and_magnitude_numbers(v_a.bits_, v_b.bits_)
        <= g_test_ctx.precision.kMaxUlps;
}

int cunittest_internal_assert_helper_float_le(float a, float b)
{
    return (a < b) || cunittest_internal_assert_helper_float_eq(a, b);
}

int cunittest_internal_assert_helper_float_ge(float a, float b)
{
    return (a > b) || cunittest_internal_assert_helper_float_eq(a, b);
}

int cunittest_internal_assert_helper_double_eq(double a, double b)
{
    double_point_t v_a; v_a.value_ = a;
    double_point_t v_b; v_b.value_ = b;

    if (_test_double_point_is_nan(&v_a) || _test_double_point_is_nan(&v_b))
    {
        return 0;
    }

    return _test_double_point_distance_between_sign_and_magnitude_numbers(v_a.bits_, v_b.bits_)
        <= g_test_ctx.precision.kMaxUlps;
}

int cunittest_internal_assert_helper_double_le(double a, double b)
{
    return (a < b) || cunittest_internal_assert_helper_double_eq(a, b);
}

int cunittest_internal_assert_helper_double_ge(double a, double b)
{
    return (a > b) || cunittest_internal_assert_helper_double_eq(a, b);
}

unsigned cunittest_internal_parameterized_index(void)
{
    return g_test_ctx.runtime.cur_parameterized_idx;
}

TEST_NORETURN
void cunittest_internal_assert_failure(void)
{
    if (g_test_ctx.info.tid != GET_TID())
    {
        /*
        * If current thread is NOT the main thread, it is dangerous to jump back
        * to caller stack, so we just abort the program.
        */
        abort();
    }
    else
    {
        ASSERT(g_test_ctx.runtime.cur_stage != stage_teardown);
        longjmp(g_test_ctx2.jmpbuf, MASK_FAILURE);
    }
}

void cunittest_skip_test(void)
{
    ASSERT(g_test_ctx.runtime.cur_stage == stage_setup);
    SET_MASK(g_test_ctx.runtime.cur_case->info.mask, MASK_SKIPPED);
}

void cunittest_internal_flush(void)
{
    fflush(NULL);
}

int cunittest_internal_break_on_failure(void)
{
    return g_test_ctx.mask.break_on_failure;
}

/************************************************************************/
/* LOG                                                                  */
/************************************************************************/

const char* cunittest_pretty_file(const char* file)
{
    const char* pos = file;

    for (; *file; ++file)
    {
        if (*file == '\\' || *file == '/')
        {
            pos = file + 1;
        }
    }
    return pos;
}
