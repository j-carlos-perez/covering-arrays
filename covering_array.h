#ifndef COVERING_ARRAY_H
#define COVERING_ARRAY_H

#include "utl/combinatorial.h"
#include "utl/memory.h"

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

#endif