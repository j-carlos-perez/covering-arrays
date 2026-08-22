/*
 * RECIPE: build a covering array from nothing, one row at a time.
 *
 * The classic greedy construction. Start with an empty array; repeatedly
 * generate candidate rows, keep the one covering the most currently-uncovered
 * t-way combinations, and append it. Stop when everything is covered.
 *
 * Shows the whole non-search surface of the library:
 *   - ca_create / ca_validate to establish coverage state
 *   - reading ca->tcomb_counter to see which column-sets are worst off
 *   - scoring a candidate row against ca->P without committing to it
 *   - ca_add_row + ca_add_row_coverage to commit
 *   - ca_save, and a final independent re-validation
 *
 * See docs/README.md sections 3 (lifecycle) and 5 (invariants).
 *
 * Usage: recipe_greedy_rows <t> <k> <v> <output_folder>
 */

#include "lib/combinatorial.h"
#include "lib/covering_array.h"
#include "lib/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CANDIDATES_PER_ROW 40
#define MAX_ROWS 2000

/*
 * Counts how many currently-uncovered (j, c) pairs a candidate row would
 * newly cover. Reads ca->P; changes nothing.
 *
 * This is the greedy score. It needs ca->P to exist, so ca_validate() must
 * have run first.
 */
static size_t score_candidate(const covering_array_t *ca, const int *row,
                              int **IToC, size_t R) {
  size_t newly_covered = 0;
  for (size_t j = 0; j < R; j++) {
    int c = get_col(row, IToC, (int)j, ca->t, ca->v);
    if (c != -1 && ca->P[j][c] == 0) {
      newly_covered++;
    }
  }
  return newly_covered;
}

/*
 * Builds a candidate row biased toward the column-set that is furthest from
 * complete.
 *
 * tcomb_counter[j] is how many tuples column-set j is still missing, so the
 * largest entry names the worst-covered columns. We pick one of its missing
 * tuples, pin those t columns to it, and fill the rest at random. That is the
 * cheap way to aim a candidate at a real gap rather than hoping.
 */
