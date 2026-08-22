#ifndef T_COLUMNS_DELTA_H
#define T_COLUMNS_DELTA_H

#include "covering_array.h"
#include "precompute.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/*
 * Incremental coverage updates for changing all t columns of one column-set
 * at once.
 *
 * Where local_calculation.h moves a single cell, these move the whole t-tuple
 * in one row -- the move that can cover a specific missing combination
 * outright. The columns to change are named by a change-set index rather than
 * a column list: change_set_idx is an index into the same lexicographic
 * C(k,t) table that t_wise() produces, so IToC[change_set_idx] holds the t
 * columns and new_vals supplies their t new symbols in that same order.
 *
 * Because t columns move together, the affected set is larger than for a
 * single cell: C(k,t) - C(k-t,t) column-sets share at least one column with
 * the changed ones, reached via precompute_get_affected(). See docs/README.md.
 */

/*
 * Scores a candidate t-column assignment without modifying the array.
 *
 * Returns how ca->covered would change if the t columns named by
 * IToC[change_set_idx] in row row_idx took the symbols in new_vals.
 *
 * Preconditions: ca->P must already exist and be correct -- ca_validate() or
 *                pv_validate() has run since the last direct write to
 *                ca->matrix. IToC must be a C(k,t) x t table filled by
 *                t_wise(IToC, ca->k, ca->t); pre must come from
 *                precompute_create(ca->k, ca->t). new_vals must hold t symbols
 *                in [0, v-1], ordered to match IToC[change_set_idx].
 *                Strength t must be <= 16.
 * Ownership:     borrows everything; frees nothing; restores ca->matrix before
 *                returning, so the array is unchanged on exit.
 * Returns:       the coverage delta, or 0 if any argument is NULL, ca->P is
 *                NULL, change_set_idx is out of range, t is out of range, or
 *                new_vals matches the current values.
 * Cost:          2 * (C(k,t) - C(k-t,t)) get_col() evaluations. No allocation.
 */
ssize_t ca_compute_tcolumns_delta(covering_array_t *ca,
                                  const ca_affected_t *pre, int **IToC,
                                  int row_idx, uint16_t change_set_idx,
                                  const int *new_vals);

/*
 * Writes new_vals into the t columns and brings the coverage state up to date.
 *
 * Updates ca->matrix, ca->P, ca->covered and ca->tcomb_counter together, so
 * afterwards all of them agree with a fresh ca_validate() on the same matrix.
 *
 * As with ca_apply_cell_change(), a return of 0 means "no NET coverage change",
 * not "nothing was written". The assignment is always applied unless new_vals
 * already equals the current values.
 *
 * Preconditions: as ca_compute_tcolumns_delta(), and ca->tcomb_counter must
 *                also exist.
 * Ownership:     borrows everything; frees nothing.
 * Returns:       the coverage delta, or 0 if any argument is NULL, ca->P or
 *                ca->tcomb_counter is NULL, change_set_idx or t is out of
 *                range, or new_vals matches the current values.
 * Cost:          2 * (C(k,t) - C(k-t,t)) get_col() evaluations, in a single
 *                pass that both scores and commits. No allocation.
 */
ssize_t ca_apply_tcolumns_change(covering_array_t *ca, const ca_affected_t *pre,
                                 int **IToC, int row_idx,
                                 uint16_t change_set_idx, const int *new_vals);

#endif
