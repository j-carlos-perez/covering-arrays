#include "parallel_validator.h"
#include "combinatorial.h"
#include "memory.h"
#include <omp.h>
#include <stdlib.h>

/*
 * Validates a covering array and computes its coverage matrix P in parallel using OpenMP.
 * 
 * For each t-combination of columns, determines which v^t symbol combinations are covered.
 * Updates ca->P (coverage counts), ca->tcomb_counter (uncovered per row), and ca->covered.
 * 
 * R = C(k, t) = number of t-combinations of columns
 * C = v^t = number of symbol combinations per t-set
 * 
 * The P matrix: P[j][c] = how many rows cover the j-th column t-set with combo c.
 * The tcomb_counter: tcomb_counter[j] = remaining uncovered combos in row j (initially C).
 * 
 * Requires ca->P and ca->tcomb_counter to be NULL (will allocate if needed).
 * Does NOT reset existing values; accumulates on top of previous coverage.
 */
void pv_validate(covering_array_t *ca) {
  if (ca == NULL || ca->matrix == NULL)
    return;

  size_t R = (size_t)binomial(ca->k, ca->t);
  size_t C = 1;
  for (int i = 0; i < ca->t; i++)
    C *= (size_t)ca->v;

  int num_threads = omp_get_max_threads();

  int **IToC = get_matrix((int)R, ca->t);
  t_wise(IToC, ca->k, ca->t);

  if (ca->P == NULL) {
    ca->P = get_matrix_uint8_calloc(R, C);
    if (ca->P == NULL) {
      free_matrix(IToC, (int)R);
      return;
    }
  }

  if (ca->tcomb_counter == NULL) {
    ca->tcomb_counter = get_vector_size_t(R);
    if (ca->tcomb_counter == NULL) {
      free_matrix(IToC, (int)R);
      free_matrix_uint8(ca->P, R);
      return;
    }
  }
  for (size_t i = 0; i < R; i++) {
    ca->tcomb_counter[i] = C;
  }
#pragma omp parallel num_threads(num_threads)
  {
    size_t chunk_size = (R + omp_get_num_threads() - 1) / omp_get_num_threads();
    size_t start = (size_t)omp_get_thread_num() * chunk_size;
    size_t end = start + chunk_size;
    if (end > R)
      end = R;

    for (size_t j = start; j < end; j++) {
      for (int i = 0; i < ca->N; i++) {
        int idx = get_col(ca->matrix[i], IToC, (int)j, ca->t, ca->v);
        if (idx >= 0) {
          ca->P[j][idx]++;
          if (ca->P[j][idx] == 1) {
            ca->tcomb_counter[j]--;
          }
        }
      }
    }
  }

  size_t covered = 0;
#pragma omp parallel for reduction(+ : covered)
  for (size_t i = 0; i < R; i++) {
    for (size_t j = 0; j < C; j++) {
      covered += (ca->P[i][j] > 0) ? 1 : 0;
    }
  }

  ca->covered = covered;
  ca->total = R * C;

  free_matrix(IToC, (int)R);
}
