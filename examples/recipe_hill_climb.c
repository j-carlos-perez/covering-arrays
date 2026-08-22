/*
 * RECIPE: incremental local search over a fixed-size array.
 *
 * Where recipe_greedy_rows grows an array, this one holds N fixed and improves
 * the cells in place -- the shape most covering-array metaheuristics take.
 *
 * The point of the exercise is that the coverage state is maintained
 * INCREMENTALLY. Nothing here calls ca_validate() inside the loop; each move
 * touches only the C(k-1,t-1) column-sets that contain the changed column, via
 * the precompute tables. The final check proves the running state still equals
 * a full recompute.
 *
 * Shows:
 *   - precompute_create + a caller-owned IToC as the search scaffolding
 *   - ca_compute_cell_delta to score a candidate without committing
 *   - ca_apply_cell_change to commit, including PLATEAU moves (delta == 0)
 *   - Set64 holding the uncovered (j,c) pairs so a random gap can be picked
 *     in O(1) instead of scanning P
 *
 * See docs/README.md sections 2 (indices), 4 (ownership) and 5 (invariants).
 *
 * Usage: recipe_hill_climb <N> <t> <k> <v>
 */

#include "lib/combinatorial.h"
#include "lib/covering_array.h"
#include "lib/local_calculation.h"
#include "lib/memory.h"
#include "lib/precompute.h"
#include "lib/set64.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * Rebuilds the uncovered-pair set from scratch.
 *
 * Key packing matters here: set64_make_key gives `a` only 15 bits, so the
 * TUPLE index c goes in `a` and the COLUMN-SET index j in `b`. C(k,t) can
 * reach 65535, which would not survive being packed into `a`.
 */
static void rebuild_uncovered(Set64 *set, const covering_array_t *ca, size_t R,
                              size_t C) {
  for (size_t j = 0; j < R; j++) {
    for (size_t c = 0; c < C; c++) {
      if (ca->P[j][c] == 0) {
        set64_insert(set, set64_make_key((uint32_t)c, (uint32_t)j));
      }
    }
  }
}

/*
 * Re-syncs the set for just the column-sets a change to `col` could have
 * touched -- which is exactly what precompute_get_col_affected() lists.
 *
 * Insert and delete are both idempotent (a duplicate insert is ignored, a
 * delete of an absent key returns false), so we can push the current truth
 * without tracking what changed.
 */
