#ifndef MEMORY_H
#define MEMORY_H
#include <stddef.h>
#include <stdint.h>

/*
 * Coverage counter type.
 *
 * P[j][c] counts how many rows cover symbol-tuple c of column-set j, so its
 * largest possible value is the row count N. This was uint8_t, which wraps at
 * 256 rows and makes a covered combination read back as uncovered; array row
 * counts are therefore capped at CA_COUNT_MAX.
 */
typedef uint16_t ca_count_t;
#define CA_COUNT_MAX UINT16_MAX

/*
 * All allocators return NULL on failure (including a failed row of a matrix,
 * in which case the rows already taken are released first) and for zero-sized
 * requests -- a zero-row matrix hands back a pointer that cannot legally be
 * dereferenced, which is exactly what let out-of-range callers run off the end.
 */
int **get_matrix(size_t r, size_t c);
int *get_vector(size_t r);
void free_matrix(int **m, size_t r);
void free_vector(int *v);

ca_count_t **get_matrix_count_calloc(size_t r, size_t c);
void free_matrix_count(ca_count_t **m, size_t r);

uint8_t **get_matrix_uint8(size_t r, size_t c);
uint8_t **get_matrix_uint8_calloc(size_t r, size_t c);
void free_matrix_uint8(uint8_t **m, size_t r);

uint8_t *get_vector_uint8(size_t r);
uint8_t *get_vector_uint8_calloc(size_t r);
void free_vector_uint8(uint8_t *v);

size_t *get_vector_size_t(size_t r);
void free_vector_size_t(size_t *v);

#endif
