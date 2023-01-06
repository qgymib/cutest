#ifndef __STRING_MATRIX_H__
#define __STRING_MATRIX_H__

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct string_matrix_data
{
	char*					data;		/**< Content. */
	size_t					size;		/**< Content size. */
} string_matrix_data_t;

typedef struct string_matrix_line
{
	string_matrix_data_t*	rank;		/**< Ranks. */
	size_t					rank_sz;	/**< Rank size. */
} string_matrix_line_t;

typedef struct string_matrix
{
	char*					raw;		/**< Payload data. */
	size_t					raw_sz;		/**< Payload data size in bytes. */

	string_matrix_line_t*	line;		/**< Lines */
	size_t					line_sz;	/**< Line size. */
} string_matrix_t;

/**
 * @brief Create string matrix.
 * @param[in] str		Source string.
 * @param[in] delim		Tokens.
 * @return				String matrix.
 */
string_matrix_t* string_matrix_create(const char* str, const char* delim);

/**
 * @brief Create string matrix from file.
 * @note The file position indicator for \p file is stay the same if string
 *   matrix create successfull. If string matrix create failed, the file
 *   position indicator for \p file is unknown.
 * @param[in] str		Source string.
 * @param[in] delim		Tokens.
 * @return				String matrix.
 */
string_matrix_t* string_matrix_create_from_file(FILE* file, const char* delim);

/**
 * @brief Destroy string matrix.
 * @param[in] matrix	String matrix instance.
 */
void string_matrix_destroy(string_matrix_t* matrix);

/**
 * @brief Access field.
 * @param[in] matrix	String matrix.
 * @param[in] line		Line.
 * @param[in] rank		Rank.
 * @return				String field.
 */
const char* string_matrix_access(string_matrix_t* matrix, size_t line, size_t rank);

#ifdef __cplusplus
}
#endif
#endif
