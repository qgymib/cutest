#ifndef _WIN32_WINNT
#   define _WIN32_WINNT   0x0600
#endif

#include "cutest.h"
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <float.h>

#if defined(_MSC_VER)
#   include <windows.h>
#   include <time.h>
#   include <io.h>
#   define isatty(x)                        _isatty(x)
#   define fileno(x)                        _fileno(x)
#   define GET_TID()                        ((unsigned long)GetCurrentThreadId())
#   define snprintf(str, size, fmt, ...)    _snprintf_s(str, size, _TRUNCATE, fmt, ##__VA_ARGS__)
#   ifndef strdup
#       define strdup(str)                  _strdup(str)
#   endif
#   define strncasecmp(s1, s2, n)           _strnicmp(s1, s2, n)
#   define sscanf(str, fmt, ...)            sscanf_s(str, fmt, ##__VA_ARGS__)
#   define strerror_r(errnum, buf, buflen)  strerror_s(buf, buflen, errnum)
#elif defined(__linux__)
#   include <sys/time.h>
#   include <unistd.h>
#   include <pthread.h>
#   define GET_TID()                        ((unsigned long)pthread_self())
#else
#   define GET_TID()                        0
#endif

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

#define ARRAY_SIZE(arr)                     (sizeof(arr) / sizeof(arr[0]))
/*
 * Before Visual Studio 2015, there is a bug that a `do { } while (0)` will triger C4127 warning
 * https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-4-c4127
 */
#if defined(_MSC_VER) && _MSC_VER < 1900
#   pragma warning(disable : 4127)
#endif

///////////////////////////////////////////////////////////////////////////////
// Map
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Map initializer helper
 * @param [in] fn       Compare function
 * @param [in] arg      User defined argument
 */
#define CUTEST_MAP_INIT(fn, arg)      { NULL, { fn, arg }, 0 }

#define RB_RED              0
#define RB_BLACK            1
#define __rb_color(pc)      ((uintptr_t)(pc) & 1)
#define __rb_is_black(pc)   __rb_color(pc)
#define __rb_is_red(pc)     (!__rb_color(pc))
#define __rb_parent(pc)     ((cutest_map_node_t*)(pc & ~3))
#define rb_color(rb)        __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb)       __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb)     __rb_is_black((rb)->__rb_parent_color)
#define rb_parent(r)        ((cutest_map_node_t*)((uintptr_t)((r)->__rb_parent_color) & ~3))

/* 'empty' nodes are nodes that are known not to be inserted in an rbtree */
#define RB_EMPTY_NODE(node)  \
    ((node)->__rb_parent_color == (cutest_map_node_t*)(node))

static void _etest_map_link_node(cutest_map_node_t* node, cutest_map_node_t* parent, cutest_map_node_t** rb_link)
{
    node->__rb_parent_color = parent;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
    return;
}

static void rb_set_black(cutest_map_node_t* rb)
{
    rb->__rb_parent_color =
        (cutest_map_node_t*)((uintptr_t)(rb->__rb_parent_color) | RB_BLACK);
}

static cutest_map_node_t* rb_red_parent(cutest_map_node_t* red)
{
    return red->__rb_parent_color;
}

static void rb_set_parent_color(cutest_map_node_t* rb, cutest_map_node_t* p, int color)
{
    rb->__rb_parent_color = (cutest_map_node_t*)((uintptr_t)p | color);
}

static void __rb_change_child(cutest_map_node_t* old_node, cutest_map_node_t* new_node,
    cutest_map_node_t* parent, cutest_map_t* root)
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
static void __rb_rotate_set_parents(cutest_map_node_t* old, cutest_map_node_t* new_node,
    cutest_map_t* root, int color)
{
    cutest_map_node_t* parent = rb_parent(old);
    new_node->__rb_parent_color = old->__rb_parent_color;
    rb_set_parent_color(old, new_node, color);
    __rb_change_child(old, new_node, parent, root);
}

