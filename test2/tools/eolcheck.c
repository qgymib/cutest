#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils/file.h"
#include "__init__.h"

typedef struct eolcheck_config
{
    const char* path;
    const char* eol;
} eolcheck_config_t;

static int _eol_check_parser(eolcheck_config_t* cfg, int argc, char* argv[])
{
    int i;
    const char* opt;

    cfg->eol = NULL;
    cfg->path = NULL;

    for (i = 0; i < argc; i++)
    {
        opt = "--file=";
        if (strncmp(argv[i], opt, strlen(opt)) == 0)
        {
            cfg->path = argv[i] + strlen(opt);
            continue;
        }

        opt = "--eol=";
        if (strncmp(argv[i], opt, strlen(opt)) == 0)
        {
            cfg->eol = argv[i] + strlen(opt);
            continue;
        }
    }

    if (cfg->path == NULL)
    {
        fprintf(stderr, "missing argument to `--file`.\n");
        return EXIT_FAILURE;
    }
    if (cfg->eol == NULL)
    {
        fprintf(stderr, "missing argument to `--eol`.\n");
        return EXIT_FAILURE;
    }

    if (strcmp(cfg->eol, "CR") != 0 && strcmp(cfg->eol, "LF") != 0 && strcmp(cfg->eol, "CRLF") != 0)
    {
        fprintf(stderr, "unknown option to `--eol`.\n");
        return EXIT_FAILURE;
    }

    return 0;
}

static int _eolcheck_is_match(char c, const char* delim)
{
    for (; *delim != '\0'; delim++)
    {
        if (*delim == c)
        {
            return 1;
        }
    }

    return 0;
}

static char* _eolcheck_strtok(char* str, const char* delim, char** saveptr)
{
    if (*saveptr == NULL)
    {
        *saveptr = str;
    }

    char* pos_start = *saveptr;

    for (; **saveptr != '\0'; *saveptr = *saveptr + 1)
    {
        if (_eolcheck_is_match(**saveptr, delim))
        {
            **saveptr = '\0';
            *saveptr = *saveptr + 1;
            return pos_start;
        }
    }

    return NULL;
}

static char _mmc_ascii_to_char(unsigned char c)
{
	if (c >= 32 && c <= 126)
	{
		return c;
	}
	return '.';
}

static void mmc_dump_hex(const void* data, size_t size, size_t width)
{
	const unsigned char* pdat = (unsigned char*)data;

	size_t idx_line;
	for (idx_line = 0; idx_line < size; idx_line += width)
	{
        fprintf(stdout, "%p: ", pdat + idx_line);

		/* printf hex */
        size_t idx_colume;
		for (idx_colume = 0; idx_colume < width; idx_colume++)
		{
			const char* postfix = (idx_colume < width - 1) ? "" : "|";

			if (idx_colume + idx_line < size)
			{
				fprintf(stdout, "%02x %s", pdat[idx_colume + idx_line], postfix);
			}
			else
			{
				fprintf(stdout, "   %s", postfix);
			}
		}
		fprintf(stdout, " ");
		/* printf char */
		for (idx_colume = 0; (idx_colume < width) && (idx_colume + idx_line < size); idx_colume++)
		{
			fprintf(stdout, "%c", _mmc_ascii_to_char(pdat[idx_colume + idx_line]));
		}
		fprintf(stdout, "\n");
	}
}

static int _eol_check_dat(const test_str_t* dat, const eolcheck_config_t* cfg)
{
    int ret = 0;
    test_str_t cpy = test_str_dup(dat);

    size_t cnt_line = 1;
    char* line = NULL;
    char* saveptr = NULL;

    for (; (line = _eolcheck_strtok(cpy.str, "\r\n", &saveptr)) != NULL; cnt_line++)
    {
        size_t line_sz = strlen(line);
        size_t line_offset = line - cpy.str;
        size_t eol_offset = line_offset + line_sz;

        switch (dat->str[eol_offset])
        {
        case '\r':
            if (eol_offset < dat->len && dat->str[eol_offset + 1] == '\n')
            {
                saveptr++;
                if (strcmp(cfg->eol, "CRLF") != 0)
                {
                    fprintf(stderr, "CRLF found on line %u in file `%s`.\n", (unsigned)cnt_line, cfg->path);
                    mmc_dump_hex(dat->str + line_offset, line_sz + 2, 16);
                    ret = EXIT_FAILURE;
                    goto fin;
                }
            }
            else
            {
                if (strcmp(cfg->eol, "CR") != 0)
                {
                    fprintf(stderr, "CR found on line %u in file `%s`.\n", (unsigned)cnt_line, cfg->path);
                    mmc_dump_hex(dat->str + line_offset, line_sz + 1, 16);
                    ret = EXIT_FAILURE;
                    goto fin;
                }
            }
            break;

        case '\n':
            if (strcmp(cfg->eol, "LF") != 0)
            {
                fprintf(stderr, "LF found on line %u in file `%s`.\n", (unsigned)cnt_line, cfg->path);
                mmc_dump_hex(dat->str + line_offset, line_sz + 1, 16);
                ret = EXIT_FAILURE;
                goto fin;
            }
            break;

        default:
            fprintf(stderr, "unexcept character `%c`.\n", dat->str[eol_offset]);
            abort();
        }
    }

fin:
    test_str_exit(&cpy);
    return ret;
}

static int _eol_check(int argc, char* argv[])
{
    eolcheck_config_t cfg;
    if (_eol_check_parser(&cfg, argc, argv) != 0)
    {
        return EXIT_FAILURE;
    }

    test_str_t dat = TEST_STR_INIT;
    if (test_file_read(cfg.path, &dat) != 0)
    {
        return EXIT_FAILURE;
    }

    int ret = _eol_check_dat(&dat, &cfg);
    test_str_exit(&dat);

    return ret;
}

const test_tool_t test_tool_eolcheck = {
"eolcheck", _eol_check,
"Check if a file contains all line ending.\n"
"--file=[PATH]\n"
"    Path to check.\n"
"--eol=CR|LF|CRLF\n"
"    Excepet line ending. CR (Macintosh) / LF (Unix) / CRLF (Windows)."
};
