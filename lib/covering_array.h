#ifndef COVERING_ARRAY_H
#define COVERING_ARRAY_H

#include "combinatorial.h"
#include "memory.h"

typedef struct covering_array {
    int N;
    int capacity;
    int k;
    int v;
    int t;
    int **matrix;
    uint8_t **P;
    size_t covered;
    size_t total;
} covering_array_t;

covering_array_t *ca_create(int N, int k, int v, int t);
void ca_destroy(covering_array_t *ca);
int ca_validate(covering_array_t *ca);
covering_array_t *ca_load(const char *filename);
int ca_save(const char *folder_path, covering_array_t *ca, const char *comment, int missing);
void ca_print(covering_array_t *ca);
int ca_add_row(covering_array_t *ca, const int *row);
int ca_init_random(covering_array_t *ca);
int ca_init_rotation_position(covering_array_t *ca);
int ca_init_rotation_full(covering_array_t *ca);

#endif