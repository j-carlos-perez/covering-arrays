#include "local_calculation.h"
#include "combinatorial.h"
#include <stddef.h>

/*
 * Computes the coverage delta when changing a single cell value.
 *
 * The delta is the change in covered combinations. Does NOT modify the array
 * - purely computational.
 *
 * For each affected t-combination, compares old vs new encoding.
 * A combo becomes covered if P[itoc_idx][new_encoding] was 0 before.
 * A combo becomes uncovered if P[itoc_idx][old_encoding] was 1 before.
 *
 * Returns the delta (change in ca->covered).
 */
ssize_t ca_compute_cell_delta(covering_array_t *ca, const ca_affected_t *pre,
                              int **IToC, int row_idx, int col_idx,
                              int new_val) {
  if (ca == NULL || pre == NULL || IToC == NULL || ca->P == NULL) {
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
  int *row = ca->matrix[row_idx];

  ssize_t delta = 0;

  for (size_t i = 0; i < affected_count; i++) {
    uint16_t itoc_idx = affected[i];

    row[col_idx] = old_val;
    int old_encoding = get_col(row, IToC, (int)itoc_idx, t, v);

    row[col_idx] = new_val;
    int new_encoding = get_col(row, IToC, (int)itoc_idx, t, v);

    if (old_encoding == new_encoding) {
      continue;
    }

    if (old_encoding != -1 && ca->P[itoc_idx][old_encoding] == 1) {
      delta--;
    }

    if (new_encoding != -1 && ca->P[itoc_idx][new_encoding] == 0) {
      delta++;
    }
  }

  row[col_idx] = old_val;

  return delta;
}

/*
 * Applies a cell change and updates the coverage matrix P in place.
 *
 * Always writes the new value and always brings P, ca->covered and
 * ca->tcomb_counter into agreement with it. In particular a net-zero delta is
 * NOT a no-op: a move can cover one combination while uncovering another, or
 * shift counts between tuples that stay covered either way. The previous
 * version returned early whenever the delta was zero, silently discarding the
 * change and leaving P describing a matrix that no longer existed -- which
 * made every plateau move in a local search a lie.
 *
 * Returns the delta (change in ca->covered); 0 means "no net coverage change",
 * not "nothing happened".
 */
ssize_t ca_apply_cell_change(covering_array_t *ca, const ca_affected_t *pre,
                             int **IToC, int row_idx, int col_idx,
                             int new_val) {
  if (ca == NULL || pre == NULL || IToC == NULL || ca->P == NULL ||
      ca->tcomb_counter == NULL) {
    return 0;
  }

  int old_val = ca->matrix[row_idx][col_idx];
  if (new_val == old_val) {
    return 0; /* the one genuine no-op */
  }

  int t = (int)pre->t;
  int v = ca->v;
  int *row = ca->matrix[row_idx];

  const uint16_t *affected = precompute_get_col_affected(pre, (size_t)col_idx);
  size_t affected_count = precompute_get_col_affected_count(pre);

  ssize_t delta = 0;

  /* Single pass: derive the delta and update P together, so the two can never
     disagree about what was applied. */
  for (size_t i = 0; i < affected_count; i++) {
    uint16_t itoc_idx = affected[i];

    row[col_idx] = old_val;
    int old_encoding = get_col(row, IToC, (int)itoc_idx, t, v);

    row[col_idx] = new_val;
    int new_encoding = get_col(row, IToC, (int)itoc_idx, t, v);

    if (old_encoding == new_encoding) {
      continue;
    }

    if (old_encoding != -1) {
      ca->P[itoc_idx][old_encoding]--;
      if (ca->P[itoc_idx][old_encoding] == 0) {
        delta--;
        ca->tcomb_counter[itoc_idx]++;
      }
    }

    if (new_encoding != -1) {
      if (ca->P[itoc_idx][new_encoding] == 0) {
        delta++;
        ca->tcomb_counter[itoc_idx]--;
      }
      ca->P[itoc_idx][new_encoding]++;
    }
  }

  ca->matrix[row_idx][col_idx] = new_val;
  ca->covered = (size_t)((ssize_t)ca->covered + delta);

  return delta;
}