static void _etest_map_insert(cutest_map_node_t* node, cutest_map_t* root)
{
    cutest_map_node_t* parent = rb_red_parent(node), * gparent, * tmp;

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

static void _etest_map_insert_color(cutest_map_node_t* node, cutest_map_t* root)
{
    _etest_map_insert(node, root);
}

/**
 * @brief Insert the node into map.
 * @warning the node must not exist in any map.
 * @param [in] handler  The pointer to the map
 * @param [in] node     The node
 * @return              0 if success, -1 otherwise
 */
static int cutest_map_insert(cutest_map_t* handler, cutest_map_node_t* node)
{
    cutest_map_node_t** new_node = &(handler->rb_root), * parent = NULL;

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

/**
 * @brief Returns an iterator to the beginning
 * @param [in] handler  The pointer to the map
 * @return              An iterator
 */
static cutest_map_node_t* cutest_map_begin(const cutest_map_t* handler)
{
    cutest_map_node_t* n = handler->rb_root;

    if (!n)
        return NULL;
    while (n->rb_left)
        n = n->rb_left;
    return n;
}

/**
 * @brief Get an iterator next to the given one.
 * @param [in] node     Current iterator
 * @return              Next iterator
 */
static cutest_map_node_t* cutest_map_next(const cutest_map_node_t* node)
{
    cutest_map_node_t* parent;

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
        return (cutest_map_node_t*)node;
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

static void rb_set_parent(cutest_map_node_t* rb, cutest_map_node_t* p)
{
    rb->__rb_parent_color = (cutest_map_node_t*)(rb_color(rb) | (uintptr_t)p);
}

static cutest_map_node_t* __rb_erase_augmented(cutest_map_node_t* node, cutest_map_t* root)
{
    cutest_map_node_t* child = node->rb_right, *tmp = node->rb_left;
    cutest_map_node_t* parent, * rebalance;
    uintptr_t pc;

    if (!tmp) {
        /*
        * Case 1: node to erase has no more than 1 child (easy!)
        *
        * Note that if there is one child it must be red due to 5)
        * and node must be black due to 4). We adjust colors locally
        * so as to bypass __rb_erase_color() later on.
        */
        pc = (uintptr_t)(node->__rb_parent_color);
        parent = __rb_parent(pc);
        __rb_change_child(node, child, parent, root);
        if (child) {
            child->__rb_parent_color = (cutest_map_node_t*)pc;
            rebalance = NULL;
        }
        else
            rebalance = __rb_is_black(pc) ? parent : NULL;
        tmp = parent;
    }
    else if (!child) {
        /* Still case 1, but this time the child is node->rb_left */
        pc = (uintptr_t)(node->__rb_parent_color);
        tmp->__rb_parent_color = (cutest_map_node_t*)pc;
        parent = __rb_parent(pc);
        __rb_change_child(node, tmp, parent, root);
        rebalance = NULL;
        tmp = parent;
    }
    else {
        cutest_map_node_t* successor = child, * child2;
        tmp = child->rb_left;
        if (!tmp) {
            /*
            * Case 2: node's successor is its right child
            *
            *    (n)          (s)
            *    / \          / \
            *  (x) (s)  ->  (x) (c)
            *        \
            *        (c)
            */
            parent = successor;
            child2 = successor->rb_right;
        }
        else {
            /*
            * Case 3: node's successor is leftmost under
            * node's right child subtree
            *
            *    (n)          (s)
            *    / \          / \
            *  (x) (y)  ->  (x) (y)
            *      /            /
            *    (p)          (p)
            *    /            /
            *  (s)          (c)
            *    \
            *    (c)
            */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->rb_left;
            } while (tmp);
            parent->rb_left = child2 = successor->rb_right;
            successor->rb_right = child;
            rb_set_parent(child, successor);
        }

        successor->rb_left = tmp = node->rb_left;
        rb_set_parent(tmp, successor);

        pc = (uintptr_t)(node->__rb_parent_color);
        tmp = __rb_parent(pc);
        __rb_change_child(node, successor, tmp, root);
        if (child2) {
            successor->__rb_parent_color = (cutest_map_node_t*)pc;
            rb_set_parent_color(child2, parent, RB_BLACK);
            rebalance = NULL;
        }
        else {
            uintptr_t pc2 = (uintptr_t)(successor->__rb_parent_color);
            successor->__rb_parent_color = (cutest_map_node_t*)pc;
            rebalance = __rb_is_black(pc2) ? parent : NULL;
        }
        tmp = successor;
    }

    return rebalance;
}

static void ____rb_erase_color(cutest_map_node_t* parent, cutest_map_t* root)
{
    cutest_map_node_t* node = NULL, * sibling, * tmp1, * tmp2;

    for (;;) {
        /*
        * Loop invariants:
        * - node is black (or NULL on first iteration)
        * - node is not the root (parent is not NULL)
        * - All leaf paths going through parent and node have a
        *   black node count that is 1 lower than other leaf paths.
        */
        sibling = parent->rb_right;
        if (node != sibling) {  /* node == parent->rb_left */
            if (rb_is_red(sibling)) {
                /*
                * Case 1 - left rotate at parent
                *
                *     P               S
                *    / \             / \
                *   N   s    -->    p   Sr
                *      / \         / \
                *     Sl  Sr      N   Sl
                */
                parent->rb_right = tmp1 = sibling->rb_left;
                sibling->rb_left = parent;
                rb_set_parent_color(tmp1, parent, RB_BLACK);
                __rb_rotate_set_parents(parent, sibling, root,
                    RB_RED);
                sibling = tmp1;
            }
            tmp1 = sibling->rb_right;
            if (!tmp1 || rb_is_black(tmp1)) {
                tmp2 = sibling->rb_left;
                if (!tmp2 || rb_is_black(tmp2)) {
                    /*
                    * Case 2 - sibling color flip
                    * (p could be either color here)
                    *
                    *    (p)           (p)
                    *    / \           / \
                    *   N   S    -->  N   s
                    *      / \           / \
                    *     Sl  Sr        Sl  Sr
                    *
                    * This leaves us violating 5) which
                    * can be fixed by flipping p to black
                    * if it was red, or by recursing at p.
                    * p is red when coming from Case 1.
                    */
                    rb_set_parent_color(sibling, parent,
                        RB_RED);
                    if (rb_is_red(parent))
                        rb_set_black(parent);
                    else {
                        node = parent;
                        parent = rb_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /*
                * Case 3 - right rotate at sibling
                * (p could be either color here)
                *
                *   (p)           (p)
                *   / \           / \
                *  N   S    -->  N   Sl
                *     / \             \
                *    sl  Sr            s
                *                       \
                *                        Sr
                */
                sibling->rb_left = tmp1 = tmp2->rb_right;
                tmp2->rb_right = sibling;
                parent->rb_right = tmp2;
                if (tmp1)
                    rb_set_parent_color(tmp1, sibling,
                        RB_BLACK);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /*
            * Case 4 - left rotate at parent + color flips
            * (p and sl could be either color here.
            *  After rotation, p becomes black, s acquires
            *  p's color, and sl keeps its color)
            *
            *      (p)             (s)
            *      / \             / \
            *     N   S     -->   P   Sr
            *        / \         / \
            *      (sl) sr      N  (sl)
            */
            parent->rb_right = tmp2 = sibling->rb_left;
            sibling->rb_left = parent;
            rb_set_parent_color(tmp1, sibling, RB_BLACK);
            if (tmp2)
                rb_set_parent(tmp2, parent);
            __rb_rotate_set_parents(parent, sibling, root,
                RB_BLACK);
            break;
        }
        else {
            sibling = parent->rb_left;
            if (rb_is_red(sibling)) {
                /* Case 1 - right rotate at parent */
                parent->rb_left = tmp1 = sibling->rb_right;
                sibling->rb_right = parent;
                rb_set_parent_color(tmp1, parent, RB_BLACK);
                __rb_rotate_set_parents(parent, sibling, root,
                    RB_RED);
                sibling = tmp1;
            }
            tmp1 = sibling->rb_left;
            if (!tmp1 || rb_is_black(tmp1)) {
                tmp2 = sibling->rb_right;
                if (!tmp2 || rb_is_black(tmp2)) {
                    /* Case 2 - sibling color flip */
                    rb_set_parent_color(sibling, parent,
                        RB_RED);
                    if (rb_is_red(parent))
                        rb_set_black(parent);
                    else {
                        node = parent;
                        parent = rb_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /* Case 3 - right rotate at sibling */
                sibling->rb_right = tmp1 = tmp2->rb_left;
                tmp2->rb_left = sibling;
                parent->rb_left = tmp2;
                if (tmp1)
                    rb_set_parent_color(tmp1, sibling,
                        RB_BLACK);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /* Case 4 - left rotate at parent + color flips */
            parent->rb_left = tmp2 = sibling->rb_right;
            sibling->rb_right = parent;
            rb_set_parent_color(tmp1, sibling, RB_BLACK);
            if (tmp2)
                rb_set_parent(tmp2, parent);
            __rb_rotate_set_parents(parent, sibling, root,
                RB_BLACK);
            break;
        }
    }
}

static void ev_map_low_erase(cutest_map_t* root, cutest_map_node_t* node)
{
    cutest_map_node_t* rebalance;
    rebalance = __rb_erase_augmented(node, root);
    if (rebalance)
        ____rb_erase_color(rebalance, root);
}

static void ev_map_erase(cutest_map_t* handler, cutest_map_node_t* node)
{
    handler->size--;
    ev_map_low_erase(handler, node);
}

///////////////////////////////////////////////////////////////////////////////
// C String
///////////////////////////////////////////////////////////////////////////////

typedef struct test_str
{
    char*                       ptr;            /**< String address. */
    size_t                      len;            /**< String length, not including NULL terminator. */
} test_str_t;

/**
 * @brief Construct a C string.
 * @param[in] str   Constanst C string.
 * @return          C string.
 */
static test_str_t _test_str(const char* str)
{
    test_str_t tmp;
    tmp.ptr = (char*)str;
    tmp.len = strlen(str);
    return tmp;
}

/**
 * @brief Split \p str into \p k and \p v by needle \p s, start at \p offset.
 * @note When failure, \p k and \p v remain untouched.
 * @return 0 if success, otherwise failure.
 */
static int _test_str_split(const test_str_t* str, test_str_t* k,
    test_str_t* v, const char* s, size_t offset)
{
    size_t i;
    size_t s_len = strlen(s);
    if (str->len < s_len)
    {
        goto error;
    }

    for (i = offset; i < str->len - s_len + 1; i++)
    {
        if (memcmp(str->ptr + i, s, s_len) == 0)
        {
            if (k != NULL)
            {
                k->ptr = str->ptr;
                k->len = i;
            }

            if (v != NULL)
            {
                v->ptr = str->ptr + i + s_len;
                v->len = str->len - i - s_len;
            }

            return 0;
        }
    }

error:
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
// Option parser
///////////////////////////////////////////////////////////////////////////////

typedef enum cutest_optparse_argtype
{
    CUTEST_OPTPARSE_NONE,
    CUTEST_OPTPARSE_REQUIRED,
    CUTEST_OPTPARSE_OPTIONAL,
} cutest_optparse_argtype_t;

typedef struct cutest_optparse_long_opt
{
    const char* longname;
    int                         shortname;
    cutest_optparse_argtype_t   argtype;
} cutest_optparse_long_opt_t;

typedef struct cutest_optparse
{
    char** argv;
    int                         permute;
    int                         optind;
    int                         optopt;
    char* optarg;
    char                        errmsg[64];
    size_t                      subopt;
} cutest_optparse_t;

#define OPTPARSE_MSG_INVALID "invalid option"
#define OPTPARSE_MSG_MISSING "option requires an argument"
#define OPTPARSE_MSG_TOOMANY "option takes no arguments"

static int _test_optparse_is_dashdash(const char* arg)
{
    return arg != 0 && arg[0] == '-' && arg[1] == '-' && arg[2] == '\0';
}

static int _test_optparse_is_shortopt(const char* arg)
{
    return arg != 0 && arg[0] == '-' && arg[1] != '-' && arg[1] != '\0';
}

static int _test_optparse_is_longopt(const char* arg)
{
    return arg != 0 && arg[0] == '-' && arg[1] == '-' && arg[2] != '\0';
}

static int _test_optparse_longopts_end(const cutest_optparse_long_opt_t* longopts, int i)
{
    return !longopts[i].longname && !longopts[i].shortname;
}

static int _test_optparse_longopts_match(const char* longname, const char* option)
{
    const char* a = option, * n = longname;
    if (longname == 0)
        return 0;
    for (; *a && *n && *a != '='; a++, n++)
        if (*a != *n)
            return 0;
    return *n == '\0' && (*a == '\0' || *a == '=');
}

static char* _test_optparse_longopts_arg(char* option)
{
    for (; *option && *option != '='; option++);
    if (*option == '=')
    {
        return option + 1;
    }
    return 0;
}

static void _test_optparse_from_long(const cutest_optparse_long_opt_t* longopts, char* optstring)
{
    char* p = optstring;
    int i;
    for (i = 0; !_test_optparse_longopts_end(longopts, i); i++)
    {
        if (longopts[i].shortname)
        {
            int a;
            *p++ = (char)(longopts[i].shortname);
            for (a = 0; a < (int)longopts[i].argtype; a++)
            {
                *p++ = ':';
            }
        }
    }
    *p = '\0';
}

static void _test_optparse_permute(cutest_optparse_t* options, int index)
{
    char* nonoption = options->argv[index];
    int i;
    for (i = index; i < options->optind - 1; i++)
    {
        options->argv[i] = options->argv[i + 1];
    }
    options->argv[options->optind - 1] = nonoption;
}

static int _test_optparse_argtype(const char* optstring, char c)
{
    int count = CUTEST_OPTPARSE_NONE;
    if (c == ':')
        return -1;
    for (; *optstring && c != *optstring; optstring++);
    if (!*optstring)
        return -1;
    if (optstring[1] == ':')
        count += optstring[2] == ':' ? 2 : 1;
    return count;
}

static int _test_optparse_error(cutest_optparse_t* options, const char* msg, const char* data)
{
    unsigned p = 0;
    const char* sep = " -- '";
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

static int _test_optparse(cutest_optparse_t* options, const char* optstring)
{
    int type;
    char* next;
    char* option = options->argv[options->optind];
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
    case CUTEST_OPTPARSE_NONE:
        if (option[1]) {
            options->subopt++;
        }
        else {
            options->subopt = 0;
            options->optind++;
        }
        return option[0];
    case CUTEST_OPTPARSE_REQUIRED:
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
    case CUTEST_OPTPARSE_OPTIONAL:
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

static int _test_optparse_long_fallback(cutest_optparse_t* options, const cutest_optparse_long_opt_t* longopts, int* longindex)
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

static int cutest_optparse_long(cutest_optparse_t* options,
    const cutest_optparse_long_opt_t* longopts, int* longindex)
{
    int i;
    char* option = options->argv[options->optind];
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
            int r = cutest_optparse_long(options, longopts, longindex);
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
        const char* name = longopts[i].longname;
        if (_test_optparse_longopts_match(name, option)) {
            char* arg;
            if (longindex)
                *longindex = i;
            options->optopt = longopts[i].shortname;
            arg = _test_optparse_longopts_arg(option);
            if (longopts[i].argtype == CUTEST_OPTPARSE_NONE && arg != 0) {
                return _test_optparse_error(options, OPTPARSE_MSG_TOOMANY, name);
            } if (arg != 0) {
                options->optarg = arg;
            }
            else if (longopts[i].argtype == CUTEST_OPTPARSE_REQUIRED) {
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

static void cutest_optparse_init(cutest_optparse_t * options, char** argv)
{
    options->argv = argv;
    options->permute = 1;
    options->optind = 1;
    options->subopt = 0;
    options->optarg = 0;
    options->errmsg[0] = '\0';
}

///////////////////////////////////////////////////////////////////////////////
// Once
///////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)

typedef INIT_ONCE test_once_t;
#define TEST_ONCE_INIT  INIT_ONCE_STATIC_INIT

static BOOL WINAPI _test_once_proxy(PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context)
{
    (void)InitOnce; (void)Context;

    void (*routine)(void) = (void (*)(void))Parameter;
    routine();

    return TRUE;
}

static void test_once(test_once_t* token, void (*routine)(void))
{
    InitOnceExecuteOnce(token, _test_once_proxy, (void*)routine, NULL);
}

#else

typedef pthread_once_t test_once_t;
#define TEST_ONCE_INIT  PTHREAD_ONCE_INIT

static void test_once(test_once_t* token, void (*routine)(void))
{
    pthread_once(token, routine);
}

#endif

///////////////////////////////////////////////////////////////////////////////
// Timestamp
///////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)

typedef struct test_timestamp_win32
{
    BOOL            usePerformanceCounter;
    LARGE_INTEGER   performanceFrequency;
    LARGE_INTEGER   offset;
    double          frequencyToMicroseconds;
} test_timestamp_win32_t;

static test_timestamp_win32_t s_timestamp_win32;

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

static void _test_init_timestamp_win32(void)
{
    s_timestamp_win32.usePerformanceCounter = QueryPerformanceFrequency(&s_timestamp_win32.performanceFrequency);

    if (s_timestamp_win32.usePerformanceCounter)
    {
        QueryPerformanceCounter(&s_timestamp_win32.offset);
        s_timestamp_win32.frequencyToMicroseconds = (double)s_timestamp_win32.performanceFrequency.QuadPart / 1000000.;
    }
    else
    {
        s_timestamp_win32.offset = _test_get_file_time_offset();
        s_timestamp_win32.frequencyToMicroseconds = 10.;
    }
}

void cutest_timestamp_get(cutest_timestamp_t* ts)
{
    /* One time initialize */
    static test_once_t token = TEST_ONCE_INIT;
    test_once(&token, _test_init_timestamp_win32);

    /* Get PerformanceCounter */
    LARGE_INTEGER t;
    if (s_timestamp_win32.usePerformanceCounter)
    {
        QueryPerformanceCounter(&t);
    }
    else
    {
        FILETIME                f;
        GetSystemTimeAsFileTime(&f);

        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    /* Shift to basis */
    t.QuadPart -= s_timestamp_win32.offset.QuadPart;

    /* Convert to microseconds */
    double microseconds = (double)t.QuadPart / s_timestamp_win32.frequencyToMicroseconds;
    t.QuadPart = (LONGLONG)microseconds;

    /* Set result */
    ts->sec = t.QuadPart / 1000000;
    ts->usec = t.QuadPart % 1000000;
}

#else

void cutest_timestamp_get(cutest_timestamp_t* ts)
{
    struct timespec tmp_ts;
    if (clock_gettime(CLOCK_MONOTONIC, &tmp_ts) < 0)
    {
        abort();
    }

    ts->sec = tmp_ts.tv_sec;
    ts->usec = tmp_ts.tv_nsec / 1000;
}

#endif

int cutest_timestamp_dif(const cutest_timestamp_t* t1, const cutest_timestamp_t* t2, cutest_timestamp_t* dif)
{
    cutest_timestamp_t tmp_dif;
    const cutest_timestamp_t* large_t = t1->sec > t2->sec ? t1 : (t1->sec < t2->sec ? t2 : (t1->usec > t2->usec ? t1 : t2));
    const cutest_timestamp_t* little_t = large_t == t1 ? t2 : t1;

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

///////////////////////////////////////////////////////////////////////////////
// Print
///////////////////////////////////////////////////////////////////////////////

typedef enum cutest_print_color
{
    CUTEST_PRINT_COLOR_DEFAULT,
    CUTEST_PRINT_COLOR_RED,
    CUTEST_PRINT_COLOR_GREEN,
    CUTEST_PRINT_COLOR_YELLOW,
} cutest_print_color_t;

#if defined(_WIN32)

static int _should_use_color(int is_tty)
{
    /**
     * On Windows the $TERM variable is usually not set, but the console there
     * does support colors.
     */
    return is_tty;
}

static int _get_bit_offset(WORD color_mask)
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

static WORD _get_color_attribute(cutest_print_color_t color)
{
    switch (color)
    {
    case CUTEST_PRINT_COLOR_RED:
        return FOREGROUND_RED;
    case CUTEST_PRINT_COLOR_GREEN:
        return FOREGROUND_GREEN;
    case CUTEST_PRINT_COLOR_YELLOW:
        return FOREGROUND_RED | FOREGROUND_GREEN;
    default:
        return 0;
    }
}

static WORD _get_new_color(cutest_print_color_t color, WORD old_color_attrs)
{
    // Let's reuse the BG
    static const WORD background_mask = BACKGROUND_BLUE | BACKGROUND_GREEN |
        BACKGROUND_RED | BACKGROUND_INTENSITY;
    static const WORD foreground_mask = FOREGROUND_BLUE | FOREGROUND_GREEN |
        FOREGROUND_RED | FOREGROUND_INTENSITY;
    const WORD existing_bg = old_color_attrs & background_mask;

    WORD new_color =
        _get_color_attribute(color) | existing_bg | FOREGROUND_INTENSITY;
    const int bg_bitOffset = _get_bit_offset(background_mask);
    const int fg_bitOffset = _get_bit_offset(foreground_mask);

    if (((new_color & background_mask) >> bg_bitOffset) ==
        ((new_color & foreground_mask) >> fg_bitOffset))
    {
        new_color ^= FOREGROUND_INTENSITY;  // invert intensity
    }
    return new_color;
}

static int _test_color_vfprintf(FILE* stream, cutest_print_color_t color, const char* fmt, va_list ap)
{
    int ret;
    const HANDLE stdout_handle = (HANDLE)_get_osfhandle(fileno(stream));

    // Gets the current text color.
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
    const WORD old_color_attrs = buffer_info.wAttributes;
    const WORD new_color = _get_new_color(color, old_color_attrs);

    // We need to flush the stream buffers into the console before each
    // SetConsoleTextAttribute call lest it affect the text that is already
    // printed but has not yet reached the console.
    fflush(stream);
    SetConsoleTextAttribute(stdout_handle, new_color);

    ret = vfprintf(stream, fmt, ap);

    fflush(stream);
    // Restores the text color.
    SetConsoleTextAttribute(stdout_handle, old_color_attrs);

    return ret;
}

#else

typedef struct color_printf_ctx
{
    int terminal_color_support;
}color_printf_ctx_t;

static color_printf_ctx_t g_color_printf = {
    0,
};

static void _initlize_color_unix(void)
{
    static const char* support_color_term_list[] = {
        "xterm",
        "xterm-color",
        "xterm-256color",
        "screen",
        "screen-256color",
        "tmux",
        "tmux-256color",
        "rxvt-unicode",
        "rxvt-unicode-256color",
        "linux",
        "cygwin",
    };

    /* On non-Windows platforms, we rely on the TERM variable. */
    const char* term = getenv("TERM");
    if (term == NULL)
    {
        return;
    }

    size_t i;
    for (i = 0; i < ARRAY_SIZE(support_color_term_list); i++)
    {
        if (strcmp(term, support_color_term_list[i]) == 0)
        {
            g_color_printf.terminal_color_support = 1;
            break;
        }
    }
}

static int _should_use_color(int is_tty)
{
    static pthread_once_t once_control = PTHREAD_ONCE_INIT;
    pthread_once(&once_control, _initlize_color_unix);

    return is_tty && g_color_printf.terminal_color_support;
}

static const char* _get_ansi_color_code_fg(cutest_print_color_t color)
{
    switch (color)
    {
    case CUTEST_PRINT_COLOR_RED:
        return "31";
    case CUTEST_PRINT_COLOR_GREEN:
        return "32";
    case CUTEST_PRINT_COLOR_YELLOW:
        return "33";
    default:
        break;
    }

    return NULL;
}

static int _test_color_vfprintf(FILE* stream, cutest_print_color_t color, const char* fmt, va_list ap)
{
    int ret;
    fprintf(stream, "\033[0;%sm", _get_ansi_color_code_fg(color));
    ret = vfprintf(stream, fmt, ap);
    fprintf(stream, "\033[m");  // Resets the terminal to default.
    fflush(stream);
    return ret;
}

#endif

/**
 * @brief Print data to \p stream.
 */
static int cutest_color_vfprintf(cutest_print_color_t color, FILE* stream, const char* fmt, va_list ap)
{
    assert(stream != NULL);

    int stream_fd = fileno(stream);
    if (!_should_use_color(isatty(stream_fd)) || (color == CUTEST_PRINT_COLOR_DEFAULT))
    {
        return vfprintf(stream, fmt, ap);
    }

    return _test_color_vfprintf(stream, color, fmt, ap);
}

static int cutest_color_fprintf(cutest_print_color_t color, FILE* stream, const char* fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = cutest_color_vfprintf(color, stream, fmt, ap);
    va_end(ap);

    return ret;
}

/************************************************************************/
/* test                                                                 */
/************************************************************************/

#define COLOR_GREEN(str)                    "@G" str "@D"
#define COLOR_RED(str)                      "@R" str "@D"
#define COLOR_YELLO(str)                    "@Y" str "@D"

#define MASK_FAILURE                        (0x01 << 0x00)
#define MASK_SKIPPED                        (0x01 << 0x01)
#define SET_MASK(val, mask)                 do { (val) |= (mask); } while (0)
#define HAS_MASK(val, mask)                 ((val) & (mask))

/**
 * @brief microseconds in one second
 */
#define USEC_IN_SEC                         (1 * 1000 * 1000)

#define CONTAINER_OF(ptr, TYPE, member) \
    ((TYPE*)((char*)(ptr) - (size_t)&((TYPE*)0)->member))

#define TEST_CASE_TABLE_INIT    CUTEST_MAP_INIT(_test_on_cmp_case, NULL)

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

typedef struct test_case_info
{
    char                    fmt_name[256];  /**< Formatted name. */
    size_t                  fmt_name_sz;    /**< The length of formatted name, not include NULL termainator. */

    cutest_case_t*          test_case;      /**< Test case. */
    cutest_case_node_t*     test_case_node; /**< Test case node. */

    cutest_timestamp_t      tv_case_beg;    /**< Start time. */
    cutest_timestamp_t      tv_case_end;    /**< End time. */
} test_case_info_t;

typedef struct test_nature_s
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
} test_nature_t;

typedef struct test_ctx
{
    struct
    {
        unsigned long       tid;                            /**< Thread ID */
        unsigned long long  seed;                           /**< Random seed */
        cutest_case_node_t* cur_node;                       /**< Current running test case node. */
    } runtime;

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
        test_str_t          pattern;                         /**< `--test_filter` */
    } filter;

    struct
    {
        unsigned            break_on_failure : 1;           /**< DebugBreak when failure */
        unsigned            no_print_time : 1;              /**< Whether to print execution cost time */
        unsigned            also_run_disabled_tests : 1;    /**< Also run disabled tests */
        unsigned            shuffle : 1;                    /**< Randomize running cases */
    } mask;

    struct
    {
        FILE*               f_out;                          /**< Output file */
        int                 need_close;                     /**< Need close */
    } io;

    jmp_buf                 jmpbuf;                         /**< Jump buffer */
    const cutest_hook_t*    hook;
}test_ctx_t;

static int _test_on_cmp_case(const cutest_map_node_t* key1, const cutest_map_node_t* key2, void* arg)
{
    (void)arg;
    int ret;
    cutest_case_node_t* n1 = CONTAINER_OF(key1, cutest_case_node_t, node);
    cutest_case_node_t* n2 = CONTAINER_OF(key2, cutest_case_node_t, node);

    cutest_case_t* t1 = n1->test_case;
    cutest_case_t* t2 = n2->test_case;

    /* randkey always go first. */
    if (n1->randkey < n2->randkey)
    {
        return -1;
    }
    if (n1->randkey > n2->randkey)
    {
        return 1;
    }

    if ((ret = strcmp(t1->info.fixture, t2->info.fixture)) != 0)
    {
        return ret;
    }

    if ((ret = strcmp(t1->info.case_name, t2->info.case_name)) != 0)
    {
        return ret;
    }

    if (n1->parameterized_idx == n2->parameterized_idx)
    {
        return 0;
    }
    return n1->parameterized_idx < n2->parameterized_idx ? -1 : 1;
}

static test_ctx_t           g_test_ctx;
static test_nature_t        g_test_nature = {
    TEST_CASE_TABLE_INIT,                                               /* .case_table */
    { 0, { 0, 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0 } },                  /* .precision */
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
static int _test_pattern_match(const char* pat, size_t pat_sz, const char* str, size_t str_sz)
{
    if (pat_sz == 0)
    {
        return str_sz == 0;
    }

    switch (*pat)
    {
    case '?':
        return str_sz != 0 && _test_pattern_match(pat + 1, pat_sz - 1, str + 1, str_sz - 1);
    case '*':
        return (str_sz != 0 && _test_pattern_match(pat, pat_sz, str + 1, str_sz - 1)) || _test_pattern_match(pat + 1, pat_sz - 1, str, str_sz);
    default:
        return *pat == *str && _test_pattern_match(pat + 1, pat_sz - 1, str + 1, str_sz - 1);
    }
}

/**
 * @return true if this test will run, false if not.
 */
static int _test_check_pattern(const char* str, size_t str_sz)
{
    test_str_t k, v = g_test_ctx.filter.pattern;

    /* If no pattern, run this test. */
    if (v.ptr == NULL)
    {
        return 1;
    }

    size_t cnt_positive_patterns = 0;
    int flag_match_positive_pattern = 0;

    /* Split patterns by `:` */
    while (_test_str_split(&v, &k, &v, ":", 0) == 0)
    {
        /* If it is a positive pattern. */
        if (k.ptr[0] != '-')
        {
            cnt_positive_patterns++;

            /* Record if it match any of positive pattern */
            if (_test_pattern_match(k.ptr, k.len, str, str_sz))
            {
                flag_match_positive_pattern = 1;
            }
        }
        else if (_test_pattern_match(k.ptr + 1, k.len - 1, str, str_sz))
        {/* If match negative pattern, don't run. */
            return 0;
        }
    }
    if (v.ptr[0] != '-')
    {
        cnt_positive_patterns++;
        if (_test_pattern_match(v.ptr, v.len, str, str_sz))
        {
            flag_match_positive_pattern = 1;
        }
    }
    else if (_test_pattern_match(v.ptr + 1, v.len - 1, str, str_sz))
    {
        return 0;
    }

    return cnt_positive_patterns ? flag_match_positive_pattern : 1;
}

/**
 * @return bool
 */
static int _test_check_disable(const char* name)
{
    return !g_test_ctx.mask.also_run_disabled_tests && (strncmp("DISABLED_", name, 9) == 0);
}

static int _print_encoded(FILE* stream, const char* str)
{
    int ret = 0;
    cutest_print_color_t color = CUTEST_PRINT_COLOR_DEFAULT;

    for (;;)
    {
        const char* p = strchr(str, '@');
        if (p == NULL)
        {
            ret += cutest_color_fprintf(color, stream, "%s", str);
            return ret;
        }

        ret += cutest_color_fprintf(color, stream, "%.*s", p - str, str);

        const char ch = p[1];
        str = p + 2;

        switch (ch)
        {
        case '@':
            ret += cutest_color_fprintf(color, stream, "@");
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

static void _test_hook_before_fixture_setup(cutest_case_t* test_case)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_setup == NULL)
    {
        return;
    }
    g_test_ctx.hook->before_setup(test_case->info.fixture);
}

static void _test_hook_after_fixture_setup(cutest_case_t* test_case, int ret)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_setup == NULL)
    {
        return;
    }
    g_test_ctx.hook->after_setup(test_case->info.fixture, ret);
}

/**
 * @brief Run setup stage.
 * @param[in] test_case The test case to run.
 * @return              0 if success, otherwise failure.
 */
static int _test_fixture_run_setup(test_case_info_t* info)
{
    int ret = 0;
    if (info->test_case->stage.setup == NULL)
    {
        return 0;
    }

    if ((ret = setjmp(g_test_ctx.jmpbuf)) != 0)
    {
        SET_MASK(info->test_case_node->mask, ret);
        goto after_setup;
    }

    _test_hook_before_fixture_setup(info->test_case);
    info->test_case->stage.setup();

after_setup:
    _test_hook_after_fixture_setup(info->test_case, ret);
    return ret;
}

static void _test_hook_before_test(test_case_info_t* info)
{
    cutest_case_t* test_case = info->test_case;
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_test == NULL)
    {
        return;
    }

    size_t fixture_sz = strlen(test_case->info.fixture);
    g_test_ctx.hook->before_test(test_case->info.fixture, info->fmt_name + fixture_sz);
}

static void _test_hook_after_test(test_case_info_t* info, int ret)
{
    cutest_case_t* test_case = info->test_case;
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_test == NULL)
    {
        return;
    }

    size_t fixture_sz = strlen(test_case->info.fixture);
    g_test_ctx.hook->after_test(test_case->info.fixture, info->fmt_name + fixture_sz, ret);
}

static void _test_hook_before_teardown(cutest_case_t* test_case)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_teardown == NULL)
    {
        return;
    }

    g_test_ctx.hook->before_teardown(test_case->info.fixture);
}

static void _test_hook_after_teardown(cutest_case_t* test_case, int ret)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_teardown == NULL)
    {
        return;
    }
    g_test_ctx.hook->after_teardown(test_case->info.fixture, ret);
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

static void _test_fixture_run_teardown(test_case_info_t* info)
{
    int ret = 0;
    if (info->test_case->stage.teardown == NULL)
    {
        return;
    }

    if ((ret = setjmp(g_test_ctx.jmpbuf)) != 0)
    {
        SET_MASK(info->test_case_node->mask, ret);
        goto after_teardown;
    }

    _test_hook_before_teardown(info->test_case);
    info->test_case->stage.teardown();

after_teardown:
    _test_hook_after_teardown(info->test_case, ret);
}

static FILE* _get_logfile(void)
{
    return g_test_ctx.io.f_out != NULL ? g_test_ctx.io.f_out : stdout;
}

static int _cutest_color_printf(cutest_print_color_t color, const char* fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = cutest_color_vfprintf(color, _get_logfile(), fmt, ap);
    va_end(ap);

    return ret;
}

static void _test_finishlize(test_case_info_t* info)
{
    cutest_timestamp_get(&info->tv_case_end);

    cutest_timestamp_t tv_diff;
    cutest_timestamp_dif(&info->tv_case_beg, &info->tv_case_end, &tv_diff);

    if (HAS_MASK(info->test_case_node->mask, MASK_FAILURE))
    {
        g_test_ctx.counter.result.failed++;
        _cutest_color_printf(CUTEST_PRINT_COLOR_RED, "[  FAILED  ]");
    }
    else if (HAS_MASK(info->test_case_node->mask, MASK_SKIPPED))
    {
        g_test_ctx.counter.result.skipped++;
        _cutest_color_printf(CUTEST_PRINT_COLOR_YELLOW, "[   SKIP   ]");
    }
    else
    {
        g_test_ctx.counter.result.success++;
        _cutest_color_printf(CUTEST_PRINT_COLOR_GREEN, "[       OK ]");
    }

    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %s", info->fmt_name);
    if (!g_test_ctx.mask.no_print_time)
    {
        unsigned long take_time = (unsigned long)(tv_diff.sec * 1000 + tv_diff.usec / 1000);
        _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " (%lu ms)", take_time);
    }
    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "\n");
}

static int _test_run_case_normal_body(test_case_info_t* info)
{
    int ret = 0;
    if ((ret = setjmp(g_test_ctx.jmpbuf)) != 0)
    {
        SET_MASK(info->test_case_node->mask, ret);
        goto after_body;
    }

    _test_hook_before_test(info);
    ((cutest_procedure_fn)info->test_case->stage.body)();

after_body:
    _test_hook_after_test(info, ret);
    return ret;
}

/**
 * @return  1 if need to process, 0 to stop.
 */
static int _test_run_prepare(test_case_info_t* info)
{
    /* Check if need to run this test case */
    if (!_test_check_pattern(info->fmt_name, info->fmt_name_sz))
    {
        return 1;
    }
    g_test_ctx.counter.result.total++;

    /* check if this test is disabled */
    if (_test_check_disable(info->test_case->info.case_name))
    {
        g_test_ctx.counter.result.disabled++;
        return 1;
    }

    _cutest_color_printf(CUTEST_PRINT_COLOR_GREEN, "[ RUN      ]");
    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %s\n", info->fmt_name);

    /* record start time */
    cutest_timestamp_get(&info->tv_case_beg);
    return 0;
}

static void _test_run_case_normal(cutest_case_node_t* node)
{
    cutest_case_t* test_case = node->test_case;

    test_case_info_t info;
    int ret = snprintf(info.fmt_name, sizeof(info.fmt_name), "%s.%s",
        test_case->info.fixture, test_case->info.case_name);
    if (ret < 0 || ret >= (int)sizeof(info.fmt_name))
    {
        abort();
    }
    info.fmt_name_sz = ret;
    info.test_case = test_case;
    info.test_case_node = node;

    if (_test_run_prepare(&info) != 0)
    {
        return;
    }

    /* setup */
    if (_test_fixture_run_setup(&info) != 0)
    {
        goto cleanup;
    }

    _test_run_case_normal_body(&info);
    _test_fixture_run_teardown(&info);

cleanup:
    _test_finishlize(&info);
}

static void _test_run_case_parameterized_body(test_case_info_t* info,
    cutest_parameterized_info_t* parameterized_info, size_t idx)
{
    _test_hook_before_test(info);
    ((cutest_parameterized_fn)info->test_case->stage.body)(parameterized_info->test_data, idx);
    _test_hook_after_test(info, -1);
}

static void _test_run_case_parameterized_idx(test_case_info_t* info,
    cutest_parameterized_info_t* parameterized_info, size_t idx)
{
    if (_test_run_prepare(info) != 0)
    {
        return;
    }

    /* setup */
    if (_test_fixture_run_setup(info) != 0)
    {
        goto cleanup;
    }

    _test_run_case_parameterized_body(info, parameterized_info, idx);
    _test_fixture_run_teardown(info);

cleanup:
    _test_finishlize(info);
}

static void _test_run_case_parameterized(cutest_case_node_t* node)
{
    cutest_case_t* test_case = node->test_case;
    cutest_parameterized_info_t* parameterized_info = node->test_case->get_parameterized_info();

    test_case_info_t info;
    info.test_case_node = node;
    info.test_case = test_case;

    int ret = snprintf(info.fmt_name, sizeof(info.fmt_name), "%s.%s/%u",
        test_case->info.fixture, test_case->info.case_name, (unsigned)node->parameterized_idx);
    if (ret < 0 || ret >= (int)sizeof(info.fmt_name))
    {
        abort();
    }
    info.fmt_name_sz = ret;

    _test_run_case_parameterized_idx(&info, parameterized_info, node->parameterized_idx);
}

/**
* run test case.
* the target case was set to `g_test_ctx.runtime.cur_case`
*/
static void _test_run_case(cutest_case_node_t* node)
{
    node->mask = 0;

    if (node->test_case->get_parameterized_info != NULL)
    {
        _test_run_case_parameterized(node);
        return;
    }

    _test_run_case_normal(node);
}

static void _test_reset_all_test(void)
{
    memset(&g_test_ctx.counter.result, 0, sizeof(g_test_ctx.counter.result));

    cutest_map_node_t* it = cutest_map_begin(&g_test_nature.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_case_node_t* node = CONTAINER_OF(it, cutest_case_node_t, node);
        node->mask = 0;
    }
}

static void _test_show_report_failed(void)
{
    char buffer[512];

    cutest_map_node_t* it = cutest_map_begin(&g_test_nature.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_case_node_t* node = CONTAINER_OF(it, cutest_case_node_t, node);
        if (!HAS_MASK(node->mask, MASK_FAILURE))
        {
            continue;
        }

        cutest_case_t* test_case = node->test_case;

        if (test_case->get_parameterized_info == NULL)
        {
            snprintf(buffer, sizeof(buffer), "%s.%s",
                test_case->info.fixture, test_case->info.case_name);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "%s.%s/%u",
                test_case->info.fixture, test_case->info.case_name, (unsigned)node->parameterized_idx);
        }

        _cutest_color_printf(CUTEST_PRINT_COLOR_RED, "[  FAILED  ]");
        _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %s\n", buffer);
    }
}

static void _test_show_report(const cutest_timestamp_t* tv_total_start,
    const cutest_timestamp_t* tv_total_end)
{
    cutest_timestamp_t tv_diff;
    cutest_timestamp_dif(tv_total_start, tv_total_end, &tv_diff);

    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "[==========]");
    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %u/%u test case%s ran.",
        g_test_ctx.counter.result.total,
        (unsigned)g_test_nature.case_table.size,
        g_test_ctx.counter.result.total > 1 ? "s" : "");
    if (!g_test_ctx.mask.no_print_time)
    {
        unsigned long take_time = (unsigned long)(tv_diff.sec * 1000 + tv_diff.usec / 1000);
        _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " (%lu ms total)", take_time);
    }
    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "\n");

    if (g_test_ctx.counter.result.disabled != 0)
    {
        _cutest_color_printf(CUTEST_PRINT_COLOR_GREEN, "[ DISABLED ]");
        _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %u test%s.\n",
            g_test_ctx.counter.result.disabled,
            g_test_ctx.counter.result.disabled > 1 ? "s" : "");
    }
    if (g_test_ctx.counter.result.skipped != 0)
    {
        _cutest_color_printf(CUTEST_PRINT_COLOR_YELLOW,"[ BYPASSED ]");
        _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %u test%s.\n",
            g_test_ctx.counter.result.skipped,
            g_test_ctx.counter.result.skipped > 1 ? "s" : "");
    }
    if (g_test_ctx.counter.result.success != 0)
    {
        _cutest_color_printf(CUTEST_PRINT_COLOR_GREEN, "[  PASSED  ]");
        _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %u test%s.\n",
            g_test_ctx.counter.result.success,
            g_test_ctx.counter.result.success > 1 ? "s" : "");
    }

    /* don't show failed tests if every test was success */
    if (g_test_ctx.counter.result.failed == 0)
    {
        return;
    }

    _cutest_color_printf(CUTEST_PRINT_COLOR_RED, "[  FAILED  ]");
    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT,
        " %u test%s, listed below:\n", g_test_ctx.counter.result.failed, g_test_ctx.counter.result.failed > 1 ? "s" : "");
    _test_show_report_failed();
}

