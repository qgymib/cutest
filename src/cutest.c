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

#define COLOR_GREEN(str)                    "@G" str "@D"
#define COLOR_RED(str)                      "@R" str "@D"
#define COLOR_YELLO(str)                    "@Y" str "@D"
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

/************************************************************************/
/* test                                                                 */
/************************************************************************/

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

#define TEST_CASE_TABLE_INIT    CUTEST_MAP_INIT(_test_on_cmp_case, NULL)

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

typedef enum test_parameterized_type
{
#define EXPAND_TEST_PARAMETERIZED_AS_ENUM(type, ign0, ign1)    type,
    TEST_PARAMETERIZED_MAP(EXPAND_TEST_PARAMETERIZED_AS_ENUM)
#undef EXPAND_TEST_PARAMETERIZED_AS_ENUM
    TEST_PARAMETERIZED_TYPE_UNKNOWN,
}test_parameterized_type_t;

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
    stage_setup = 0,
    stage_run,
    stage_teardown,
}test_case_stage_t;

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
        cutest_map_node_t*  cur_it;                         /**< Current cursor position */
        cutest_case_t*      cur_case;                       /**< Current running test case */
        unsigned            cur_parameterized_idx;          /**< Current parameterized index */
        test_case_stage_t   cur_stage;                      /**< Current running stage */
    }runtime;

    struct
    {
        cutest_timestamp_t  tv_case_start;                  /**< Test start time */
        cutest_timestamp_t  tv_case_end;                    /**< Test end time */

        cutest_timestamp_t  tv_total_start;                 /**< The start time of whole test */
        cutest_timestamp_t  tv_total_end;                   /**< The end time of whole test */

        cutest_timestamp_t  tv_diff;                        /**< Time diff */
    }timestamp;

    struct
    {
        struct
        {
            unsigned        total;                          /**< The number of total running cases */
            unsigned        disabled;                       /**< The number of disabled cases */
            unsigned        success;                        /**< The number of successed cases */
            unsigned        skipped;                        /**< The number of skipped cases */
            unsigned        failed;                         /**< The number of failed cases */
        }result;

        struct
        {
            unsigned        repeat;                         /**< How many times need to repeat */
            unsigned        repeated;                       /**< How many times alread repeated */
        }repeat;
    }counter;

    struct
    {
        unsigned            break_on_failure : 1;           /**< DebugBreak when failure */
        unsigned            no_print_time : 1;              /**< Whether to print execution cost time */
        unsigned            also_run_disabled_tests : 1;    /**< Also run disabled tests */
        unsigned            shuffle : 1;                    /**< Randomize running cases */
    }mask;

    struct
    {
        char**              positive_patterns;              /**< positive patterns for filter */
        char**              negative_patterns;              /**< negative patterns for filter */
        size_t              n_negative;                     /**< The number of negative patterns */
        size_t              n_postive;                      /**< The number of positive patterns */
    }filter;                                                /**< Context for float/double compare */

    struct
    {
        FILE*               f_out;                          /**< Output file */
        int                 need_close;                     /**< Need close */
    }io;

    const cutest_hook_t*    hook;
}test_ctx_t;

typedef struct test_ctx2
{
    char                    strbuf[512];                    /**< String buffer */
    jmp_buf                 jmpbuf;                         /**< Jump buffer */
}test_ctx2_t;

static int _test_on_cmp_case(const cutest_map_node_t* key1, const cutest_map_node_t* key2, void* arg)
{
    (void)arg;
    const cutest_case_t* t_case_1 = CONTAINER_OF(key1, cutest_case_t, node_table);
    const cutest_case_t* t_case_2 = CONTAINER_OF(key2, cutest_case_t, node_table);

    if (t_case_1->cache.randkey < t_case_2->cache.randkey)
    {
        return -1;
    }
    if (t_case_1->cache.randkey > t_case_2->cache.randkey)
    {
        return 1;
    }
    return strcmp(t_case_1->info.full_name, t_case_2->info.full_name);
}

