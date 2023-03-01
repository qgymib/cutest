#include "string_matrix.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#if defined(_WIN32)
#   define strtok_r(s, d, c)    strtok_s(s, d, c)
#endif

static int _string_matrix_expand_rank(string_matrix_line_t* line)
{
    size_t new_rank_sz = line->rank_sz + 1;
    size_t new_size = sizeof(string_matrix_data_t) * new_rank_sz;
    string_matrix_data_t* new_rank = realloc(line->rank, new_size);

    if (new_rank == NULL)
    {
        return -1;
    }

    new_rank[new_rank_sz - 1].data = NULL;
    new_rank[new_rank_sz - 1].size = 0;

    line->rank_sz = new_rank_sz;
    line->rank = new_rank;

    return 0;
}

static int _string_matrix_append_line(string_matrix_t* matrix, const char* delim, char* str)
{
    char* saveptr = NULL;
    string_matrix_line_t* line = &matrix->line[matrix->line_sz - 1];

    char* pos;
    while ((pos = strtok_r(str, delim, &saveptr)) != NULL)
    {
        str = NULL;
        if (_string_matrix_expand_rank(line) != 0)
        {
            return -1;
        }

        line->rank[line->rank_sz - 1].data = pos;
        line->rank[line->rank_sz - 1].size = strlen(pos);
    }

    return 0;
}

static int _string_matrix_expand_line(string_matrix_t* matrix)
{
    size_t new_line_sz = matrix->line_sz + 1;
    size_t new_size = sizeof(string_matrix_line_t) * new_line_sz;
    string_matrix_line_t* new_line = realloc(matrix->line, new_size);

    if (new_line == NULL)
    {
        return -1;
    }

    new_line[new_line_sz - 1].rank = NULL;
    new_line[new_line_sz - 1].rank_sz = 0;

    matrix->line = new_line;
    matrix->line_sz = new_line_sz;

    return 0;
}

static int _string_matrix_create(string_matrix_t* matrix, const char* delim)
{
    char* str = matrix->raw;
    char* saveptr = NULL;

    char* pos;
    while ((pos = strtok_r(str, "\r\n", &saveptr)) != NULL)
    {
        str = NULL;
        if (_string_matrix_expand_line(matrix) != 0)
        {
            return -1;
        }

        if (_string_matrix_append_line(matrix, delim, pos) != 0)
        {
            return -1;
        }
    }

    return 0;
}

string_matrix_t* string_matrix_create(const char* str, const char* delim)
{
    size_t raw_sz = strlen(str) + 1;
    size_t malloc_size = sizeof(string_matrix_t) + raw_sz;
    string_matrix_t* matrix = malloc(malloc_size);
    if (matrix == NULL)
    {
        return NULL;
    }

    matrix->line = NULL;
    matrix->line_sz = 0;
    matrix->raw = (char*)(matrix + 1);
    matrix->raw_sz = raw_sz;
    memcpy(matrix->raw, str, raw_sz);

    if (_string_matrix_create(matrix, delim) != 0)
    {
        string_matrix_destroy(matrix);
        return NULL;
    }

    return matrix;
}

static string_matrix_t* _string_matrix_create_from_file_2(FILE* file, const char* delim)
{
    if (fseek(file, 0, SEEK_END) == -1)
    {
        return NULL;
    }

    long tell_ret = ftell(file);
    if (tell_ret == -1)
    {
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) == -1)
    {
        return NULL;
    }

    size_t malloc_size = sizeof(string_matrix_t) + tell_ret + 1;
    string_matrix_t* matrix = malloc(malloc_size);
    if (matrix == NULL)
    {
        return NULL;
    }

    matrix->line = NULL;
    matrix->line_sz = 0;
    matrix->raw = (char*)(matrix + 1);
    matrix->raw_sz = tell_ret + 1;

    if (fread(matrix->raw, 1, tell_ret, file) != (size_t)tell_ret)
    {
        free(matrix);
        return NULL;
    }
    matrix->raw[matrix->raw_sz - 1] = '\0';

    if (_string_matrix_create(matrix, delim) != 0)
    {
        string_matrix_destroy(matrix);
        return NULL;
    }

    return matrix;
}

string_matrix_t* string_matrix_create_from_file(FILE* file, const char* delim)
{
    long offset_bak = ftell(file);
    assert(file != NULL);

    string_matrix_t* matrix = _string_matrix_create_from_file_2(file, delim);
    if (fseek(file, offset_bak, SEEK_SET) == -1)
    {
        string_matrix_destroy(matrix);
        return NULL;
    }

    return matrix;
}

void string_matrix_destroy(string_matrix_t* matrix)
{
    size_t i;
    for (i = 0; i < matrix->line_sz; i++)
    {
        free(matrix->line[i].rank);
        matrix->line[i].rank = NULL;
        matrix->line[i].rank_sz = 0;
    }

    free(matrix->line);
    matrix->line = NULL;
    matrix->line_sz = 0;

    matrix->raw = NULL;
    matrix->raw_sz = 0;

    free(matrix);
}

const char* string_matrix_access(string_matrix_t* matrix, size_t line, size_t rank)
{
    return matrix->line[line].rank[rank].data;
}
