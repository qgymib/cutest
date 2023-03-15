#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "test.h"
#include "utils/strtok_f.h"
#include "utils/foreachline.h"

#if defined(_WIN32)
#define strdup(s)                       _strdup(s)
#endif

typedef struct doc_verify
{
    char*       h_main_page;
    size_t      h_main_page_sz;
    size_t      h_main_page_cap;

    char*       md_main_page;
} doc_verify_t;

static doc_verify_t g_doc_ctx;

static const char* _find_mainpage(size_t* size)
{
    const char* pattern =
        "/**\n"
        " * @mainpage CUnitTest\n";
    const char* start = strstr((const char*)cutest_h_hex, pattern);
    ASSERT_NE_PTR(start, NULL, "start pattern not found");

    start += strlen(pattern);

    pattern = "\n */\n";
    const char* end = strstr(start, pattern);
    ASSERT_NE_PTR(end, NULL, "end pattern not found");

    *size = end - start + 1;
    return start;
}

static void _find_main_page_in_header(void)
{
    const char* main_page = _find_mainpage(&g_doc_ctx.h_main_page_sz);
    g_doc_ctx.h_main_page_cap = g_doc_ctx.h_main_page_sz + 1;
    g_doc_ctx.h_main_page = malloc(g_doc_ctx.h_main_page_cap);

    memcpy(g_doc_ctx.h_main_page, main_page, g_doc_ctx.h_main_page_sz);
    g_doc_ctx.h_main_page[g_doc_ctx.h_main_page_sz] = '\0';
}

static void _generate_main_page_in_readmd(void)
{
    char* tmp = foreach_line_add((const char*)readme_md_hex, " * ", NULL);
    g_doc_ctx.md_main_page = foreach_line_remove_trailing_space(tmp);
    free(tmp);
}

TEST_FIXTURE_SETUP(doc)
{
    _find_main_page_in_header();
    _generate_main_page_in_readmd();
}

TEST_FIXTURE_TEARDOWN(doc)
{
    if (g_doc_ctx.h_main_page != NULL)
    {
        free(g_doc_ctx.h_main_page);
        g_doc_ctx.h_main_page = NULL;
    }
    g_doc_ctx.h_main_page_cap = 0;
    g_doc_ctx.h_main_page_sz = 0;

    if (g_doc_ctx.md_main_page != NULL)
    {
        free(g_doc_ctx.md_main_page);
        g_doc_ctx.md_main_page = NULL;
    }
}

TEST_F(doc, mainpage)
{
    ASSERT_EQ_INT(strcmp(g_doc_ctx.h_main_page, g_doc_ctx.md_main_page), 0,
        "Documents in `README.md' and `cutest.h' not synchronized.\n"
        "Please copy the following content into `cutest.h' as mainpage:\n"
        "/**\n"
        " * @mainpage CUnitTest\n"
        "%s"
        " */\n", g_doc_ctx.md_main_page);
}