static void _test_setup_arg_pattern(char* user_pattern)
{
    g_test_ctx.filter.pattern = _test_str(user_pattern);
}

static void _test_list_tests_print_name(const cutest_case_node_t* node)
{
    cutest_case_t* test_case = node->test_case;
    if (test_case->get_parameterized_info == NULL)
    {
        _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "  %s\n", test_case->info.case_name);
        return;
    }

    cutest_parameterized_info_t* parameterized_info = test_case->get_parameterized_info();
    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "  %s/%u  # <%s> %s\n",
        test_case->info.case_name, parameterized_info->test_data_sz, parameterized_info->type_name, parameterized_info->test_data_info);
}

static void _test_list_tests(void)
{
    const char* last_class_name = "";

    cutest_map_node_t* it = cutest_map_begin(&g_test_nature.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_case_node_t* node = CONTAINER_OF(it, cutest_case_node_t, node);
        cutest_case_t* test_case = node->test_case;

        /* some compiler will make same string with different address */
        if (last_class_name != test_case->info.fixture
            && strcmp(last_class_name, test_case->info.fixture) != 0)
        {
            last_class_name = test_case->info.fixture;
            _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "%s.\n", last_class_name);
        }
        _test_list_tests_print_name(node);
    }
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
    g_test_ctx.mask.no_print_time = !val;
}

