#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "lib/covering_array.h"
#include "lib/precompute.h"
#include "lib/combinatorial.h"
#include "lib/local_calculation.h"

/*
 * Example demonstrating:
 * 1. Creating a covering array with random values
 * 2. Validating it and computing P matrix
 * 3. Precomputing affected t-combinations for column changes
 * 4. Updating a single cell and incrementally updating P matrix
 */

int main(int argc, char *argv[])
{
    /* ========================================================================
     * STEP 1: Parse command line arguments (N, t, k, v)
     * ======================================================================== */
    if (argc != 5) {
        fprintf(stderr, "Usage: %s N t k v\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);  /* Number of rows */
    int t = atoi(argv[2]);  /* Strength of t-way combinations */
    int k = atoi(argv[3]);  /* Number of columns */
    int v = atoi(argv[4]);  /* Alphabet size (values per column) */

    if (N <= 0 || t <= 0 || k <= 0 || v <= 0) {
        fprintf(stderr, "Error: all parameters must be positive\n");
        return 1;
    }

    /* Initialize random number generator */
    srand((unsigned int)time(NULL));

    /* ========================================================================
     * STEP 2: Create covering array and populate with random values
     * ======================================================================== */
    covering_array_t *ca = ca_create(N, k, v, t);
    if (ca == NULL) {
        fprintf(stderr, "Error: failed to create covering array\n");
        return 1;
    }

    /* Fill matrix with random values from 0 to v-1 */
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < k; j++) {
            ca->matrix[i][j] = rand() % v;
        }
    }

    printf("Initial matrix:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < k; j++) {
            printf("%d ", ca->matrix[i][j]);
        }
        printf("\n");
    }

    /* ========================================================================
     * STEP 3: Validate the covering array
     *
     * This computes:
     * - P matrix: R x C uint8_t matrix where P[r][c] = count of rows covering
     *             the r-th t-combination encoded as column c
     * - covered: number of (r,c) pairs with count > 0
     * - total: R * C (total number of t-combination encodings)
     * ======================================================================== */
    printf("\nValidating initial matrix...\n");
    int valid = ca_validate(ca);
    printf("Validation result: %s\n", valid ? "VALID" : "INVALID");
    printf("Covered: %zu / %zu\n", ca->covered, ca->total);

    /* ========================================================================
     * STEP 4: Precompute affected t-combinations for each column
     *
     * When a single cell in column c changes, only the t-combinations that
     * include column c are affected. This precomputes the IToC indices of
     * those combinations for each column, enabling efficient incremental
     * P matrix updates.
     *
     * - pre->col_offsets[c] gives the start position for column c
     * - pre->col_indices contains the IToC row indices for each column
     * - Number affected per column = C(k-1, t-1)
     * ======================================================================== */
    printf("\nPrecomputing affected t-combinations for each column...\n");
    ca_affected_t *pre = precompute_create((size_t)k, (size_t)t);
    if (pre == NULL) {
        fprintf(stderr, "Error: failed to create precompute\n");
        ca_destroy(ca);
        return 1;
    }
    printf("Precomputation complete. Affected per column: %zu\n",
           precompute_get_col_affected_count(pre));

    /* ========================================================================
     * STEP 5: Pick a random cell to modify
     * ======================================================================== */
    int row_idx = rand() % N;
    int col_idx = rand() % k;
    int old_val = ca->matrix[row_idx][col_idx];

    /* Ensure new value is different from old */
    int new_val;
    do {
        new_val = rand() % v;
    } while (new_val == old_val);

    printf("\n=== Updating cell (%d, %d) from %d to %d ===\n",
           row_idx, col_idx, old_val, new_val);

    /* Do NOT modify the matrix here - functions will handle it */
    /* The functions read old_val from current matrix state */

    /* ========================================================================
     * STEP 6: Get affected t-combinations for this column
     *
     * These are the IToC row indices whose t-combination includes column col_idx.
     * Only these combinations need their P entries updated.
     * ======================================================================== */
    const uint16_t *affected = precompute_get_col_affected(pre, (size_t)col_idx);
    size_t affected_count = precompute_get_col_affected_count(pre);

    printf("Affected IToC indices (%zu total): ", affected_count);
    for (size_t i = 0; i < affected_count && i < 10; i++) {
        printf("%u ", affected[i]);
    }
    if (affected_count > 10) printf("...");
    printf("\n");

    size_t old_covered = ca->covered;

    /* ========================================================================
     * STEP 7: Build IToC table for encoding t-tuples to column indices
     *
     * IToC[r][j] gives the column index of the j-th element in the
     * r-th t-combination. This is used by get_col() to encode a row's
     * values for a given t-combination into a single column index.
     * ======================================================================== */
    int **IToC = get_matrix((int)pre->change_sets, t);
    t_wise(IToC, k, t);

    /* ========================================================================
     * STEP 8: Incrementally update P matrix using local_calculation functions
     *
     * 1. First compute delta using ca_compute_cell_delta (read-only)
     * 2. Show encoding changes for display
     * 3. Apply the change using ca_apply_cell_change
     * ======================================================================== */

    /* Compute delta (read-only - matrix and P unchanged) */
    ssize_t delta = ca_compute_cell_delta(ca, pre, IToC, row_idx, col_idx, new_val);

    /* Show encoding changes for each affected t-combination */
    for (size_t i = 0; i < affected_count; i++) {
        uint16_t itoc_idx = affected[i];

        /* Compute old_encoding (with old value) */
        int saved_val = ca->matrix[row_idx][col_idx];
        ca->matrix[row_idx][col_idx] = old_val;
        int old_encoding = get_col(ca->matrix[row_idx], IToC, (int)itoc_idx, t, v);

        /* Compute new_encoding (with new value) */
        ca->matrix[row_idx][col_idx] = new_val;
        int new_encoding = get_col(ca->matrix[row_idx], IToC, (int)itoc_idx, t, v);
        ca->matrix[row_idx][col_idx] = saved_val;

        printf("  IToC[%u]: encoding %d -> %d\n", itoc_idx, old_encoding, new_encoding);
    }

    /* Apply the change (modifies both matrix and P) */
    ca_apply_cell_change(ca, pre, IToC, row_idx, col_idx, new_val);

    free_matrix(IToC, (int)pre->change_sets);

    printf("\nCoverage change: %zu -> %zu (delta: %zd)\n",
           old_covered, ca->covered, delta);

    /* ========================================================================
     * STEP 9: Sanity check - recompute P from scratch using a fresh CA
     *
     * Create a new CA, copy the current (updated) matrix, and validate to
     * recompute P matrix from scratch. Compare results to verify incremental
     * update is correct.
     * ======================================================================== */
    covering_array_t *ca_verify = ca_create(ca->N, ca->k, ca->v, ca->t);
    if (ca_verify == NULL) {
        fprintf(stderr, "Error: failed to create verification CA\n");
        precompute_destroy(pre);
        ca_destroy(ca);
        return 1;
    }

    for (int i = 0; i < ca->N; i++) {
        for (int j = 0; j < ca->k; j++) {
            ca_verify->matrix[i][j] = ca->matrix[i][j];
        }
    }
    ca_validate(ca_verify);

    size_t R = binomial(k, t);
    size_t C = 1;
    for (int i = 0; i < t; i++) {
        C *= (size_t)v;
    }

    printf("\n=== Sanity Check ===\n");
    printf("Incremental covered: %zu\n", ca->covered);
    printf("Full recompute covered: %zu\n", ca_verify->covered);
    printf("Covered difference: %zd\n", (ssize_t)ca->covered - (ssize_t)ca_verify->covered);

    printf("P matrix match: ");
    int p_match = 1;
    for (size_t i = 0; i < R; i++) {
        for (size_t j = 0; j < C; j++) {
            if (ca->P[i][j] != ca_verify->P[i][j]) {
                if (p_match) printf("NO\n");
                printf("  P[%zu][%zu]: incremental=%d, full=%d (diff=%d)\n",
                       i, j, ca->P[i][j], ca_verify->P[i][j],
                       (int)ca->P[i][j] - (int)ca_verify->P[i][j]);
                p_match = 0;
            }
        }
    }
    if (p_match) printf("YES\n");

    ca_destroy(ca_verify);

    printf("\n=== Final P matrix sample (first 3 rows, first 5 cols) ===\n");
    for (size_t i = 0; i < R && i < 3; i++) {
        printf("Row %zu: ", i);
        for (size_t j = 0; j < C && j < 5; j++) {
            printf("%d ", ca->P[i][j]);
        }
        printf("\n");
    }

    /* ========================================================================
     * Cleanup
     * ======================================================================== */
    precompute_destroy(pre);
    ca_destroy(ca);

    printf("\nDone.\n");
    return 0;
}