static test_ctx2_t          g_test_ctx2;                                // no need to initialize
static test_ctx_t           g_test_ctx = {
    { 0, 0, NULL, NULL, 0, stage_setup },                               /* .runtime */
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },               /* .timestamp */
    { { 0, 0, 0, 0, 0 }, { 0, 0 } },                                    /* .counter */
    { 0, 0, 0, 0 },                                                     /* .mask */
    { NULL, NULL, 0, 0 },                                               /* .filter */
    { NULL, 0 },                                                        /* .io */
    NULL,                                                               /* .hook */
};
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
static WORD _test_get_color_attribute(cutest_print_color_t color)
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

static WORD _test_get_new_color(cutest_print_color_t color, WORD old_color_attrs)
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
    for (i = 0; i < g_test_ctx.filter.n_negative; i++)
    {
        if (_test_pattern_matches_string(g_test_ctx.filter.negative_patterns[i], str))
        {
            return 0;
        }
    }

    if (g_test_ctx.filter.n_postive == 0)
    {
        return 1;
    }

    for (i = 0; i < g_test_ctx.filter.n_postive; i++)
    {
        if (_test_pattern_matches_string(g_test_ctx.filter.positive_patterns[i], str))
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
    return !g_test_ctx.mask.also_run_disabled_tests && (strncmp("DISABLED_", name, 9) == 0);
}

#if defined(_WIN32)

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

#endif

static int _should_use_color(int is_tty)
{
#if defined(_WIN32)
    /**
     * On Windows the TERM variable is usually not set, but the console there
     * does support colors.
     */
    return is_tty;
#else
    static pthread_once_t once_control = PTHREAD_ONCE_INIT;
    pthread_once(&once_control, _initlize_color_unix);

    return is_tty && g_color_printf.terminal_color_support;
#endif
}

static int _print_encoded(FILE* stream, const char* str)
{
    char* str_tmp;
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

        str_tmp = strndup(str, p - str);
        ret += cutest_color_fprintf(color, stream, "%s", str_tmp);
        free(str_tmp);

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
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_fixture_setup == NULL)
    {
        return;
    }
    g_test_ctx.hook->before_fixture_setup(test_case->info.suit_name);
}

static void _test_hook_after_fixture_setup(cutest_case_t* test_case, int ret)
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

static void _test_hook_before_normal_test(cutest_case_t* test_case)
{
    switch (test_case->info.type)
    {
    case CUTEST_CASE_TYPE_SIMPLE:
        if (g_test_ctx.hook != NULL && g_test_ctx.hook->before_simple_test != NULL)
        {
            g_test_ctx.hook->before_simple_test(test_case->info.suit_name, test_case->info.case_name);
        }
        break;

    case CUTEST_CASE_TYPE_FIXTURE:
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

static void _test_hook_after_normal_test(cutest_case_t* test_case, int ret)
{
    switch (test_case->info.type)
    {
    case CUTEST_CASE_TYPE_SIMPLE:
        if (g_test_ctx.hook != NULL && g_test_ctx.hook->after_simple_test != NULL)
        {
            g_test_ctx.hook->after_simple_test(test_case->info.suit_name, test_case->info.case_name, ret);
        }
        break;

    case CUTEST_CASE_TYPE_FIXTURE:
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

static void _test_hook_before_parameterized_test(cutest_case_t* test_case, unsigned index)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_parameterized_test == NULL)
    {
        return;
    }
    g_test_ctx.hook->before_parameterized_test(test_case->info.suit_name,
        test_case->info.case_name, index, (unsigned)test_case->parameterized.n_dat);
}

static void _test_hook_after_parameterized_test(cutest_case_t* test_case, unsigned index, int ret)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->after_parameterized_test == NULL)
    {
        return;
    }
    g_test_ctx.hook->after_parameterized_test(test_case->info.suit_name,
        test_case->info.case_name, index, (unsigned)test_case->parameterized.n_dat, ret);
}