static void _test_setup_arg_random_seed(const char* str)
{
    long long val = time(NULL);
    sscanf(str, "%lld", &val);

    _test_srand(val);
}

static void _test_setup_precision(void)
{
    /* Ensure size match */
    assert(sizeof(((double_point_t*)NULL)->bits_) == sizeof(((double_point_t*)NULL)->value_));
    assert(sizeof(((float_point_t*)NULL)->bits_) == sizeof(((float_point_t*)NULL)->value_));

    g_test_nature.precision.kMaxUlps = 4;
    // double
    {
        g_test_nature.precision._double.kBitCount_64 = 8 * sizeof(((double_point_t*)NULL)->value_);
        g_test_nature.precision._double.kSignBitMask_64 = (uint64_t)1 << (g_test_nature.precision._double.kBitCount_64 - 1);
        g_test_nature.precision._double.kFractionBitCount_64 = DBL_MANT_DIG - 1;
        g_test_nature.precision._double.kExponentBitCount_64 = g_test_nature.precision._double.kBitCount_64 - 1 - g_test_nature.precision._double.kFractionBitCount_64;
        g_test_nature.precision._double.kFractionBitMask_64 = (~(uint64_t)0) >> (g_test_nature.precision._double.kExponentBitCount_64 + 1);
        g_test_nature.precision._double.kExponentBitMask_64 = ~(g_test_nature.precision._double.kSignBitMask_64 | g_test_nature.precision._double.kFractionBitMask_64);
    }

    // float
    {
        g_test_nature.precision._float.kBitCount_32 = 8 * sizeof(((float_point_t*)NULL)->value_);
        g_test_nature.precision._float.kSignBitMask_32 = (uint32_t)1 << (g_test_nature.precision._float.kBitCount_32 - 1);
        g_test_nature.precision._float.kFractionBitCount_32 = FLT_MANT_DIG - 1;
        g_test_nature.precision._float.kExponentBitCount_32 = g_test_nature.precision._float.kBitCount_32 - 1 - g_test_nature.precision._float.kFractionBitCount_32;
        g_test_nature.precision._float.kFractionBitMask_32 = (~(uint32_t)0) >> (g_test_nature.precision._float.kExponentBitCount_32 + 1);
        g_test_nature.precision._float.kExponentBitMask_32 = ~(g_test_nature.precision._float.kSignBitMask_32 | g_test_nature.precision._float.kFractionBitMask_32);
    }
}

