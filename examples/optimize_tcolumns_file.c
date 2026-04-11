#include "lib/combinatorial.h"
#include "lib/covering_array.h"
#include "lib/local_calculation.h"
#include "lib/precompute.h"
#include "lib/t_columns_delta.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int find_best_values_for_tcolumns(covering_array_t *ca,
                                         const ca_affected_t *pre, int **IToC,
                                         int row_idx, uint16_t change_set_idx,
                                         int *out_best_values,
                                         ssize_t *out_delta) {
  if (ca == NULL || pre == NULL || IToC == NULL || out_best_values == NULL ||
      out_delta == NULL) {
    return -1;
  }

  int t = (int)pre->t;
  int *changing_cols = IToC[change_set_idx];

  printf("  Change set %u affects columns: {", change_set_idx);
  for (int j = 0; j < t; j++) {
    printf("%d%s", changing_cols[j], j < t - 1 ? "," : "");
  }
  printf("}\n");

  int *current_vals = malloc(t * sizeof(int));
  for (int j = 0; j < t; j++) {
    current_vals[j] = ca->matrix[row_idx][changing_cols[j]];
  }

  printf("  Current values: {");
  for (int j = 0; j < t; j++) {
    printf("%d%s", current_vals[j], j < t - 1 ? "," : "");
  }
  printf("}\n");

  ssize_t best_delta = 0;
  int *best_values = malloc(t * sizeof(int));
  for (int j = 0; j < t; j++) {
    best_values[j] = current_vals[j];
  }

  for (int v0 = 0; v0 < ca->v; v0++) {
    for (int v1 = 0; v1 < ca->v; v1++) {
      if (t == 2) {
        int new_vals[2] = {v0, v1};
        ssize_t delta = ca_compute_tcolumns_delta(ca, pre, IToC, row_idx,
                                                  change_set_idx, new_vals);
        if (delta > best_delta) {
          best_delta = delta;
          best_values[0] = v0;
          best_values[1] = v1;
        }
      }
    }
  }

  if (t == 3) {
    for (int v0 = 0; v0 < ca->v; v0++) {
      for (int v1 = 0; v1 < ca->v; v1++) {
        for (int v2 = 0; v2 < ca->v; v2++) {
          int new_vals[3] = {v0, v1, v2};
          ssize_t delta = ca_compute_tcolumns_delta(ca, pre, IToC, row_idx,
                                                    change_set_idx, new_vals);
          if (delta > best_delta) {
            best_delta = delta;
            best_values[0] = v0;
            best_values[1] = v1;
            best_values[2] = v2;
          }
        }
      }
    }
  }

  printf("  Best values: {");
  for (int j = 0; j < t; j++) {
    printf("%d%s", best_values[j], j < t - 1 ? "," : "");
  }
  printf("} (delta: %zd)\n", best_delta);

  for (int j = 0; j < t; j++) {
    out_best_values[j] = best_values[j];
  }
  *out_delta = best_delta;

  free(current_vals);
  free(best_values);

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input.ca> <output_folder>\n", argv[0]);
    fprintf(stderr, "  Load a covering array from file and find best values "
                    "for t-columns\n");
    return 1;
  }

  const char *filename = argv[1];
  const char *output_folder = argv[2];

  srand((unsigned int)time(NULL));

  printf("Loading covering array from: %s\n", filename);
  covering_array_t *ca = ca_load(filename);
  if (ca == NULL) {
    fprintf(stderr, "Error: failed to load covering array from '%s'\n",
            filename);
    return 1;
  }

  printf("CA(%d; %d, %d, %d)\n", ca->N, ca->t, ca->k, ca->v);

  printf("\nMatrix (%dx%d):\n", ca->N, ca->k);
  for (int i = 0; i < ca->N && i < 5; i++) {
    for (int j = 0; j < ca->k; j++) {
      printf("%d ", ca->matrix[i][j]);
    }
    printf("\n");
  }
  if (ca->N > 5)
    printf("  ... (%d more rows)\n", ca->N - 5);

  printf("\nValidating...\n");
  ca_validate(ca);
  printf("Coverage: %zu / %zu (%.1f%%)\n", ca->covered, ca->total,
         100.0 * (double)ca->covered / (double)ca->total);

  printf("\nPrecomputing affected t-combinations...\n");
  ca_affected_t *pre = precompute_create((size_t)ca->k, (size_t)ca->t);
  if (pre == NULL) {
    fprintf(stderr, "Error: failed to create precompute\n");
    ca_destroy(ca);
    return 1;
  }
  printf("Change sets (C(k,t)): %zu\n", pre->change_sets);
  printf("Affected per change: %zu\n", pre->affected_per_change);

  size_t R = pre->change_sets;
  int **IToC = get_matrix((int)R, ca->t);
  t_wise(IToC, ca->k, ca->t);

  int best_row_idx = 0;
  printf("\n=== Testing all rows and change sets ===\n");
  ssize_t best_overall_delta = 0;
  uint16_t best_change_set_idx = 0;
  int *best_overall_values = malloc(ca->t * sizeof(int));

  for (int r = 0; r < ca->N; r++) {
    for (uint16_t cs = 0; cs < R; cs++) {
      int *test_values = malloc(ca->t * sizeof(int));
      ssize_t test_delta;
      if (find_best_values_for_tcolumns(ca, pre, IToC, r, cs, test_values,
                                        &test_delta) == 0) {
        if (test_delta > best_overall_delta) {
          best_overall_delta = test_delta;
          best_row_idx = r;
          best_change_set_idx = cs;
          for (int j = 0; j < ca->t; j++) {
            best_overall_values[j] = test_values[j];
          }
        }
      }
      free(test_values);
    }
    if (r % 100 == 0)
      printf("  Processed row %d / %d\n", r, ca->N);
  }

  printf("\nBest: row %d, change set %u, delta %zd\n", best_row_idx,
         best_change_set_idx, best_overall_delta);
  printf("Best values: {");
  for (int j = 0; j < ca->t; j++) {
    printf("%d%s", best_overall_values[j], j < ca->t - 1 ? "," : "");
  }
  printf("}\n");

  if (best_overall_delta > 0) {
    printf("\n=== Applying change ===\n");
    ca_apply_tcolumns_change(ca, pre, IToC, best_row_idx, best_change_set_idx,
                             best_overall_values);
    printf("Coverage after change: %zu / %zu (%.1f%%)\n", ca->covered,
           ca->total, 100.0 * (double)ca->covered / (double)ca->total);
  } else {
    printf("\nNo improvement found, skipping change.\n");
  }

  free(best_overall_values);

  printf("\n=== Sanity Check ===\n");
  covering_array_t *ca_verify = ca_create(ca->N, ca->k, ca->v, ca->t);
  if (ca_verify == NULL) {
    fprintf(stderr, "Error: failed to create verification CA\n");
    precompute_destroy(pre);
    ca_destroy(ca);
    free_matrix(IToC, (int)R);
    return 1;
  }

  for (int i = 0; i < ca->N; i++) {
    for (int j = 0; j < ca->k; j++) {
      ca_verify->matrix[i][j] = ca->matrix[i][j];
    }
  }
  ca_validate(ca_verify);

  printf("Incremental covered: %zu\n", ca->covered);
  printf("Full recompute covered: %zu\n", ca_verify->covered);
  printf("Covered difference: %zd\n",
         (ssize_t)ca->covered - (ssize_t)ca_verify->covered);

  size_t C = 1;
  for (int i = 0; i < ca->t; i++)
    C *= (size_t)ca->v;

  printf("P matrix match: ");
  int p_match = 1;
  for (size_t i = 0; i < R && p_match; i++) {
    for (size_t j = 0; j < C && p_match; j++) {
      if (ca->P[i][j] != ca_verify->P[i][j]) {
        p_match = 0;
      }
    }
  }
  printf("%s\n", p_match ? "YES" : "NO");

  printf("\n=== Saving result ===\n");
  int missing = (int)(ca->total - ca->covered);
  if (ca_save(output_folder, ca, "Optimized covering array (t-columns)",
              missing) == 0) {
    printf("Saved to: %s (missing: %d)\n", output_folder, missing);
  } else {
    fprintf(stderr, "Warning: failed to save CA\n");
  }

  ca_destroy(ca_verify);
  free_matrix(IToC, (int)R);
  precompute_destroy(pre);
  ca_destroy(ca);

  printf("\nDone.\n");
  return 0;
}