static void _test_fixture_run_body(void)
{
    if (g_test_ctx.runtime.cur_case->info.type != CUTEST_CASE_TYPE_PARAMETERIZED)
    {
        _test_hook_before_normal_test(g_test_ctx.runtime.cur_case);
        ((cutest_procedure_fn)g_test_ctx.runtime.cur_case->stage.body)();
        _test_hook_after_normal_test(g_test_ctx.runtime.cur_case, 0);
    }
    else
    {
        for (g_test_ctx.runtime.cur_parameterized_idx = 0;
            g_test_ctx.runtime.cur_parameterized_idx < g_test_ctx.runtime.cur_case->parameterized.n_dat;
            g_test_ctx.runtime.cur_parameterized_idx++)
        {
            _test_hook_before_parameterized_test(g_test_ctx.runtime.cur_case, g_test_ctx.runtime.cur_parameterized_idx);
            ((cutest_parameterized_fn)g_test_ctx.runtime.cur_case->stage.body)(g_test_ctx.runtime.cur_case->parameterized.p_dat);
            _test_hook_after_parameterized_test(g_test_ctx.runtime.cur_case, g_test_ctx.runtime.cur_parameterized_idx, -1);
        }
    }
}

static void _test_hook_before_fixture_teardown(cutest_case_t* test_case)
{
    if (g_test_ctx.hook == NULL || g_test_ctx.hook->before_fixture_teardown == NULL)
    {
        return;
    }

    g_test_ctx.hook->before_fixture_teardown(test_case->info.suit_name);
}

static void _test_hook_after_fixture_teardown(cutest_case_t* test_case, int ret)
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

    _cutest_color_printf(CUTEST_PRINT_COLOR_GREEN, "[ RUN      ]");
    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %s\n", g_test_ctx2.strbuf);

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
    ASSERT(cutest_timestamp_get(&g_test_ctx.timestamp.tv_case_start) == 0);

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
        if (g_test_ctx.runtime.cur_case->info.type != CUTEST_CASE_TYPE_PARAMETERIZED)
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
    ASSERT(cutest_timestamp_get(&g_test_ctx.timestamp.tv_case_end) == 0);

    /* teardown */
    g_test_ctx.runtime.cur_stage = stage_teardown;
    _test_fixture_run_teardown();

procedure_teardown_fin:
    cutest_timestamp_dif(&g_test_ctx.timestamp.tv_case_start, &g_test_ctx.timestamp.tv_case_end, &g_test_ctx.timestamp.tv_diff);

    if (HAS_MASK(g_test_ctx.runtime.cur_case->info.mask, MASK_FAILURE))
    {
        g_test_ctx.counter.result.failed++;
        _cutest_color_printf(CUTEST_PRINT_COLOR_RED, "[  FAILED  ]");
    }
    else if (HAS_MASK(g_test_ctx.runtime.cur_case->info.mask, MASK_SKIPPED))
    {
        g_test_ctx.counter.result.skipped++;
        _cutest_color_printf(CUTEST_PRINT_COLOR_YELLOW, "[   SKIP   ]");
    }
    else
    {
        g_test_ctx.counter.result.success++;
        _cutest_color_printf(CUTEST_PRINT_COLOR_GREEN, "[       OK ]");
    }

    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %s", g_test_ctx2.strbuf);
    if (!g_test_ctx.mask.no_print_time)
    {
        unsigned long take_time = (unsigned long)(g_test_ctx.timestamp.tv_diff.sec * 1000
            + g_test_ctx.timestamp.tv_diff.usec / 1000);
        _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " (%lu ms)", take_time);
    }
    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "\n");
}

static void _test_reset_all_test(void)
{
    memset(&g_test_ctx.counter.result, 0, sizeof(g_test_ctx.counter.result));
    memset(&g_test_ctx.timestamp, 0, sizeof(g_test_ctx.timestamp));

    cutest_map_node_t* it = cutest_map_begin(&g_test_nature.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_case_t* case_data = CONTAINER_OF(it, cutest_case_t, node_table);
        case_data->info.mask = 0;
    }
}

static void _test_show_report_failed(void)
{
    cutest_map_node_t* it = cutest_map_begin(&g_test_nature.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_case_t* case_data = CONTAINER_OF(it, cutest_case_t, node_table);
        if (!HAS_MASK(case_data->info.mask, MASK_FAILURE))
        {
            continue;
        }

        snprintf(g_test_ctx2.strbuf, sizeof(g_test_ctx2.strbuf), "%s.%s",
            case_data->info.suit_name, case_data->info.case_name);

        _cutest_color_printf(CUTEST_PRINT_COLOR_RED, "[  FAILED  ]");
        _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %s\n", g_test_ctx2.strbuf);
    }
}

