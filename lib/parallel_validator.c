#include "parallel_validator.h"
#include "combinatorial.h"
#include <omp.h>
#include <stdlib.h>

void pv_validate(covering_array_t *ca) {
    if (ca == NULL || ca->matrix == NULL)
        return;

    size_t R = (size_t)binomial(ca->k, ca->t);
    size_t C = 1;
    for (int i = 0; i < ca->t; i++)
        C *= (size_t)ca->v;

    int **IToC = get_matrix((int)R, ca->t);
    t_wise(IToC, ca->k, ca->t);

    if (ca->P == NULL) {
        ca->P = get_matrix_uint8(R, C);
        if (ca->P == NULL) {
            free_matrix(IToC, (int)R);
            return;
        }
    }

    for (size_t i = 0; i < R; i++) {
        for (size_t j = 0; j < C; j++) {
            ca->P[i][j] = 0;
        }
    }

    int num_threads = omp_get_max_threads();

#pragma omp parallel num_threads(num_threads)
    {
        size_t tid = (size_t)omp_get_thread_num();
        size_t chunk_size = (R + omp_get_num_threads() - 1) / omp_get_num_threads();
        size_t start = tid * chunk_size;
        size_t end = start + chunk_size;
        if (end > R)
            end = R;

        for (size_t j = start; j < end; j++) {
            for (int i = 0; i < ca->N; i++) {
                int idx = get_col(ca->matrix[i], IToC, (int)j, ca->t, ca->v);
                if (idx >= 0) {
                    ca->P[j][idx]++;
                }
            }
        }
    }

    size_t covered = 0;
    for (size_t i = 0; i < R; i++) {
        for (size_t j = 0; j < C; j++) {
            if (ca->P[i][j] > 0) {
                covered++;
            }
        }
    }

    ca->covered = covered;
    ca->total = R * C;

    free_matrix(IToC, (int)R);
}