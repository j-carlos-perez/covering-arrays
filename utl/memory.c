#include <stdlib.h>
#include "memory.h"

int **get_matrix(int r, int c)
{
    int **m;
    m = (int **)malloc(r * sizeof(int *));
    for (int i = 0; i < r; i++) {
        m[i] = malloc(c * sizeof(int));
    }
    return m;
}

int *get_vector(int r)
{
    int *v;
    v = (int *)malloc(r * sizeof(int));
    return v;
}

void free_matrix(int **m, int r)
{
    for (int i = 0; i < r; i++) {
        free(m[i]);
    }
    free(m);
}

void free_vector(int *v)
{
    free(v);
}