static void _test_show_report(void)
{
    cutest_timestamp_dif(&g_test_ctx.timestamp.tv_total_start,
        &g_test_ctx.timestamp.tv_total_end, &g_test_ctx.timestamp.tv_diff);

    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "[==========]");
    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " %u/%u test case%s ran.",
        g_test_ctx.counter.result.total,
        (unsigned)g_test_nature.case_table.size,
        g_test_ctx.counter.result.total > 1 ? "s" : "");
    if (!g_test_ctx.mask.no_print_time)
    {
        unsigned long take_time = (unsigned long)(g_test_ctx.timestamp.tv_diff.sec * 1000
            + g_test_ctx.timestamp.tv_diff.usec / 1000);
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

static void _test_list_tests_gen_param_info(char* buffer, size_t size, test_parameterized_type_t param_type, const cutest_case_t* case_data, size_t idx)
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

static void _test_list_tests_print_name(const cutest_case_t* case_data)
{
    char buffer[256];
    if (case_data->info.type != CUTEST_CASE_TYPE_PARAMETERIZED)
    {
        _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "  %s\n", case_data->info.case_name);
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

        _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "  %s/%lu  # TEST_GET_PARAM() = %s\n",
            case_data->info.case_name, (unsigned long)i, buffer);
    }
}

static void _test_list_tests(void)
{
    const char* last_class_name = "";

    cutest_map_node_t* it = cutest_map_begin(&g_test_nature.case_table);
    for (; it != NULL; it = cutest_map_next(it))
    {
        cutest_case_t* case_data = CONTAINER_OF(it, cutest_case_t, node_table);
        /* some compiler will make same string with different address */
        if (last_class_name != case_data->info.suit_name
            && strcmp(last_class_name, case_data->info.suit_name) != 0)
        {
            last_class_name = case_data->info.suit_name;
            _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "%s.\n", last_class_name);
        }
        _test_list_tests_print_name(case_data);
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
        cutest_case_t* test_case = CONTAINER_OF(it, cutest_case_t, node_table);
        it = cutest_map_next(it);
        ev_map_erase(&g_test_nature.case_table, &test_case->node_table);

        test_case->cache.randkey = _test_rand();
        cutest_map_insert(&copy_case_table, &test_case->node_table);
    }
    g_test_nature.case_table = copy_case_table;
}

#if defined(_WIN32)
static BOOL WINAPI _test_setup_once_body(PINIT_ONCE InitOnce, PVOID Parameter, PVOID* Context)
{
    (void)InitOnce; (void)Parameter; (void)Context;
#else
static void _test_setup_once_body(void)
{
#endif

    _test_setup_precision();

#if defined(_WIN32)
    return TRUE;
}
#else
}
#endif

/**
 * @brief Setup resources that will not change.
 */
static void cutest_setup_once(void)
{
#if defined(_WIN32)
    static INIT_ONCE token = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce(&token, _test_setup_once_body, NULL, NULL);
#else
    static pthread_once_t once_control = PTHREAD_ONCE_INIT;
    pthread_once(&once_control, _test_setup_once_body);
#endif
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

static void _test_run_test_loop(void)
{
    _test_reset_all_test();

    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, "[==========]");
    _cutest_color_printf(CUTEST_PRINT_COLOR_DEFAULT, " total %u test%s registered.\n",
        (unsigned)g_test_nature.case_table.size,
        g_test_nature.case_table.size > 1 ? "s" : "");

    cutest_timestamp_get(&g_test_ctx.timestamp.tv_total_start);

    g_test_ctx.runtime.cur_it = cutest_map_begin(&g_test_nature.case_table);
    for (; g_test_ctx.runtime.cur_it != NULL;
        g_test_ctx.runtime.cur_it = cutest_map_next(g_test_ctx.runtime.cur_it))
    {
        g_test_ctx.runtime.cur_case = CONTAINER_OF(g_test_ctx.runtime.cur_it, cutest_case_t, node_table);
        _test_run_case();
    }

    cutest_timestamp_get(&g_test_ctx.timestamp.tv_total_end);

    _test_show_report();
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
    memset(&g_test_ctx.timestamp, 0, sizeof(g_test_ctx.timestamp));
    memset(&g_test_ctx.counter, 0, sizeof(g_test_ctx.counter));
    memset(&g_test_ctx.mask, 0, sizeof(g_test_ctx.mask));

    if (g_test_ctx.filter.positive_patterns != NULL)
    {
        free(g_test_ctx.filter.positive_patterns);
        memset(&g_test_ctx.filter, 0, sizeof(g_test_ctx.filter));
    }

    _test_close_logfile();
    g_test_ctx.hook = NULL;
}

