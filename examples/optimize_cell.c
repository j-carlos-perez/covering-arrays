#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include "lib/covering_array.h"
#include "lib/precompute.h"
#include "lib/combinatorial.h"
#include "lib/local_calculation.h"

/*
 * Example demonstrating:
 * 1. Creating a covering array with random values
 * 2. Finding the best value for a random cell (value that maximizes delta)
 * 3. Applying the change and verifying
 */

static int find_best_value_for_cell(covering_array_t *ca, const ca_affected_t *pre,
                                     int **IToC, int row_idx, int col_idx,
                                     int *out_best_value, ssize_t *out_delta)
{
    if (ca == NULL || pre == NULL || IToC == NULL || out_best_value == NULL || out_delta == NULL) {
        return -1;
    }

    int current_val = ca->matrix[row_idx][col_idx];
    ssize_t best_delta = 0;
    int best_value = current_val;

    printf("  Current value: %d\n", current_val);

    for (int candidate_val = 0; candidate_val < ca->v; candidate_val++) {
        if (candidate_val == current_val) {
            continue;
        }

        ssize_t delta = ca_compute_cell_delta(ca, pre, IToC, row_idx, col_idx, candidate_val);
        printf("  Candidate value %d: delta = %zd\n", candidate_val, delta);

        if (delta > best_delta) {
            best_delta = delta;
            best_value = candidate_val;
        }
    }

    *out_best_value = best_value;
    *out_delta = best_delta;

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 6) {
        fprintf(stderr, "Usage: %s N t k v <output_folder>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int t = atoi(argv[2]);
    int k = atoi(argv[3]);
    int v = atoi(argv[4]);
    const char *output_folder = argv[5];

    if (N <= 0 || t <= 0 || k <= 0 || v <= 0) {
        fprintf(stderr, "Error: all parameters must be positive\n");
        return 1;
    }

    srand((unsigned int)time(NULL));

    covering_array_t *ca = ca_create(N, k, v, t);
    if (ca == NULL) {
        fprintf(stderr, "Error: failed to create covering array\n");
        return 1;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < k; j++) {
            ca->matrix[i][j] = rand() % v;
        }
    }

    printf("Initial matrix (%dx%d):\n", N, k);
    for (int i = 0; i < N && i < 5; i++) {
        for (int j = 0; j < k; j++) {
            printf("%d ", ca->matrix[i][j]);
        }
        printf("\n");
    }
    if (N > 5) printf("  ... (%d more rows)\n", N - 5);

    printf("\nValidating initial matrix...\n");
    ca_validate(ca);
    printf("Coverage: %zu / %zu (%.1f%%)\n", ca->covered, ca->total,
           100.0 * (double)ca->covered / (double)ca->total);

    printf("\nPrecomputing affected t-combinations...\n");
    ca_affected_t *pre = precompute_create((size_t)k, (size_t)t);
    if (pre == NULL) {
        fprintf(stderr, "Error: failed to create precompute\n");
        ca_destroy(ca);
        return 1;
    }
    printf("Affected per column: %zu\n", precompute_get_col_affected_count(pre));

    size_t R = pre->change_sets;
    int **IToC = get_matrix((int)R, t);
    t_wise(IToC, k, t);

    printf("\n=== Selecting random cell ===\n");
    int row_idx = rand() % N;
    int col_idx = rand() % k;
    printf("Picked cell (%d, %d)\n", row_idx, col_idx);

    printf("\n=== Finding best value for cell (%d, %d) ===\n", row_idx, col_idx);
    int best_value;
    ssize_t delta;
    if (find_best_value_for_cell(ca, pre, IToC, row_idx, col_idx, &best_value, &delta) != 0) {
        fprintf(stderr, "Error: failed to find best value\n");
        precompute_destroy(pre);
        ca_destroy(ca);
        free_matrix(IToC, (int)R);
        return 1;
    }

    printf("\nBest value: %zd (delta: %zd)\n", (ssize_t)best_value, delta);

    if (delta > 0) {
        printf("\n=== Applying change ===\n");
        ca_apply_cell_change(ca, pre, IToC, row_idx, col_idx, best_value);
        printf("Coverage after change: %zu / %zu (%.1f%%)\n", ca->covered, ca->total,
               100.0 * (double)ca->covered / (double)ca->total);
    } else {
        printf("\nNo improvement found, skipping change.\n");
    }

    printf("\n=== Sanity Check ===\n");
    covering_array_t *ca_verify = ca_create(ca->N, ca->k, ca->v, ca->t);
    if (ca_verify == NULL) {
        fprintf(stderr, "Error: failed to create verification CA\n");
        precompute_destroy(pre);
        ca_destroy(ca);
        free_matrix(IToC, (int)R);
        return 1;
    }

    for (int i = 0; i < ca->N; i++) {
        for (int j = 0; j < ca->k; j++) {
            ca_verify->matrix[i][j] = ca->matrix[i][j];
        }
    }
    ca_validate(ca_verify);

    printf("Incremental covered: %zu\n", ca->covered);
    printf("Full recompute covered: %zu\n", ca_verify->covered);
    printf("Covered difference: %zd\n", (ssize_t)ca->covered - (ssize_t)ca_verify->covered);

    size_t C = 1;
    for (int i = 0; i < t; i++) C *= (size_t)v;

    printf("P matrix match: ");
    int p_match = 1;
    for (size_t i = 0; i < R && p_match; i++) {
        for (size_t j = 0; j < C && p_match; j++) {
            if (ca->P[i][j] != ca_verify->P[i][j]) {
                p_match = 0;
            }
        }
    }
    printf("%s\n", p_match ? "YES" : "NO");

    printf("\n=== Saving result ===\n");
    int missing = (int)(ca->total - ca->covered);
    if (ca_save(output_folder, ca, "Optimized covering array", missing) == 0) {
        printf("Saved to: %s (missing: %d)\n", output_folder, missing);
    } else {
        fprintf(stderr, "Warning: failed to save CA\n");
    }

    ca_destroy(ca_verify);
    free_matrix(IToC, (int)R);
    precompute_destroy(pre);
    ca_destroy(ca);

    printf("\nDone.\n");
    return 0;
}