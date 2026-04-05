#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "covering_array.h"

covering_array_t *ca_create(int N, int k, int v, int t)
{
    covering_array_t *ca = malloc(sizeof(covering_array_t));
    if (ca == NULL) {
        return NULL;
    }

    ca->N = N;
    ca->k = k;
    ca->v = v;
    ca->t = t;
    ca->matrix = get_matrix(N, k);

    if (ca->matrix == NULL) {
        free(ca);
        return NULL;
    }

    return ca;
}

void ca_destroy(covering_array_t *ca)
{
    if (ca == NULL) {
        return;
    }

    if (ca->matrix != NULL) {
        free_matrix(ca->matrix, ca->N);
    }

    free(ca);
}

int ca_validate(covering_array_t *ca)
{
    if (ca == NULL || ca->matrix == NULL) {
        return 0;
    }

    size_t R = binomial(ca->k, ca->t);
    size_t C = 1;
    for (int i = 0; i < ca->t; i++) {
        C *= (size_t)ca->v;
    }

    int **IToC = get_matrix((int)R, ca->t);
    t_wise(IToC, ca->k, ca->t);

    int **P = get_matrix((int)R, (int)C);
    for (size_t i = 0; i < R; i++) {
        for (size_t j = 0; j < C; j++) {
            P[i][j] = 0;
        }
    }

    for (int i = 0; i < ca->N; i++) {
        for (size_t j = 0; j < R; j++) {
            int c = get_col(ca->matrix[i], IToC, (int)j, ca->t, ca->v);
            if (c != -1) {
                P[j][c]++;
            }
        }
    }

    size_t covered = 0;
    for (size_t i = 0; i < R; i++) {
        for (size_t j = 0; j < C; j++) {
            if (P[i][j] > 0) {
                covered++;
            }
        }
    }

    size_t total = R * C;
    int valid = (covered == total);

    free_matrix(IToC, (int)R);
    free_matrix(P, (int)R);

    return valid;
}