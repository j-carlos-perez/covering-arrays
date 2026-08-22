#include "t_columns_delta.h"
#include "combinatorial.h"
#include <stddef.h>
#include <stdlib.h>

/* t is the covering-array strength: small (2..6 in practice), so the saved
   values fit comfortably on the stack and need no allocation per call. */
#define TCOL_MAX_T 16

static int tcolumns_args_valid(const covering_array_t *ca,
                               const ca_affected_t *pre, int **IToC,
                               int row_idx, uint16_t change_set_idx,
                               const int *new_vals, int needs_counter) {
  if (ca == NULL || pre == NULL || IToC == NULL || new_vals == NULL ||
      ca->matrix == NULL || ca->P == NULL || ca->total == 0 ||
      (needs_counter && ca->tcomb_counter == NULL) || row_idx < 0 ||
      row_idx >= ca->N || pre->k != (size_t)ca->k ||
      pre->t != (size_t)ca->t || change_set_idx >= pre->change_sets ||
      pre->t < 1 || pre->t > TCOL_MAX_T || IToC[change_set_idx] == NULL) {
    return 0;
  }
  for (size_t j = 0; j < pre->t; j++) {
    if (new_vals[j] < 0 || new_vals[j] >= ca->v) {
      return 0;
    }
    int column = IToC[change_set_idx][j];
    if (column < 0 || column >= ca->k) {
      return 0;
    }
  }
  return 1;
}

/*
 * Computes the coverage delta when changing t columns in a row.
 *
 * The change_set_idx selects which t columns to modify (from IToC matrix).
 * new_vals contains the new values for those t columns.
 *
 * Does NOT modify the array - purely computational.
 *
 * Returns the delta (change in ca->covered).
 */
ssize_t ca_compute_tcolumns_delta(covering_array_t *ca,
                                  const ca_affected_t *pre, int **IToC,
                                  int row_idx, uint16_t change_set_idx,
                                  const int *new_vals) {
  if (!tcolumns_args_valid(ca, pre, IToC, row_idx, change_set_idx, new_vals,
                           0)) {
    return 0;
  }

  int t = (int)pre->t;

  const uint16_t *affected = precompute_get_affected(pre, change_set_idx);
  size_t affected_count = precompute_get_affected_count(pre);
  int v = ca->v;
  int *row = ca->matrix[row_idx];

  /* Get the t columns that are changing */
  int *changing_cols = IToC[change_set_idx];

  /* Save current values of the changing columns */
  int saved_vals[TCOL_MAX_T];
  for (int j = 0; j < t; j++) {
    saved_vals[j] = row[changing_cols[j]];
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
    return 0;
  }

  ssize_t delta = 0;

  for (size_t i = 0; i < affected_count; i++) {
    uint16_t itoc_idx = affected[i];

    /* Compute old encoding (restore old values) */
    for (int j = 0; j < t; j++) {
      row[changing_cols[j]] = saved_vals[j];
    }
    int old_encoding = get_col(row, IToC, (int)itoc_idx, t, v);

    /* Compute new encoding (apply new values) */
    for (int j = 0; j < t; j++) {
      row[changing_cols[j]] = new_vals[j];
    }
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

  /* Restore original values */
  for (int j = 0; j < t; j++) {
    row[changing_cols[j]] = saved_vals[j];
  }

  return delta;
}

/*
 * Applies a t-column change and updates the coverage matrix P in place.
 *
 * Always writes the new values and always brings P, ca->covered and
 * ca->tcomb_counter into agreement with them. A net-zero delta is NOT a no-op
 * -- see the note on ca_apply_cell_change() for why the previous early return
 * silently dropped changes.
 *
 * Returns the delta (change in ca->covered).
 */
ssize_t ca_apply_tcolumns_change(covering_array_t *ca, const ca_affected_t *pre,
                                 int **IToC, int row_idx,
                                 uint16_t change_set_idx, const int *new_vals) {
  if (!tcolumns_args_valid(ca, pre, IToC, row_idx, change_set_idx, new_vals,
                           1)) {
    return 0;
  }

  int t = (int)pre->t;

  const uint16_t *affected = precompute_get_affected(pre, change_set_idx);
  size_t affected_count = precompute_get_affected_count(pre);
  int v = ca->v;
  int *row = ca->matrix[row_idx];

  /* Get the t columns that are changing */
  int *changing_cols = IToC[change_set_idx];

  /* Save current values */
  int saved_vals[TCOL_MAX_T];
  for (int j = 0; j < t; j++) {
    saved_vals[j] = row[changing_cols[j]];
  }

  int any_change = 0;
  for (int j = 0; j < t; j++) {
    if (new_vals[j] != saved_vals[j]) {
      any_change = 1;
      break;
    }
  }
  if (!any_change) {
    return 0; /* the one genuine no-op */
  }

  ssize_t delta = 0;

  /* Single pass: derive the delta and update P together. */
  for (size_t i = 0; i < affected_count; i++) {
    uint16_t itoc_idx = affected[i];

    for (int j = 0; j < t; j++) {
      row[changing_cols[j]] = saved_vals[j];
    }
    int old_encoding = get_col(row, IToC, (int)itoc_idx, t, v);

    for (int j = 0; j < t; j++) {
      row[changing_cols[j]] = new_vals[j];
    }
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

  /* Apply the changes to the matrix */
  for (int j = 0; j < t; j++) {
    row[changing_cols[j]] = new_vals[j];
  }

  ca->covered = (size_t)((ssize_t)ca->covered + delta);

  return delta;
}
