#ifndef PARALLEL_VALIDATOR_H
#define PARALLEL_VALIDATOR_H

#include "covering_array.h"

/*
 * OpenMP-parallel equivalent of ca_validate().
 *
 * Recomputes the entire coverage state from ca->matrix -- allocating P and
 * tcomb_counter on first call -- and produces results identical to
 * ca_validate() for the same array. The work is split across threads by
 * column-set, so each thread owns a disjoint range of P rows.
 *
 * Idempotent: both P and tcomb_counter are reset before counting, so calling
 * this twice gives the same answer as calling it once. (An earlier version
 * reset only tcomb_counter, so a second call double-counted P and reported
 * combinations as missing that were already covered.)
 *
 * Unlike ca_validate() this returns void. For a non-NULL ca it clears covered
 * and total before doing any work, including when the matrix/parameters are
 * invalid or allocation fails. Read total afterwards to tell whether validation
 * completed, and compare covered with total for the verdict:
 *
 *     pv_validate(ca);
 *     int fully_covering = (ca->total > 0 && ca->covered == ca->total);
 *
 * Preconditions: ca->matrix filled. Link with OpenMP (see the Makefile).
 * Thread safety: internally parallel -- do NOT call it concurrently on the
 *                same array, and do not read ca's coverage fields while it
 *                runs.
 * Cost:          N * C(k,t) get_col() evaluations spread across threads, plus
 *                one C(k,t) x t allocation for its column-set table.
 */
void pv_validate(covering_array_t *ca);

#endif