static void _test_shuffle_cases(void)
{
    cutest_map_t copy_case_table = TEST_CASE_TABLE_INIT;

    cutest_map_node_t* it = cutest_map_begin(&g_test_nature.case_table);
    while (it != NULL)
    {
        cutest_case_node_t* node = CONTAINER_OF(it, cutest_case_node_t, node);
        it = cutest_map_next(it);
        ev_map_erase(&g_test_nature.case_table, &node->node);

        node->randkey = _test_rand();
        cutest_map_insert(&copy_case_table, &node->node);
    }
    g_test_nature.case_table = copy_case_table;
}

/**
 * @brief Setup resources that will not change.
 */
static void cutest_setup_once(void)
{
    static test_once_t token = TEST_ONCE_INIT;
    test_once(&token, _test_setup_precision);
}

static void _test_close_logfile(void)
{
    if (g_test_ctx.io.need_close)
    {
        fclose(g_test_ctx.io.f_out);
    }
    g_test_ctx.io.f_out = NULL;
    g_test_ctx.io.need_close = 0;
}

#if !defined(_WIN32)
static int fopen_s(FILE** pFile, const char* filename, const char* mode)
{
    if ((*pFile = fopen(filename, mode)) == NULL)
    {
        return errno;
    }
    return 0;
}
#endif

