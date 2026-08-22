#ifndef PRECOMPUTE_H
#define PRECOMPUTE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Precomputed "which column-sets does this change touch?" lookup tables.
 *
 * When a search changes part of a row, only some of the C(k,t) column-sets can
 * change coverage: the ones that contain at least one changed column. Working
 * that out per move would cost as much as the move saves, so it is computed
 * once here and reused.
 *
 * Two independent views are stored, matching the two kinds of move:
 *
 *   PER-COLUMN     -- for changing ONE cell (local_calculation.h).
 *                     precompute_get_col_affected(pre, col) lists the
 *                     C(k-1, t-1) column-sets containing col.
 *
 *   PER-CHANGE-SET -- for changing t columns at once (t_columns_delta.h).
 *                     precompute_get_affected(pre, j) lists the
 *                     C(k,t) - C(k-t,t) column-sets sharing at least one
 *                     column with column-set j.
 *
 * Both return const uint16_t * and both return COLUMN-SET INDICES -- the same
 * j used for IToC rows, ca->P rows and ca->tcomb_counter. They differ in what
 * you look up by (a column versus a column-set) and in how many entries come
 * back, so read the count from the matching accessor rather than assuming.
 */
typedef struct {
  size_t k;                    /* columns this table was built for */
  size_t t;                    /* strength this table was built for */
  size_t change_sets;          /* C(k,t): valid change-set indices and the
                                  number of column-sets overall */
  size_t affected_per_change;  /* C(k,t) - C(k-t,t): entries per change-set */
  uint16_t *indices;           /* per-change-set view, change_sets rows of
                                  affected_per_change entries */

  size_t entries_per_col;      /* C(k-1, t-1): entries per column */
  size_t *col_offsets;         /* k + 1 offsets into col_indices */
  uint16_t *col_indices;       /* per-column view, k runs of entries_per_col */
} ca_affected_t;

/*
 * Builds both lookup tables for a given k and t.
 *
 * k and t must match the array you intend to use this with: the indices stored
 * here are only meaningful against a C(k,t) column-set ordering built with the
 * same parameters. Nothing checks this at use time, so a mismatch is silent
 * corruption rather than an error.
 *
 * Limits: 1 <= t <= k, C(k,t) <= 65535 (indices are uint16_t),
 *         C(k-1,t-1) <= 65535, and the tables must fit in 16 GB. Each failure
 *         prints which limit was hit.
 *
 * Ownership: caller must release with precompute_destroy().
 * Returns:   the tables, or NULL if a limit is exceeded or allocation fails.
 * Cost:      builds a temporary C(k,t) x t column-set table and scans it once
 *            per column, then once per change-set.
 */
ca_affected_t *precompute_create(size_t k, size_t t);

/*
 * Releases the tables. Accepts NULL.
 */
void precompute_destroy(ca_affected_t *pre);

/*
 * PER-CHANGE-SET view: the column-sets whose coverage can change when the t
 * columns of column-set change_set_idx are all reassigned.
 *
 * Use with t_columns_delta.h. The returned block holds exactly
 * precompute_get_affected_count(pre) entries and is owned by pre.
 *
 * Returns: pointer to the run, or NULL if pre is NULL or change_set_idx is
 *          >= pre->change_sets.
 */
const uint16_t *precompute_get_affected(const ca_affected_t *pre,
                                        size_t change_set_idx);

/*
 * Entries in every per-change-set run: C(k,t) - C(k-t,t).
 * Returns 0 if pre is NULL.
 */
size_t precompute_get_affected_count(const ca_affected_t *pre);

/*
 * PER-COLUMN view: the column-sets that contain the given column, and so the
 * only ones whose coverage can change when a single cell in that column moves.
 *
 * Use with local_calculation.h. The returned block holds exactly
 * precompute_get_col_affected_count(pre) entries and is owned by pre.
 *
 * Returns: pointer to the run, or NULL if pre is NULL or column is >= pre->k.
 */
const uint16_t *precompute_get_col_affected(const ca_affected_t *pre,
                                            size_t column);

/*
 * Entries in every per-column run: C(k-1, t-1). Every column appears in
 * exactly that many column-sets, so one count serves all of them.
 * Returns 0 if pre is NULL.
 */
size_t precompute_get_col_affected_count(const ca_affected_t *pre);

#endif
