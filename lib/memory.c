#include "memory.h"
#include <stdint.h>
#include <stdlib.h>

static void *malloc_array(size_t count, size_t element_size) {
  if (count == 0 || element_size == 0 || count > SIZE_MAX / element_size) {
    return NULL;
  }
  return malloc(count * element_size);
}

static void *calloc_array(size_t count, size_t element_size) {
  if (count == 0 || element_size == 0 || count > SIZE_MAX / element_size) {
    return NULL;
  }
  return calloc(count, element_size);
}

/*
 * Allocates a 2D matrix of int with r rows and c columns.
 * Each element is uninitialized (malloc).
 * Returns NULL if r or c is zero or any allocation fails.
 * Caller must free with free_matrix().
 */
int **get_matrix(size_t r, size_t c) {
  if (r == 0 || c == 0) {
    return NULL;
  }
  int **m = (int **)malloc_array(r, sizeof(int *));
  if (m == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < r; i++) {
    m[i] = (int *)malloc_array(c, sizeof(int));
    if (m[i] == NULL) {
      for (size_t j = 0; j < i; j++) {
        free(m[j]);
      }
      free(m);
      return NULL;
    }
  }
  return m;
}

/*
 * Allocates a 1D vector of int with r elements.
 * Each element is uninitialized (malloc).
 * Caller must free with free_vector().
 */
int *get_vector(size_t r) {
  if (r == 0) {
    return NULL;
  }
  return (int *)malloc_array(r, sizeof(int));
}

/*
 * Frees a 2D matrix allocated with get_matrix().
 * Iterates through all rows before freeing the outer array.
 */
void free_matrix(int **m, size_t r) {
  if (m == NULL) {
    return;
  }
  for (size_t i = 0; i < r; i++) {
    free(m[i]);
  }
  free(m);
}

/*
 * Frees a 1D vector allocated with get_vector().
 */
void free_vector(int *v) { free(v); }

/*
 * Allocates a 2D matrix of coverage counters, zero-initialized.
 * Returns NULL if r or c is zero or any allocation fails.
 * Caller must free with free_matrix_count().
 */
ca_count_t **get_matrix_count_calloc(size_t r, size_t c) {
  if (r == 0 || c == 0) {
    return NULL;
  }
  ca_count_t **m =
      (ca_count_t **)malloc_array(r, sizeof(ca_count_t *));
  if (m == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < r; i++) {
    m[i] = (ca_count_t *)calloc_array(c, sizeof(ca_count_t));
    if (m[i] == NULL) {
      for (size_t j = 0; j < i; j++) {
        free(m[j]);
      }
      free(m);
      return NULL;
    }
  }
  return m;
}

/*
 * Frees a 2D matrix allocated with get_matrix_count_calloc().
 */
void free_matrix_count(ca_count_t **m, size_t r) {
  if (m == NULL) {
    return;
  }
  for (size_t i = 0; i < r; i++) {
    free(m[i]);
  }
  free(m);
}

/*
 * Allocates a 2D matrix of uint8_t with r rows and c columns.
 * Each element is uninitialized (malloc).
 * Returns NULL if r or c is zero or any allocation fails.
 * Caller must free with free_matrix_uint8().
 */
uint8_t **get_matrix_uint8(size_t r, size_t c) {
  if (r == 0 || c == 0) {
    return NULL;
  }
  uint8_t **m = (uint8_t **)malloc_array(r, sizeof(uint8_t *));
  if (m == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < r; i++) {
    m[i] = (uint8_t *)malloc_array(c, sizeof(uint8_t));
    if (m[i] == NULL) {
      for (size_t j = 0; j < i; j++) {
        free(m[j]);
      }
      free(m);
      return NULL;
    }
  }
  return m;
}

/*
 * Allocates a 2D matrix of uint8_t with r rows and c columns.
 * Each element is zero-initialized (calloc).
 * Returns NULL if r or c is zero or any allocation fails.
 * Caller must free with free_matrix_uint8().
 */
uint8_t **get_matrix_uint8_calloc(size_t r, size_t c) {
  if (r == 0 || c == 0) {
    return NULL;
  }
  uint8_t **m = (uint8_t **)malloc_array(r, sizeof(uint8_t *));
  if (m == NULL) {
    return NULL;
  }
  for (size_t i = 0; i < r; i++) {
    m[i] = (uint8_t *)calloc_array(c, sizeof(uint8_t));
    if (m[i] == NULL) {
      for (size_t j = 0; j < i; j++) {
        free(m[j]);
      }
      free(m);
      return NULL;
    }
  }
  return m;
}

/*
 * Frees a 2D matrix allocated with get_matrix_uint8() or
 * get_matrix_uint8_calloc(). Iterates through all rows before freeing the
 * outer array.
 */
void free_matrix_uint8(uint8_t **m, size_t r) {
  if (m == NULL) {
    return;
  }
  for (size_t i = 0; i < r; i++) {
    free(m[i]);
  }
  free(m);
}

/*
 * Allocates a 1D vector of uint8_t with r elements.
 * Each element is uninitialized (malloc).
 * Caller must free with free_vector_uint8().
 */
uint8_t *get_vector_uint8(size_t r) {
  if (r == 0) {
    return NULL;
  }
  return (uint8_t *)malloc_array(r, sizeof(uint8_t));
}

/*
 * Allocates a 1D vector of uint8_t with r elements.
 * Each element is zero-initialized (calloc).
 * Caller must free with free_vector_uint8().
 */
uint8_t *get_vector_uint8_calloc(size_t r) {
  if (r == 0) {
    return NULL;
  }
  return (uint8_t *)calloc_array(r, sizeof(uint8_t));
}

/*
 * Frees a 1D vector allocated with get_vector_uint8() or
 * get_vector_uint8_calloc().
 */
void free_vector_uint8(uint8_t *v) { free(v); }

/*
 * Allocates a 1D vector of size_t with r elements.
 * Each element is uninitialized (malloc).
 * Caller must free with free_vector_size_t().
 */
size_t *get_vector_size_t(size_t r) {
  if (r == 0) {
    return NULL;
  }
  return (size_t *)malloc_array(r, sizeof(size_t));
}

/*
 * Frees a 1D vector allocated with get_vector_size_t().
 */
void free_vector_size_t(size_t *v) { free(v); }