static int _test_setup_arg_logfile(const char* path)
{
    _test_close_logfile();

    int errcode = fopen_s(&g_test_ctx.io.f_out, path, "w");
    if (errcode != 0)
    {
        PERR(errcode, "open file `%s` failed", path);
        return -1;
    }

    g_test_ctx.io.need_close = 1;
    return 0;
}

static void _test_prepare(void)
{
    _test_srand(time(NULL));
    g_test_ctx.runtime.tid = GET_TID();
    g_test_ctx.counter.repeat.repeat = 1;
}

/**
 * @brief Setup test context
 * @param[in] argc      The number of command line argument.
 * @param[in] argv      Command line argument list.
 * @param[in] hook      Global test hook.
 * @param[out] b_exit   Whether need to exit.
 * @return              0 if success, otherwise failure.
 */
static int _test_setup(int argc, char* argv[], const cutest_hook_t* hook, int* b_exit)
{
    (void)argc;

    memset(&g_test_ctx, 0, sizeof(g_test_ctx));
    cutest_setup_once();
    _test_prepare();

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
            g_test_ctx.mask.also_run_disabled_tests = 1;
            break;
        case test_repeat:
            _test_setup_arg_repeat(options.optarg);
            break;
        case test_shuffle:
            g_test_ctx.mask.shuffle = 1;
            break;
        case test_random_seed:
            _test_setup_arg_random_seed(options.optarg);
            break;
        case test_print_time:
            _test_setup_arg_print_time(options.optarg);
            break;
        case test_break_on_failure:
            g_test_ctx.mask.break_on_failure = 1;
            break;
        case test_logfile:
            _test_setup_arg_logfile(options.optarg);
            break;
        case help:
            _print_encoded(_get_logfile(), s_test_help_encoded);
            *b_exit = 1;
            return 0;
        default:
            break;
        }
    }

    /* shuffle if necessary */
    if (g_test_ctx.mask.shuffle)
    {
        _test_shuffle_cases();
    }
    g_test_ctx.hook = hook;

    return 0;
}

