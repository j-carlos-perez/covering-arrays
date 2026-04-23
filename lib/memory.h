#ifndef MEMORY_H
#define MEMORY_H
#include <stddef.h>
#include <stdint.h>

int **get_matrix(size_t r, size_t c);
int *get_vector(size_t r);
void free_matrix(int **m, size_t r);
void free_vector(int *v);

uint8_t **get_matrix_uint8(size_t r, size_t c);
uint8_t **get_matrix_uint8_calloc(size_t r, size_t c);
void free_matrix_uint8(uint8_t **m, size_t r);

uint8_t *get_vector_uint8(size_t r);
uint8_t *get_vector_uint8_calloc(size_t r);
void free_vector_uint8(uint8_t *v);

size_t *get_vector_size_t(size_t r);
void free_vector_size_t(size_t *v);

#endif
