#ifndef MEMORY_H
#define MEMORY_H
#include <stddef.h>
#include <stdint.h>

/*
 * Allocation helpers for the row-of-pointers matrices used throughout the
 * library, plus the coverage counter type.
 *
 * A matrix is an array of r independently allocated rows, so m[i][j] works and
 * a row can be handed to a function on its own (get_col() takes one). Free
 * with the matching free_*, passing the SAME row count used to allocate --
 * nothing records it.
 */

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
 * All allocators return NULL on failure, when count * element-size would
 * overflow size_t, and for zero-sized requests. If a matrix row allocation
 * fails, the rows already allocated are released first.
 */
/* r x c ints, uninitialised. The workhorse: column-set tables (IToC) are
   allocated with get_matrix(binomial(k,t), t). */
int **get_matrix(size_t r, size_t c);

/* r ints, uninitialised. */
int *get_vector(size_t r);

/* Frees a matrix from get_matrix(). r must match the allocation. NULL-safe. */
void free_matrix(int **m, size_t r);

/* NULL-safe. */
void free_vector(int *v);

/* r x c coverage counters, zeroed. This is how covering_array_t::P is
   allocated: r = C(k,t) column-sets, c = v^t tuples. */
ca_count_t **get_matrix_count_calloc(size_t r, size_t c);

/* Frees a matrix from get_matrix_count_calloc(). NULL-safe. */
void free_matrix_count(ca_count_t **m, size_t r);

/* r x c bytes, uninitialised / zeroed. General-purpose byte matrices; the
   coverage matrix uses the ca_count_t pair above instead. */
uint8_t **get_matrix_uint8(size_t r, size_t c);
uint8_t **get_matrix_uint8_calloc(size_t r, size_t c);

/* Frees a matrix from either get_matrix_uint8* call. NULL-safe. */
void free_matrix_uint8(uint8_t **m, size_t r);

/* r bytes, uninitialised / zeroed, and the matching free. NULL-safe. */
uint8_t *get_vector_uint8(size_t r);
uint8_t *get_vector_uint8_calloc(size_t r);
void free_vector_uint8(uint8_t *v);

/* r size_t values, uninitialised, and the matching free. This is how
   covering_array_t::tcomb_counter is allocated. NULL-safe. */
size_t *get_vector_size_t(size_t r);
void free_vector_size_t(size_t *v);

#endif
