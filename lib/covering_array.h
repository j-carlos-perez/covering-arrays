#ifndef COVERING_ARRAY_H
#define COVERING_ARRAY_H

#include "combinatorial.h"
#include "memory.h"

typedef struct covering_array {
    int N;
    int k;
    int v;
    int t;
    int **matrix;
} covering_array_t;

covering_array_t *ca_create(int N, int k, int v, int t);
void ca_destroy(covering_array_t *ca);
int ca_validate(covering_array_t *ca);
covering_array_t *ca_load(const char *filename);
int ca_save(const char *folder_path, covering_array_t *ca, const char *comment, int missing);
void ca_print(covering_array_t *ca);

#endif