static void build_targeted_candidate(const covering_array_t *ca, int *row,
                                     int **IToC, size_t R, size_t worst_j) {
  for (int col = 0; col < ca->k; col++) {
    row[col] = rand_below(ca->v);
  }
  (void)R;

  /* Find a tuple that column-set worst_j does not yet have. */
  size_t C = 1;
  for (int i = 0; i < ca->t; i++) {
    C *= (size_t)ca->v;
  }

  size_t start = (size_t)rand_below((int)C);
  for (size_t step = 0; step < C; step++) {
    size_t c = (start + step) % C;
    if (ca->P[worst_j][c] == 0) {
      /* Decode that tuple and pin the columns of worst_j to it. */
      int *symbols = get_vector((size_t)ca->t);
      if (symbols != NULL && inv_ruffini(symbols, (int)c, ca->v, ca->t) == 0) {
        for (int i = 0; i < ca->t; i++) {
          row[IToC[worst_j][i]] = symbols[i];
        }
      }
      free_vector(symbols);
      return;
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Usage: %s <t> <k> <v> <output_folder>\n", argv[0]);
    fprintf(stderr, "  t: strength, k: columns, v: alphabet size\n");
    fprintf(stderr, "\nExample: %s 2 6 3 ./output_test\n", argv[0]);
    return 1;
  }

  int t = atoi(argv[1]);
  int k = atoi(argv[2]);
  int v = atoi(argv[3]);
  const char *output_folder = argv[4];

  /* Screen parameters explicitly so we can report which limit was hit;
     ca_create applies the same test but only says "failed". */
  if (!ca_params_valid(1, k, v, t)) {
    fprintf(stderr,
            "Error: unusable parameters (need v >= 2 and 1 <= t <= k)\n");
    return 1;
  }

  srand((unsigned int)time(NULL));

  /* ---------------------------------------------------------------------
   * STEP 1: start from a single random row.
   *
   * ca_create needs N >= 1, so we begin with one row and grow from there.
   * ------------------------------------------------------------------- */
  covering_array_t *ca = ca_create(1, k, v, t);
  if (ca == NULL) {
    fprintf(stderr, "Error: failed to create covering array\n");
    return 1;
  }
  if (ca_init_random(ca) != 0) {
    fprintf(stderr, "Error: failed to initialize covering array\n");
    ca_destroy(ca);
    return 1;
  }

  /* ---------------------------------------------------------------------
   * STEP 2: validate, which ALLOCATES ca->P and ca->tcomb_counter.
   *
   * Everything below reads those, so this step is mandatory, not optional.
   * ------------------------------------------------------------------- */
  ca_validate(ca);
  if (ca->total == 0) {
    fprintf(stderr, "Error: failed to initialize coverage state\n");
    ca_destroy(ca);
    return 1;
  }

  size_t R = (size_t)binomial(k, t);
  printf("=== Greedy row construction ===\n");
  printf("  target: CA(N; %d, %d, %d)\n", t, k, v);
  printf("  %zu column-sets x %zu tuples = %zu combinations to cover\n", R,
         ca->total / R, ca->total);
  printf("  starting from 1 random row: %zu/%zu covered\n\n", ca->covered,
         ca->total);

  /* Our own column-set table. Caller-owned: we allocate it, we free it. */
  int **IToC = get_matrix(R, (size_t)t);
  if (IToC == NULL || t_wise(IToC, k, t) != 0) {
    fprintf(stderr, "Error: failed to build the column-set table\n");
    free_matrix(IToC, R);
    ca_destroy(ca);
    return 1;
  }

  int *candidate = get_vector((size_t)k);
  int *best_row = get_vector((size_t)k);
  if (candidate == NULL || best_row == NULL) {
    fprintf(stderr, "Error: out of memory\n");
    free_vector(candidate);
    free_vector(best_row);
    free_matrix(IToC, R);
    ca_destroy(ca);
    return 1;
  }

  /* ---------------------------------------------------------------------
   * STEP 3: the greedy loop.
   * ------------------------------------------------------------------- */
  while (ca->covered < ca->total && ca->N < MAX_ROWS) {
    /* Which column-set is furthest from complete? */
    size_t worst_j = 0;
    size_t worst_missing = 0;
    for (size_t j = 0; j < R; j++) {
      if (ca->tcomb_counter[j] > worst_missing) {
        worst_missing = ca->tcomb_counter[j];
        worst_j = j;
      }
    }

    /* Generate candidates and keep the best-scoring one. */
    size_t best_score = 0;
    for (int attempt = 0; attempt < CANDIDATES_PER_ROW; attempt++) {
      build_targeted_candidate(ca, candidate, IToC, R, worst_j);
      size_t score = score_candidate(ca, candidate, IToC, R);
      if (score > best_score || attempt == 0) {
        best_score = score;
        for (int col = 0; col < k; col++) {
          best_row[col] = candidate[col];
        }
      }
    }

    if (best_score == 0) {
      printf("  no candidate covers anything new; stopping early\n");
      break;
    }

    /* Commit. ca_add_row stores the row; ca_add_row_coverage folds it into
       P, covered and tcomb_counter. They are independent, so both are
       needed to keep matrix and coverage state in step. */
    if (ca_add_row(ca, best_row) != 0) {
      fprintf(stderr, "Error: failed to append row\n");
      break;
    }
    if (ca_add_row_coverage(ca, best_row) != 0) {
      fprintf(stderr, "Error: failed to update row coverage\n");
      break;
    }

    if (ca->N <= 10 || ca->N % 5 == 0 || ca->covered == ca->total) {
      printf("  row %-3d covers %-4zu new -> %zu/%zu (%.1f%%)\n", ca->N,
             best_score, ca->covered, ca->total,
             100.0 * (double)ca->covered / (double)ca->total);
    }
  }

  printf("\n=== Result ===\n");
  printf("  N = %d rows, coverage %zu/%zu (%.1f%%) - %s\n", ca->N, ca->covered,
         ca->total, 100.0 * (double)ca->covered / (double)ca->total,
         ca->covered == ca->total ? "COMPLETE" : "INCOMPLETE");

  /* ---------------------------------------------------------------------
   * STEP 4: check the incremental bookkeeping against a full recompute.
   *
   * ca_add_row_coverage maintained P/covered/tcomb_counter as we went; a
   * fresh ca_validate recomputes them from the matrix alone. They must agree.
   * ------------------------------------------------------------------- */
  size_t incremental_covered = ca->covered;
  ca_validate(ca);
  printf("\n=== Sanity check ===\n");
  printf("  incremental: %zu\n", incremental_covered);
  printf("  recomputed:  %zu\n", ca->covered);
  printf("  %s\n", incremental_covered == ca->covered
                       ? "MATCH - bookkeeping is consistent"
                       : "*** MISMATCH ***");

  if (ca_save(output_folder, ca, "Built by recipe_greedy_rows") == 0) {
    printf("\nSaved to: %s\n", output_folder);
  } else {
    fprintf(stderr, "\nWarning: failed to save (does '%s' exist?)\n",
            output_folder);
  }

  /* Free what we own; ca_destroy handles matrix, P and tcomb_counter. */
  free_vector(candidate);
  free_vector(best_row);
  free_matrix(IToC, R);
  ca_destroy(ca);

  printf("Done.\n");
  return 0;
}
