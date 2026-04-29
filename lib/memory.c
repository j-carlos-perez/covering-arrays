#include "memory.h"
#include <stdlib.h>

/*
 * Allocates a 2D matrix of int with r rows and c columns.
 * Each element is uninitialized (malloc).
 * Caller must free with free_matrix().
 */
int **get_matrix(size_t r, size_t c) {
  int **m;
  m = (int **)malloc(r * sizeof(int *));
  for (size_t i = 0; i < r; i++) {
    m[i] = malloc(c * sizeof(int));
  }
  return m;
}

/*
 * Allocates a 1D vector of int with r elements.
 * Each element is uninitialized (malloc).
 * Caller must free with free_vector().
 */
int *get_vector(size_t r) {
  int *v;
  v = (int *)malloc(r * sizeof(int));
  return v;
}

/*
 * Frees a 2D matrix allocated with get_matrix().
 * Iterates through all rows before freeing the outer array.
 */
void free_matrix(int **m, size_t r) {
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
 * Allocates a 2D matrix of uint8_t with r rows and c columns.
 * Each element is uninitialized (malloc).
 * Caller must free with free_matrix_uint8().
 */
uint8_t **get_matrix_uint8(size_t r, size_t c) {
  uint8_t **m;
  m = (uint8_t **)malloc(r * sizeof(uint8_t *));
  for (size_t i = 0; i < r; i++) {
    m[i] = malloc(c * sizeof(uint8_t));
  }
  return m;
}

/*
 * Allocates a 2D matrix of uint8_t with r rows and c columns.
 * Each element is zero-initialized (calloc).
 * Caller must free with free_matrix_uint8().
 */
uint8_t **get_matrix_uint8_calloc(size_t r, size_t c) {
  uint8_t **m;
  m = (uint8_t **)malloc(r * sizeof(uint8_t *));
  for (size_t i = 0; i < r; i++) {
    m[i] = calloc(c, sizeof(uint8_t));
  }
  return m;
}

/*
 * Frees a 2D matrix allocated with get_matrix_uint8() or get_matrix_uint8_calloc().
 * Iterates through all rows before freeing the outer array.
 */
void free_matrix_uint8(uint8_t **m, size_t r) {
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
  return (uint8_t *)malloc(r * sizeof(uint8_t));
}

/*
 * Allocates a 1D vector of uint8_t with r elements.
 * Each element is zero-initialized (calloc).
 * Caller must free with free_vector_uint8().
 */
uint8_t *get_vector_uint8_calloc(size_t r) {
  return (uint8_t *)calloc(r, sizeof(uint8_t));
}

/*
 * Frees a 1D vector allocated with get_vector_uint8() or get_vector_uint8_calloc().
 */
void free_vector_uint8(uint8_t *v) { free(v); }

/*
 * Allocates a 1D vector of size_t with r elements.
 * Each element is uninitialized (malloc).
 * Caller must free with free_vector_size_t().
 */
size_t *get_vector_size_t(size_t r) {
  return (size_t *)malloc(r * sizeof(size_t));
}

/*
 * Frees a 1D vector allocated with get_vector_size_t().
 */
void free_vector_size_t(size_t *v) { free(v); }