static void _run_all_test_once(void)
{
    _test_reset_all_test();

    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "[==========]");
    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " total %u test%s registered.\n",
        (unsigned)g_test_nature.case_table.size,
        g_test_nature.case_table.size > 1 ? "s" : "");

    cutest_timestamp_t tv_total_start, tv_total_end;
    cutest_timestamp_get(&tv_total_start);

    cutest_map_node_t* it = cutest_map_begin(&g_test_nature.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        g_test_ctx.runtime.cur_node = CONTAINER_OF(it, cutest_case_node_t, node);
        _test_run_case(g_test_ctx.runtime.cur_node);
    }

    cutest_timestamp_get(&tv_total_end);

    _test_show_report(&tv_total_start, &tv_total_end);
}

static uint32_t _test_float_point_exponent_bits(const float_point_t* p)
{
    return g_test_nature.precision._float.kExponentBitMask_32 & p->bits_;
}

static uint64_t _test_double_point_exponent_bits(const double_point_t* p)
{
    return g_test_nature.precision._double.kExponentBitMask_64 & p->bits_;
}

static uint32_t _test_float_point_fraction_bits(const float_point_t* p)
{
    return g_test_nature.precision._float.kFractionBitMask_32 & p->bits_;
}

static uint64_t _test_double_point_fraction_bits(const double_point_t* p)
{
    return g_test_nature.precision._double.kFractionBitMask_64 & p->bits_;
}

