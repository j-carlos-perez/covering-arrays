#include <stddef.h>
#include <stdlib.h>
#include "t_columns_delta.h"
#include "combinatorial.h"

ssize_t ca_compute_tcolumns_delta(covering_array_t *ca, const ca_affected_t *pre,
                                   int **IToC, int row_idx, 
                                   uint16_t change_set_idx, const int *new_vals)
{
    if (ca == NULL || pre == NULL || IToC == NULL || new_vals == NULL) {
        return 0;
    }

    if (change_set_idx >= pre->change_sets) {
        return 0;
    }

    const uint16_t *affected = precompute_get_affected(pre, change_set_idx);
    size_t affected_count = precompute_get_affected_count(pre);
    int t = (int)pre->t;
    int v = ca->v;

    /* Get the t columns that are changing */
    int *changing_cols = IToC[change_set_idx];

    /* Save current values of the changing columns */
    int *saved_vals = malloc(t * sizeof(int));
    if (saved_vals == NULL) {
        return 0;
    }
    for (int j = 0; j < t; j++) {
        saved_vals[j] = ca->matrix[row_idx][changing_cols[j]];
    }

    /* Check if any value actually changes */
    int any_change = 0;
    for (int j = 0; j < t; j++) {
        if (new_vals[j] != saved_vals[j]) {
            any_change = 1;
            break;
        }
    }

    if (!any_change) {
        free(saved_vals);
        return 0;
    }

    ssize_t delta = 0;

    for (size_t i = 0; i < affected_count; i++) {
        uint16_t itoc_idx = affected[i];

        /* Compute old encoding (restore old values) */
        for (int j = 0; j < t; j++) {
            ca->matrix[row_idx][changing_cols[j]] = saved_vals[j];
        }
        int old_encoding = get_col(ca->matrix[row_idx], IToC, (int)itoc_idx, t, v);

        /* Compute new encoding (apply new values) */
        for (int j = 0; j < t; j++) {
            ca->matrix[row_idx][changing_cols[j]] = new_vals[j];
        }
        int new_encoding = get_col(ca->matrix[row_idx], IToC, (int)itoc_idx, t, v);

        if (old_encoding == new_encoding) {
            continue;
        }

        if (old_encoding != -1) {
            if (ca->P[itoc_idx][old_encoding] == 1) {
                delta--;
            }
        }

        if (new_encoding != -1) {
            if (ca->P[itoc_idx][new_encoding] == 0) {
                delta++;
            }
        }
    }

    /* Restore original values */
    for (int j = 0; j < t; j++) {
        ca->matrix[row_idx][changing_cols[j]] = saved_vals[j];
    }

    free(saved_vals);

    return delta;
}

ssize_t ca_apply_tcolumns_change(covering_array_t *ca, const ca_affected_t *pre,
                                  int **IToC, int row_idx,
                                  uint16_t change_set_idx, const int *new_vals)
{
    if (ca == NULL || pre == NULL || IToC == NULL || new_vals == NULL) {
        return 0;
    }

    if (change_set_idx >= pre->change_sets) {
        return 0;
    }

    ssize_t delta = ca_compute_tcolumns_delta(ca, pre, IToC, row_idx, change_set_idx, new_vals);

    if (delta == 0) {
        return 0;
    }

    const uint16_t *affected = precompute_get_affected(pre, change_set_idx);
    size_t affected_count = precompute_get_affected_count(pre);
    int t = (int)pre->t;
    int v = ca->v;

    /* Get the t columns that are changing */
    int *changing_cols = IToC[change_set_idx];

    /* Save current values */
    int *saved_vals = malloc(t * sizeof(int));
    if (saved_vals == NULL) {
        return 0;
    }
    for (int j = 0; j < t; j++) {
        saved_vals[j] = ca->matrix[row_idx][changing_cols[j]];
    }

    /* Update P matrix */
    for (size_t i = 0; i < affected_count; i++) {
        uint16_t itoc_idx = affected[i];

        /* Compute old encoding */
        for (int j = 0; j < t; j++) {
            ca->matrix[row_idx][changing_cols[j]] = saved_vals[j];
        }
        int old_encoding = get_col(ca->matrix[row_idx], IToC, (int)itoc_idx, t, v);

        /* Compute new encoding */
        for (int j = 0; j < t; j++) {
            ca->matrix[row_idx][changing_cols[j]] = new_vals[j];
        }
        int new_encoding = get_col(ca->matrix[row_idx], IToC, (int)itoc_idx, t, v);

        if (old_encoding != -1) {
            ca->P[itoc_idx][old_encoding]--;
        }

        if (new_encoding != -1) {
            ca->P[itoc_idx][new_encoding]++;
        }
    }

    /* Apply the changes to the matrix */
    for (int j = 0; j < t; j++) {
        ca->matrix[row_idx][changing_cols[j]] = new_vals[j];
    }

    free(saved_vals);

    ca->covered += delta;

    return delta;
}