static void sync_after_change(Set64 *set, const covering_array_t *ca,
                              const ca_affected_t *pre, int col, size_t C) {
  const uint16_t *affected = precompute_get_col_affected(pre, (size_t)col);
  size_t n = precompute_get_col_affected_count(pre);

  for (size_t i = 0; i < n; i++) {
    uint16_t j = affected[i];
    for (size_t c = 0; c < C; c++) {
      uint64_t key = set64_make_key((uint32_t)c, (uint32_t)j);
      if (ca->P[j][c] == 0) {
        set64_insert(set, key);
      } else {
        set64_delete(set, key);
      }
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Usage: %s <N> <t> <k> <v>\n", argv[0]);
    fprintf(stderr, "  N: rows, t: strength, k: columns, v: alphabet size\n");
    fprintf(stderr, "\nExample: %s 12 2 6 3\n", argv[0]);
    return 1;
  }

  int N = atoi(argv[1]);
  int t = atoi(argv[2]);
  int k = atoi(argv[3]);
  int v = atoi(argv[4]);

  if (!ca_params_valid(N, k, v, t)) {
    fprintf(stderr,
            "Error: unusable parameters (need N >= 1, v >= 2, 1 <= t <= k, "
            "N <= %d)\n",
            CA_COUNT_MAX);
    return 1;
  }

  size_t C = 1;
  for (int i = 0; i < t; i++) {
    C *= (size_t)v;
  }
  if (C > (1u << SET64_A_BITS)) {
    fprintf(stderr,
            "Error: v^t = %zu exceeds what set64_make_key can pack into `a` "
            "(%u). Use smaller t or v for this recipe.\n",
            C, 1u << SET64_A_BITS);
    return 1;
  }

  srand((unsigned int)time(NULL));

  /* ---------------------------------------------------------------------
   * STEP 1: create and fill the array.
   * ------------------------------------------------------------------- */
  covering_array_t *ca = ca_create(N, k, v, t);
  if (ca == NULL) {
    fprintf(stderr, "Error: failed to create covering array\n");
    return 1;
  }
  if (ca_init_random_balanced(ca) != 0) {
    fprintf(stderr, "Error: failed to initialize covering array\n");
    ca_destroy(ca);
    return 1;
  }

  /* ---------------------------------------------------------------------
   * STEP 2: validate once. This allocates P and tcomb_counter.
   *
   * Without it the delta functions below find ca->P == NULL and silently
   * return 0 -- the search would appear to run and do nothing.
   * ------------------------------------------------------------------- */
  ca_validate(ca);
  if (ca->total == 0) {
    fprintf(stderr, "Error: failed to initialize coverage state\n");
    ca_destroy(ca);
    return 1;
  }
  size_t R = (size_t)binomial(k, t);
  size_t initial_covered = ca->covered;

  printf("=== Incremental hill climb ===\n");
  printf("  CA(%d; %d, %d, %d)\n", N, t, k, v);
  printf("  %zu column-sets x %zu tuples = %zu combinations\n", R, C,
         ca->total);
  printf("  after random init: %zu/%zu (%.1f%%)\n\n", ca->covered, ca->total,
         100.0 * (double)ca->covered / (double)ca->total);

  /* ---------------------------------------------------------------------
   * STEP 3: build the search scaffolding.
   *
   * Both are CALLER-OWNED and must use the same k and t as the array. Delta
   * calls reject a mismatched precompute table with a zero/no-op result.
   * ------------------------------------------------------------------- */
  ca_affected_t *pre = precompute_create((size_t)k, (size_t)t);
  if (pre == NULL) {
    fprintf(stderr, "Error: failed to build precompute tables\n");
    ca_destroy(ca);
    return 1;
  }

  int **IToC = get_matrix(R, (size_t)t);
  if (IToC == NULL || t_wise(IToC, k, t) != 0) {
    fprintf(stderr, "Error: failed to build the column-set table\n");
    free_matrix(IToC, R);
    precompute_destroy(pre);
    ca_destroy(ca);
    return 1;
  }

  printf("  precompute: %zu column-sets touched per single-cell change "
         "(C(k-1,t-1))\n",
         precompute_get_col_affected_count(pre));
  printf("  a full revalidation would touch all %zu\n\n", R);

  /* The uncovered-pair worklist. */
  Set64 *uncovered = set64_create((uint32_t)(R * C / 4 + 16));
  if (uncovered == NULL) {
    fprintf(stderr, "Error: failed to create the uncovered set\n");
    free_matrix(IToC, R);
    precompute_destroy(pre);
    ca_destroy(ca);
    return 1;
  }
  rebuild_uncovered(uncovered, ca, R, C);
  printf("  uncovered worklist holds %u pairs\n\n", uncovered->size);

  /* ---------------------------------------------------------------------
   * STEP 4: the search loop.
   * ------------------------------------------------------------------- */
  int *symbols = get_vector((size_t)t);
  if (symbols == NULL) {
    fprintf(stderr, "Error: out of memory\n");
    set64_free(uncovered);
    free_matrix(IToC, R);
    precompute_destroy(pre);
    ca_destroy(ca);
    return 1;
  }

  const int max_steps = 20000;
  int improving_moves = 0, plateau_moves = 0, rejected = 0;

  for (int step = 0; step < max_steps && ca->covered < ca->total; step++) {
    /* Pick a random still-uncovered combination to attack. This is what the
       Set64 buys: O(1), versus scanning P for a zero. */
    uint64_t key = set64_random(uncovered);
    if (key == SET64_EMPTY_KEY) {
      break; /* nothing left uncovered */
    }
    size_t target_c = set64_key_get_a(key);
    size_t target_j = set64_key_get_b(key);

    /* Decode the tuple we want, so we know which symbol each of the t
       columns of column-set target_j would need. */
    if (inv_ruffini(symbols, (int)target_c, v, t) != 0) {
      continue;
    }

    /* Aim one cell at it: a random row, and a random one of those t columns. */
    int row_idx = rand_below(ca->N);
    int which = rand_below(t);
    int col_idx = IToC[target_j][which];
    int wanted = symbols[which];

    /* Score the move without committing. */
    ssize_t delta = ca_compute_cell_delta(ca, pre, IToC, row_idx, col_idx,
                                          wanted);

    if (delta > 0) {
      ca_apply_cell_change(ca, pre, IToC, row_idx, col_idx, wanted);
      sync_after_change(uncovered, ca, pre, col_idx, C);
      improving_moves++;
    } else if (delta == 0) {
      /* PLATEAU MOVE. A zero delta means no NET change in coverage -- not
         that nothing would happen. The cell really does change, and P,
         covered and tcomb_counter are all updated to match. Accepting these
         is what lets a search drift across a flat region instead of getting
         stuck; it is only safe because apply always applies. */
      if (rand_below(4) == 0) {
        ca_apply_cell_change(ca, pre, IToC, row_idx, col_idx, wanted);
        sync_after_change(uncovered, ca, pre, col_idx, C);
        plateau_moves++;
      } else {
        rejected++;
      }
    } else {
      rejected++; /* would lose coverage; leave it alone */
    }
  }

  free_vector(symbols);

  printf("=== Search finished ===\n");
  printf("  improving moves: %d\n", improving_moves);
  printf("  plateau moves:   %d  (delta == 0, applied anyway)\n",
         plateau_moves);
  printf("  rejected:        %d\n", rejected);
  printf("  coverage: %zu -> %zu / %zu (%.1f%%) - %s\n\n", initial_covered,
         ca->covered, ca->total,
         100.0 * (double)ca->covered / (double)ca->total,
         ca->covered == ca->total ? "COMPLETE" : "incomplete");

  /* ---------------------------------------------------------------------
   * STEP 5: prove the incremental state is exact.
   *
   * Copy the matrix into a fresh array and validate it from scratch. Every
   * part of the coverage state must agree with what the incremental updates
   * produced -- this is the invariant in docs/README.md section 5.
   * ------------------------------------------------------------------- */
  printf("=== Incremental vs. full recompute ===\n");
  covering_array_t *reference = ca_create(ca->N, ca->k, ca->v, ca->t);
  if (reference == NULL) {
    fprintf(stderr, "Error: failed to create the reference array\n");
  } else {
    for (int i = 0; i < ca->N; i++) {
      for (int j = 0; j < ca->k; j++) {
        reference->matrix[i][j] = ca->matrix[i][j];
      }
    }
    ca_validate(reference);

    int p_match = 1, tc_match = 1;
    for (size_t j = 0; j < R; j++) {
      if (reference->tcomb_counter[j] != ca->tcomb_counter[j]) {
        tc_match = 0;
      }
      for (size_t c = 0; c < C; c++) {
        if (reference->P[j][c] != ca->P[j][c]) {
          p_match = 0;
        }
      }
    }

    printf("  covered:       incremental %zu, recomputed %zu -> %s\n",
           ca->covered, reference->covered,
           ca->covered == reference->covered ? "match" : "MISMATCH");
    printf("  P matrix:      %s\n", p_match ? "match" : "MISMATCH");
    printf("  tcomb_counter: %s\n", tc_match ? "match" : "MISMATCH");
    printf("\n  %s\n",
           (p_match && tc_match && ca->covered == reference->covered)
               ? "The incremental state is exactly a full revalidation."
               : "*** INCREMENTAL STATE DIVERGED ***");

    ca_destroy(reference);
  }

  /* Free what we own; ca_destroy handles matrix, P and tcomb_counter. */
  set64_free(uncovered);
  free_matrix(IToC, R);
  precompute_destroy(pre);
  ca_destroy(ca);

  printf("Done.\n");
  return 0;
}
