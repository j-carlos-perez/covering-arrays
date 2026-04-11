#include <stddef.h>
#include "local_calculation.h"
#include "combinatorial.h"

ssize_t ca_compute_cell_delta(covering_array_t *ca, const ca_affected_t *pre,
                               int **IToC, int row_idx, int col_idx, int new_val)
{
    if (ca == NULL || pre == NULL || IToC == NULL) {
        return 0;
    }

    int old_val = ca->matrix[row_idx][col_idx];

    if (new_val == old_val) {
        return 0;
    }

    const uint16_t *affected = precompute_get_col_affected(pre, (size_t)col_idx);
    size_t affected_count = precompute_get_col_affected_count(pre);
    int t = (int)pre->t;
    int v = ca->v;

    ssize_t delta = 0;

    for (size_t i = 0; i < affected_count; i++) {
        uint16_t itoc_idx = affected[i];

        int saved_val = ca->matrix[row_idx][col_idx];

        ca->matrix[row_idx][col_idx] = old_val;
        int old_encoding = get_col(ca->matrix[row_idx], IToC, (int)itoc_idx, t, v);

        ca->matrix[row_idx][col_idx] = new_val;
        int new_encoding = get_col(ca->matrix[row_idx], IToC, (int)itoc_idx, t, v);

        ca->matrix[row_idx][col_idx] = saved_val;

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

    return delta;
}

ssize_t ca_apply_cell_change(covering_array_t *ca, const ca_affected_t *pre,
                              int **IToC, int row_idx, int col_idx, int new_val)
{
    if (ca == NULL || pre == NULL || IToC == NULL) {
        return 0;
    }

    ssize_t delta = ca_compute_cell_delta(ca, pre, IToC, row_idx, col_idx, new_val);

    if (delta == 0) {
        return 0;
    }

    int old_val = ca->matrix[row_idx][col_idx];
    int t = (int)pre->t;
    int v = ca->v;

    const uint16_t *affected = precompute_get_col_affected(pre, (size_t)col_idx);
    size_t affected_count = precompute_get_col_affected_count(pre);

    for (size_t i = 0; i < affected_count; i++) {
        uint16_t itoc_idx = affected[i];

        int saved_val = ca->matrix[row_idx][col_idx];

        ca->matrix[row_idx][col_idx] = old_val;
        int old_encoding = get_col(ca->matrix[row_idx], IToC, (int)itoc_idx, t, v);

        ca->matrix[row_idx][col_idx] = new_val;
        int new_encoding = get_col(ca->matrix[row_idx], IToC, (int)itoc_idx, t, v);

        if (old_encoding != -1) {
            ca->P[itoc_idx][old_encoding]--;
        }

        if (new_encoding != -1) {
            ca->P[itoc_idx][new_encoding]++;
        }

        ca->matrix[row_idx][col_idx] = saved_val;
    }

    ca->matrix[row_idx][col_idx] = new_val;
    ca->covered += delta;

    return delta;
}