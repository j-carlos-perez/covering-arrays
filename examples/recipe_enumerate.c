/*
 * RECIPE: the combinatorial building blocks, with no covering array involved.
 *
 * Demonstrates the index scheme that everything else in lib/ is built on:
 *   - column-set index j, produced by t_wise() in lexicographic order
 *   - tuple index c, produced by get_col() and decoded by inv_ruffini()
 * plus the counting and iterator primitives.
 *
 * See docs/README.md section 2 for the model this makes concrete.
 *
 * Usage: recipe_enumerate <k> <t> <v>
 */

#include "lib/combinatorial.h"
#include "lib/memory.h"
#include <stdio.h>
#include <stdlib.h>

/* Callback for t_wise_visit(): prints each column-set as it is visited.
   `combination` is reused between calls, so copy it if you need to keep it. */
static void print_combination(int *combination, int index, int k, int t,
                              void *user_data) {
  (void)k;
  int *limit = (int *)user_data;
  if (index >= *limit) {
    return; /* keep the output short for large C(k,t) */
  }
  printf("    j=%-3d {", index);
  for (int i = 0; i < t; i++) {
    printf("%d%s", combination[i], i < t - 1 ? "," : "");
  }
  printf("}\n");
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <k> <t> <v>\n", argv[0]);
    fprintf(stderr, "  k: columns, t: strength, v: alphabet size\n");
    fprintf(stderr, "\nExample: %s 4 2 3\n", argv[0]);
    return 1;
  }

  int k = atoi(argv[1]);
  int t = atoi(argv[2]);
  int v = atoi(argv[3]);

  if (k < 1 || t < 1 || t > k || v < 2) {
    fprintf(stderr, "Error: need 1 <= t <= k and v >= 2\n");
    return 1;
  }

  /* ---------------------------------------------------------------------
   * STEP 1: count the column-sets.
   *
   * binomial() reports overflow rather than wrapping, so anything used as an
   * allocation size has to be screened before it is trusted.
   * ------------------------------------------------------------------- */
  uint64_t R64 = binomial(k, t);
  if (!binomial_is_usable(R64)) {
    fprintf(stderr, "Error: C(%d,%d) is too large to work with\n", k, t);
    return 1;
  }
  size_t R = (size_t)R64;

  size_t C = 1;
  for (int i = 0; i < t; i++) {
    C *= (size_t)v;
  }

  printf("=== Parameters ===\n");
  printf("  k=%d columns, t=%d strength, v=%d symbols\n", k, t, v);
  printf("  C(k,t) = %zu column-sets   (index j)\n", R);
  printf("  v^t    = %zu tuples        (index c)\n", C);
  printf("  a full covering array must cover all %zu (j,c) pairs\n\n", R * C);

  /* What overflow screening is for: */
  printf("=== binomial() overflow screening ===\n");
  printf("  C(63,29) = %llu  (usable: %s)\n",
         (unsigned long long)binomial(63, 29),
         binomial_is_usable(binomial(63, 29)) ? "yes" : "no");
  printf("  C(70,35) = %s\n\n",
         binomial(70, 35) == BINOMIAL_OVERFLOW
             ? "BINOMIAL_OVERFLOW -- do not use as a size"
             : "unexpectedly fine");

  /* ---------------------------------------------------------------------
   * STEP 2: enumerate column-sets WITHOUT materialising a table.
   *
   * Use t_wise_visit when C(k,t) rows would be wasteful to store. The order
   * is identical to t_wise().
   * ------------------------------------------------------------------- */
  printf("=== Column-sets via t_wise_visit (no table allocated) ===\n");
  int print_limit = 10;
  int visited = t_wise_visit(k, t, print_combination, &print_limit);
  if (visited > print_limit) {
    printf("    ... %d more\n", visited - print_limit);
  }
  printf("  visited %d column-sets\n\n", visited);

  /* ---------------------------------------------------------------------
   * STEP 3: materialise the table (IToC).
   *
   * This is the table every coverage function wants. Row j holds the t column
   * indices of column-set j, in lexicographic order -- a stable contract, not
   * an implementation detail.
   * ------------------------------------------------------------------- */
  int **IToC = get_matrix(R, (size_t)t);
  if (IToC == NULL) {
    fprintf(stderr, "Error: out of memory\n");
    return 1;
  }
  if (t_wise(IToC, k, t) != 0) {
    fprintf(stderr, "Error: t_wise rejected k=%d t=%d\n", k, t);
    free_matrix(IToC, R);
    return 1;
  }
  printf("=== IToC table built (%zu rows x %d) ===\n", R, t);
  printf("  IToC[j] holds the columns of column-set j;\n");
  printf("  the same j indexes ca->P[j] and ca->tcomb_counter[j].\n\n");

  /* ---------------------------------------------------------------------
   * STEP 4: encode and decode tuples.
   *
   * get_col() reads the t symbols a row places in column-set j and packs them
   * base-v, first column most significant. inv_ruffini() reverses it.
   * ------------------------------------------------------------------- */
  printf("=== Tuple encoding on a sample row ===\n");
  int *row = get_vector((size_t)k);
  if (row == NULL) {
    free_matrix(IToC, R);
    return 1;
  }
  srand(12345);
  printf("  row = [");
  for (int i = 0; i < k; i++) {
    row[i] = rand_below(v);
    printf("%d%s", row[i], i < k - 1 ? " " : "");
  }
  printf("]\n");

  size_t show = R < 6 ? R : 6;
  for (size_t j = 0; j < show; j++) {
    int c = get_col(row, IToC, (int)j, t, v);

    printf("    j=%-3zu {", j);
    for (int i = 0; i < t; i++) {
      printf("%d%s", IToC[j][i], i < t - 1 ? "," : "");
    }
    printf("} -> symbols (");
    for (int i = 0; i < t; i++) {
      printf("%d%s", row[IToC[j][i]], i < t - 1 ? "," : "");
    }
    printf(") -> c=%-4d", c);

    /* Round-trip it back through inv_ruffini to prove they are inverses. */
    int *decoded = get_vector((size_t)t);
    if (decoded != NULL && c >= 0 && inv_ruffini(decoded, c, v, t) == 0) {
      printf("  decodes to (");
      for (int i = 0; i < t; i++) {
        printf("%d%s", decoded[i], i < t - 1 ? "," : "");
      }
      printf(")");
    }
    free_vector(decoded);
    printf("\n");
  }
  if (R > show) {
    printf("    ... %zu more\n", R - show);
  }

  /* The wildcard: symbol value v means "unassigned" and has no encoding. */
  int saved = row[IToC[0][0]];
  row[IToC[0][0]] = v;
  printf("  setting column %d to the wildcard v=%d -> get_col returns %d\n",
         IToC[0][0], v, get_col(row, IToC, 0, t, v));
  row[IToC[0][0]] = saved;
  printf("\n");

  free_vector(row);
  free_matrix(IToC, R);

  /* ---------------------------------------------------------------------
   * STEP 5: the standalone iterators.
   *
   * Neither touches a covering array; both are in-place and driven by a
   * "is there another one?" return value.
   * ------------------------------------------------------------------- */
  printf("=== Permutation iterator ===\n");
  {
    int n = k < 4 ? k : 4;
    int *perm = get_vector((size_t)n);
    for (int i = 0; i < n; i++) {
      perm[i] = i;
    }
    init_permutation(perm, n); /* sorts: the first permutation in lex order */

    int count = 0;
    do {
      if (count < 6) {
        printf("    [");
        for (int i = 0; i < n; i++) {
          printf("%d%s", perm[i], i < n - 1 ? " " : "");
        }
        printf("]\n");
      }
      count++;
    } while (next_permutation(perm, n));
    printf("  %d permutations of %d elements", count, n);
    if (count > 6) {
      printf(" (first 6 shown)");
    }
    printf("\n\n");
    free_vector(perm);
  }

  printf("=== Gray code iterator ===\n");
  {
    int n = k < 3 ? k : 3;
    int *gray = get_vector((size_t)n);

    /* init_gray_code resets the shared walk state; it must start every
       sequence, including one that follows an abandoned walk. */
    init_gray_code(gray, n);

    int count = 0;
    do {
      if (count < 8) {
        printf("    [");
        for (int i = 0; i < n; i++) {
          printf("%d%s", gray[i], i < n - 1 ? " " : "");
        }
        printf("]   (consecutive codes differ in one position by one step)\n");
      }
      count++;
    } while (next_gray_code(gray, n, v));
    int expected = 1;
    for (int i = 0; i < n; i++) {
      expected *= v;
    }
    printf("  %d codes over %d positions, base %d (v^n = %d)%s\n", count, n, v,
           expected, count == expected ? "" : "  <-- MISMATCH");
    free_vector(gray);
  }

  printf("\nDone.\n");
  return 0;
}
