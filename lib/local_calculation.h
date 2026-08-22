#ifndef LOCAL_CALCULATION_H
#define LOCAL_CALCULATION_H

#include "covering_array.h"
#include "precompute.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/*
 * Incremental coverage updates for single-cell changes.
 *
 * Changing one cell (row_idx, col_idx) can only affect the column-sets that
 * contain col_idx -- C(k-1, t-1) of the C(k,t) total. Both functions here walk
 * exactly that set, via precompute_get_col_affected(), instead of rescoring the
 * whole array. See docs/README.md for the index model.
 *
 * Pair them as: ca_compute_cell_delta() to score candidate values without
 * touching anything, then ca_apply_cell_change() once to commit the winner.
 */

/*
 * Scores a candidate cell value without modifying the array.
 *
 * Returns how ca->covered would change if ca->matrix[row_idx][col_idx] became
 * new_val: positive if the move covers more t-way combinations than it loses,
 * negative if it loses more, zero if it breaks even (which includes both "no
 * combination changes state" and "one is covered while another is uncovered").
 *
 * Preconditions: ca->P must already exist and be correct, which means
 *                ca_validate() or pv_validate() has run since the last direct
 *                write to ca->matrix. IToC must be a C(k,t) x t table filled by
 *                t_wise(IToC, ca->k, ca->t), and pre must come from
 *                precompute_create(ca->k, ca->t) -- the same k and t as ca.
 * Ownership:     borrows everything; frees nothing; leaves ca unchanged.
 * Returns:       the coverage delta, or 0 if any argument is NULL, ca->P is
 *                NULL, or new_val equals the current value.
 * Cost:          2 * C(k-1, t-1) get_col() evaluations. No allocation.
 */
ssize_t ca_compute_cell_delta(covering_array_t *ca, const ca_affected_t *pre,
                              int **IToC, int row_idx, int col_idx,
                              int new_val);

/*
 * Writes new_val into the cell and brings the coverage state up to date.
 *
 * Updates ca->matrix, ca->P, ca->covered and ca->tcomb_counter together, so
 * afterwards all of them agree with what a fresh ca_validate() on the same
 * matrix would produce. That equivalence is enforced by
 * unittests/test_regression.c.
 *
 * A return of 0 means "no NET change in coverage" -- it does not mean the cell
 * was left alone. The change is always applied unless new_val already equals
 * the current value. (An earlier version returned early on a zero delta and
 * silently discarded the move, which quietly corrupted P for any search that
 * accepted plateau moves.)
 *
 * Preconditions: as ca_compute_cell_delta(), and ca->tcomb_counter must also
 *                exist -- ca_validate() and pv_validate() allocate both.
 * Ownership:     borrows everything; frees nothing.
 * Returns:       the coverage delta, or 0 if any argument is NULL, ca->P or
 *                ca->tcomb_counter is NULL, or new_val equals the current
 *                value (the only genuine no-op).
 * Cost:          2 * C(k-1, t-1) get_col() evaluations, in a single pass that
 *                both scores and commits. No allocation.
 */
ssize_t ca_apply_cell_change(covering_array_t *ca, const ca_affected_t *pre,
                             int **IToC, int row_idx, int col_idx, int new_val);

#endif