static int _test_float_point_is_nan(const float_point_t* p)
{
    return (_test_float_point_exponent_bits(p) == g_test_nature.precision._float.kExponentBitMask_32)
        && (_test_float_point_fraction_bits(p) != 0);
}

static int _test_double_point_is_nan(const double_point_t* p)
{
    return (_test_double_point_exponent_bits(p) == g_test_nature.precision._double.kExponentBitMask_64)
        && (_test_double_point_fraction_bits(p) != 0);
}

static uint32_t _test_float_point_sign_and_magnitude_to_biased(const uint32_t sam)
{
    if (g_test_nature.precision._float.kSignBitMask_32 & sam)
    {
        // sam represents a negative number.
        return ~sam + 1;
    }

    // sam represents a positive number.
    return g_test_nature.precision._float.kSignBitMask_32 | sam;
}

static uint64_t _test_double_point_sign_and_magnitude_to_biased(const uint64_t sam)
{
    if (g_test_nature.precision._double.kSignBitMask_64 & sam)
    {
        // sam represents a negative number.
        return ~sam + 1;
    }

    // sam represents a positive number.
    return g_test_nature.precision._double.kSignBitMask_64 | sam;
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

static void _test_cleanup(void)
{
    memset(&g_test_ctx.runtime, 0, sizeof(g_test_ctx.runtime));
    memset(&g_test_ctx.counter, 0, sizeof(g_test_ctx.counter));
    memset(&g_test_ctx.mask, 0, sizeof(g_test_ctx.mask));
    memset(&g_test_ctx.filter, 0, sizeof(g_test_ctx.filter));

    _test_close_logfile();
    g_test_ctx.hook = NULL;
}

static void _run_all_tests(void)
{
    for (g_test_ctx.counter.repeat.repeated = 0;
        g_test_ctx.counter.repeat.repeated < g_test_ctx.counter.repeat.repeat;
        g_test_ctx.counter.repeat.repeated++)
    {
        if (g_test_ctx.counter.repeat.repeat > 1)
        {
            _cutest_color_printf(CUTEST_PRINT_COLOR_YELLOW, "[==========]");
            _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " start loop: %u/%u\n",
                g_test_ctx.counter.repeat.repeated + 1, g_test_ctx.counter.repeat.repeat);
        }

        _run_all_test_once();

        if (g_test_ctx.counter.repeat.repeat > 1)
        {
            _cutest_color_printf(CUTEST_PRINT_COLOR_YELLOW, "[==========]");
            _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " end loop (%u/%u)\n",
                g_test_ctx.counter.repeat.repeated + 1, g_test_ctx.counter.repeat.repeat);
            if (g_test_ctx.counter.repeat.repeated < g_test_ctx.counter.repeat.repeat - 1)
            {
                _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "\n");
            }
        }
    }
}

void cutest_register_case(cutest_case_t* data, cutest_case_node_t* node, size_t node_sz)
{
    (void)node_sz;

    if (data->get_parameterized_info == NULL)
    {
        assert(node_sz >= 1);

        node->test_case = data;
        node->mask = 0;
        node->randkey = 0;
        node->parameterized_idx = 0;

        if (cutest_map_insert(&g_test_nature.case_table, &node->node) != 0)
        {
            abort();
        }
        return;
    }

    size_t i;
    cutest_parameterized_info_t* parameterized_info = data->get_parameterized_info();
    for (i = 0; i < parameterized_info->test_data_sz; i++)
    {
        assert(i < node_sz);

        node[i].test_case = data;
        node[i].randkey = 0;
        node[i].parameterized_idx = i;
        node[i].mask = 0;

        if (cutest_map_insert(&g_test_nature.case_table, &node[i].node) != 0)
        {
            abort();
        }
    }
}

int cutest_run_tests(int argc, char* argv[], const cutest_hook_t* hook)
{
    int ret = 0;

    /* Parser parameter */
    int b_exit = 0;
    if ((ret = _test_setup(argc, argv, hook, &b_exit)) < 0 || b_exit)
    {
        goto fin;
    }

    _test_hook_before_all_test(argc, argv);
    _run_all_tests();

    ret = (int)g_test_ctx.counter.result.failed;
fin:
    _test_hook_after_all_test();
    _test_cleanup();

    return ret != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

void cutest_unwrap_assert_fail(const char *expr, const char *file, int line, const char *func)
{
    fprintf(stderr, "Assertion failed: %s (%s: %s: %d)\n", expr, file, func, line);
    fflush(NULL);
    abort();
}

const char* cutest_get_current_fixture(void)
{
    if (g_test_ctx.runtime.cur_node == NULL)
    {
        return NULL;
    }
    return g_test_ctx.runtime.cur_node->test_case->info.fixture;
}

const char* cutest_get_current_test(void)
{
    if (g_test_ctx.runtime.cur_node == NULL)
    {
        return NULL;
    }
    return g_test_ctx.runtime.cur_node->test_case->info.case_name;
}

int cutest_internal_assert_helper_str_eq(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}

int cutest_internal_assert_helper_float_eq(float a, float b)
{
    float_point_t v_a; v_a.value_ = a;
    float_point_t v_b; v_b.value_ = b;

    if (_test_float_point_is_nan(&v_a) || _test_float_point_is_nan(&v_b))
    {
        return 0;
    }

    return _test_float_point_distance_between_sign_and_magnitude_numbers(v_a.bits_, v_b.bits_)
        <= g_test_nature.precision.kMaxUlps;
}

int cutest_internal_assert_helper_float_le(float a, float b)
{
    return (a < b) || cutest_internal_assert_helper_float_eq(a, b);
}

int cutest_internal_assert_helper_float_ge(float a, float b)
{
    return (a > b) || cutest_internal_assert_helper_float_eq(a, b);
}

int cutest_internal_assert_helper_double_eq(double a, double b)
{
    double_point_t v_a; v_a.value_ = a;
    double_point_t v_b; v_b.value_ = b;

    if (_test_double_point_is_nan(&v_a) || _test_double_point_is_nan(&v_b))
    {
        return 0;
    }

    return _test_double_point_distance_between_sign_and_magnitude_numbers(v_a.bits_, v_b.bits_)
        <= g_test_nature.precision.kMaxUlps;
}

int cutest_internal_assert_helper_double_le(double a, double b)
{
    return (a < b) || cutest_internal_assert_helper_double_eq(a, b);
}

int cutest_internal_assert_helper_double_ge(double a, double b)
{
    return (a > b) || cutest_internal_assert_helper_double_eq(a, b);
}

void cutest_internal_assert_failure(void)
{
    if (g_test_ctx.runtime.tid != GET_TID())
    {
        /**
         * If current thread is NOT the main thread, it is dangerous to jump back
         * to caller stack, so we just abort the program.
         */
        abort();
    }
    else
    {
        longjmp(g_test_ctx.jmpbuf, MASK_FAILURE);
    }
}

void cutest_skip_test(void)
{
    SET_MASK(g_test_ctx.runtime.cur_node->mask, MASK_SKIPPED);
}

void cutest_internal_flush(void)
{
    fflush(NULL);
}

int cutest_internal_break_on_failure(void)
{
    return g_test_ctx.mask.break_on_failure;
}

/************************************************************************/
/* LOG                                                                  */
/************************************************************************/

const char* cutest_pretty_file(const char* file)
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

int cutest_printf(const char* fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = cutest_color_vfprintf(CUTEST_PRINT_COLOR_DEFAULT, _get_logfile(), fmt, ap);
    va_end(ap);

    return ret;
}
