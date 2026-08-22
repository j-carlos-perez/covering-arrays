#ifndef COMBINATORIAL_H
#define COMBINATORIAL_H

#include <stdint.h>

/* Returned by binomial() when C(k,r) does not fit in a uint64_t. */
#define BINOMIAL_OVERFLOW UINT64_MAX

/*
 * Uniform random integer in [0, bound).
 *
 * `rand() % bound` is biased whenever bound does not divide RAND_MAX+1: the
 * low residues occur once more often than the high ones. That skew works
 * directly against the balanced initialisers, whose whole purpose is to
 * control the symbol distribution. Returns 0 for bound <= 0.
 */
int rand_below(int bound);

void shuffle(int *array, int n);

/*
 * C(k, r), computed without overflowing an intermediate.
 * Returns 0 when r < 0, k < 0, or k < r; BINOMIAL_OVERFLOW if the result
 * does not fit in a uint64_t. Callers that use the result as an allocation
 * size must check for both.
 */
uint64_t binomial(int k, int r);

/* Non-zero if n is a usable count (neither the overflow marker nor larger
 * than what the platform can index). */
int binomial_is_usable(uint64_t n);

int t_wise(int **GTP, int k, int t);
int inv_ruffini(int *V, int num, int v, int t);
int get_col(const int *line, int **IToC, int j, int t, int v);

int **generate_t_combinations(int k, int t, int *out_n);

typedef void (*t_combination_callback)(int *combination, int index, int k,
                                       int t, void *user_data);
int t_wise_visit(int k, int t, t_combination_callback cb, void *user_data);

void init_permutation(int *arr, int n);
int next_permutation(int *arr, int n);

void init_gray_code(int *arr, int n);
int next_gray_code(int *arr, int n, int v);

#endif
