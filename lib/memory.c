#include "memory.h"
#include <stdlib.h>

int **get_matrix(size_t r, size_t c) {
  int **m;
  m = (int **)malloc(r * sizeof(int *));
  for (size_t i = 0; i < r; i++) {
    m[i] = malloc(c * sizeof(int));
  }
  return m;
}

int *get_vector(size_t r) {
  int *v;
  v = (int *)malloc(r * sizeof(int));
  return v;
}

void free_matrix(int **m, size_t r) {
  for (size_t i = 0; i < r; i++) {
    free(m[i]);
  }
  free(m);
}

void free_vector(int *v) { free(v); }

uint8_t **get_matrix_uint8(size_t r, size_t c) {
  uint8_t **m;
  m = (uint8_t **)malloc(r * sizeof(uint8_t *));
  for (size_t i = 0; i < r; i++) {
    m[i] = malloc(c * sizeof(uint8_t));
  }
  return m;
}

uint8_t **get_matrix_uint8_calloc(size_t r, size_t c) {
  uint8_t **m;
  m = (uint8_t **)malloc(r * sizeof(uint8_t *));
  for (size_t i = 0; i < r; i++) {
    m[i] = calloc(c, sizeof(uint8_t));
  }
  return m;
}

void free_matrix_uint8(uint8_t **m, size_t r) {
  for (size_t i = 0; i < r; i++) {
    free(m[i]);
  }
  free(m);
}

uint8_t *get_vector_uint8(size_t r) {
  return (uint8_t *)malloc(r * sizeof(uint8_t));
}

uint8_t *get_vector_uint8_calloc(size_t r) {
  return (uint8_t *)calloc(r, sizeof(uint8_t));
}

void free_vector_uint8(uint8_t *v) { free(v); }

size_t *get_vector_size_t(size_t r) {
  return (size_t *)malloc(r * sizeof(size_t));
}

void free_vector_size_t(size_t *v) { free(v); }
