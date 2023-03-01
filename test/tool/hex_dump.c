#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define LINE_BYTES  16

static const char* s_help =
"--input=PATH\n"
"    Path to input file.\n"
"--output=PATH\n"
"    Path to output file.\n"
"--name=STRING\n"
"    Array name.\n"
"--help\n"
"    Show this help and exit.\n";

typedef struct hex_ctx
{
    const char* input_path;
    const char* output_path;
    const char* name;

    FILE*       input_file;
    FILE*       output_file;
    size_t      total_write;

    char        buf[LINE_BYTES];
} hex_ctx_t;

static hex_ctx_t g_ctx;

#if !defined(_WIN32)
static int fopen_s(FILE** out, const char* path, const char* mode)
{
    if ((*out = fopen(path, mode)) == NULL)
    {
        return errno;
    }
    return 0;
}
#endif

static void _setup(int argc, char* argv[])
{
    int i;
    int ret;
    const char* opt;

    for (i = 0; i < argc; i++)
    {
        opt = "--input=";
        if (strncmp(argv[i], opt, strlen(opt)) == 0)
        {
            g_ctx.input_path = argv[i] + strlen(opt);
            continue;
        }

        opt = "--output=";
        if (strncmp(argv[i], opt, strlen(opt)) == 0)
        {
            g_ctx.output_path = argv[i] + strlen(opt);
            continue;
        }

        opt = "--name=";
        if (strncmp(argv[i], opt, strlen(opt)) == 0)
        {
            g_ctx.name = argv[i] + strlen(opt);
            continue;
        }

        if (strcmp(argv[i], "--help") == 0)
        {
            printf("%s", s_help);
            exit(0);
        }
    }

    if (g_ctx.input_path == NULL)
    {
        fprintf(stderr, "missing argument `--input='.\n");
        exit(EXIT_FAILURE);
    }
    if (g_ctx.output_path == NULL)
    {
        fprintf(stderr, "missing argument `--output='.\n");
        exit(EXIT_FAILURE);
    }

    if ((ret = fopen_s(&g_ctx.input_file, g_ctx.input_path, "rb")) != 0)
    {
        fprintf(stderr, "cannot open %s: %d.\n", g_ctx.input_path, ret);
        exit(EXIT_FAILURE);
    }

    if ((ret = fopen_s(&g_ctx.output_file, g_ctx.output_path, "wb")) != 0)
    {
        fprintf(stderr, "cannot open %s: %d.\n", g_ctx.output_path, ret);
        exit(EXIT_FAILURE);
    }
}

static void _at_exit(void)
{
    if (g_ctx.input_file != NULL)
    {
        fclose(g_ctx.input_file);
        g_ctx.input_file = NULL;
    }

    if (g_ctx.output_file != NULL)
    {
        fclose(g_ctx.output_file);
        g_ctx.output_file = NULL;
    }
}

static void _dump_line(FILE* dst, const void* data, size_t size)
{
    size_t i;
    const unsigned char* pdat = data;

    for (i = 0; i < size; i++)
    {
        fprintf(dst, "0x%02x,", pdat[i]);
        g_ctx.total_write++;

        if (g_ctx.total_write % LINE_BYTES != 0)
        {
            fprintf(dst, " ");
        }
        else
        {
            fprintf(dst, "\n");
        }
    }
}

static void _dump_file(void)
{
    fprintf(g_ctx.output_file,
        "#include <stdint.h>\n"
        "#include <stddef.h>\n"
        "const uint8_t %s[] = {\n", g_ctx.name);

    while (!feof(g_ctx.input_file))
    {
        if (ferror(g_ctx.input_file))
        {
            fprintf(stderr, "error when reading %s\n.", g_ctx.input_path);
            exit(EXIT_FAILURE);
        }

        size_t nread = fread(g_ctx.buf, 1, sizeof(g_ctx.buf), g_ctx.input_file);
        _dump_line(g_ctx.output_file, g_ctx.buf, nread);
    }

    fprintf(g_ctx.output_file,
        "0x00, };\n"
        "const size_t %s_sz = %zu;\n", g_ctx.name, g_ctx.total_write);
}

int main(int argc, char* argv[])
{
    atexit(_at_exit);
    _setup(argc, argv);

    _dump_file();

    return 0;
}