static void _test_run_tests_condition(void)
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

        _test_run_test_loop();

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

static const char* _cutest_get_log_level_str(cutest_log_level_t level)
{
    switch(level)
    {
    case CUTEST_LOG_DEBUG:
        return "D";
    case CUTEST_LOG_INFO:
        return "I";
    case CUTEST_LOG_WARN:
        return "W";
    case CUTEST_LOG_ERROR:
        return "E";
    case CUTEST_LOG_FATAL:
        return "F";
    default:
        break;
    }
    return "U";
}

static void _cutest_default_log(cutest_log_meta_t* info, const char* fmt, va_list ap, FILE* out)
{
    cutest_color_fprintf(CUTEST_PRINT_COLOR_DEFAULT, out, "[%s %s:%d] ",
            _cutest_get_log_level_str(info->leve), info->file, info->line);
    cutest_color_vfprintf(CUTEST_PRINT_COLOR_DEFAULT, out, fmt, ap);
    cutest_color_fprintf(CUTEST_PRINT_COLOR_DEFAULT, out, "\n");
}

int cutest_timestamp_get(cutest_timestamp_t* ts)
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

int cutest_register_case(cutest_case_t* data)
{
    /**
     * Theoretically this operation will not fail. If two tests have duplicated
     * name, it will fail at program link stage.
     */
    return cutest_map_insert(&g_test_nature.case_table, &data->node_table);
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
    _test_run_tests_condition();

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

const char* cutest_get_current_suit_name(void)
{
    if (g_test_ctx.runtime.cur_case == NULL)
    {
        return NULL;
    }
    return g_test_ctx.runtime.cur_case->info.suit_name;
}

const char* cutest_get_current_case_name(void)
{
    if (g_test_ctx.runtime.cur_case == NULL)
    {
        return NULL;
    }
    return g_test_ctx.runtime.cur_case->info.case_name;
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

unsigned cutest_internal_parameterized_index(void)
{
    return g_test_ctx.runtime.cur_parameterized_idx;
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
        ASSERT(g_test_ctx.runtime.cur_stage != stage_teardown);
        longjmp(g_test_ctx2.jmpbuf, MASK_FAILURE);
    }
}

void cutest_skip_test(void)
{
    ASSERT(g_test_ctx.runtime.cur_stage == stage_setup);
    SET_MASK(g_test_ctx.runtime.cur_case->info.mask, MASK_SKIPPED);
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
    ret = cutest_color_vfprintf(CUTEST_PRINT_COLOR_DEFAULT, _get_logfile(),
        fmt, ap);
    va_end(ap);

    return ret;
}

int cutest_color_fprintf(cutest_print_color_t color, FILE* stream, const char* fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = cutest_color_vfprintf(color, stream, fmt, ap);
    va_end(ap);

    return ret;
}

int cutest_color_vfprintf(cutest_print_color_t color, FILE* stream, const char* fmt, va_list ap)
{
    assert(stream != NULL);

    int stream_fd = fileno(stream);
    if (!_should_use_color(isatty(stream_fd)) || (color == CUTEST_PRINT_COLOR_DEFAULT))
    {
        return vfprintf(stream, fmt, ap);
    }

    int ret;
#if defined(_WIN32)
    const HANDLE stdout_handle = (HANDLE)_get_osfhandle(stream_fd);

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
#else
    fprintf(stream, "\033[0;%sm", _get_ansi_color_code_fg(color));
    ret = vfprintf(stream, fmt, ap);
    fprintf(stream, "\033[m");  // Resets the terminal to default.
    fflush(stream);
#endif

    return ret;
}

void cutest_log(cutest_log_meta_t* info, const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if (g_test_ctx.hook != NULL && g_test_ctx.hook->on_log_print != NULL)
    {
        g_test_ctx.hook->on_log_print(info, fmt, ap, _get_logfile());
    }
    else
    {
        _cutest_default_log(info, fmt, ap, _get_logfile());
    }
    va_end(ap);
}
