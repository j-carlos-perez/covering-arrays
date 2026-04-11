#include "lib/combinatorial.h"
#include "lib/covering_array.h"
#include "lib/pair_diversity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s <t> <k> <v> <method> [output_folder]\n", prog);
  fprintf(stderr, "  method: random | position | full\n");
  fprintf(stderr, "  output_folder: optional, defaults to ./output_test\n");
}

static int init_random_optimized(covering_array_t *ca) {
  int *seed = malloc((size_t)ca->k * sizeof(int));
  if (seed == NULL) {
    return -1;
  }

  for (int i = 0; i < ca->N; i++) {
    if (pd_generate_balanced_seed(ca->k, ca->v, 0, 0, seed, NULL) != 0) {
      free(seed);
      return -1;
    }
    for (int j = 0; j < ca->k; j++) {
      ca->matrix[i][j] = seed[j];
    }
  }

  free(seed);
  return 0;
}

static int init_position_optimized(covering_array_t *ca) {
  int *seed;

  if (ca->N != ca->k) {
    fprintf(stderr, "Error: N (%d) must equal k (%d) for position rotation\n",
            ca->N, ca->k);
    return -1;
  }

  seed = malloc((size_t)ca->k * sizeof(int));
  if (seed == NULL) {
    return -1;
  }

  if (pd_generate_balanced_seed(ca->k, ca->v, 0, 0, seed, NULL) != 0) {
    free(seed);
    return -1;
  }

  for (int j = 0; j < ca->k; j++) {
    ca->matrix[0][j] = seed[j];
  }

  for (int i = 1; i < ca->N; i++) {
    for (int j = 0; j < ca->k; j++) {
      ca->matrix[i][j] = ca->matrix[0][(j - i + ca->k) % ca->k];
    }
  }

  free(seed);
  return 0;
}

static int init_full_optimized(covering_array_t *ca) {
  int *seed;

  if (ca->N != ca->k) {
    fprintf(stderr, "Error: N (%d) must equal k (%d) for full rotation\n",
            ca->N, ca->k);
    return -1;
  }

  seed = malloc((size_t)ca->k * sizeof(int));
  if (seed == NULL) {
    return -1;
  }

  if (pd_generate_balanced_seed(ca->k, ca->v, 0, 0, seed, NULL) != 0) {
    free(seed);
    return -1;
  }

  for (int j = 0; j < ca->k; j++) {
    ca->matrix[0][j] = seed[j];
  }

  for (int i = 1; i < ca->N; i++) {
    for (int j = 0; j < ca->k; j++) {
      int shifted = ca->matrix[0][(j - i + ca->k) % ca->k];
      ca->matrix[i][j] = (shifted + i) % ca->v;
    }
  }

  free(seed);
  return 0;
}

int main(int argc, char *argv[]) {
  int t;
  int k;
  int v;
  int N;
  int result;
  int valid;
  int missing;
  size_t min_N = 1;
  const char *method;
  const char *output_folder;
  char comment[96];
  covering_array_t *ca;

  if (argc < 5) {
    print_usage(argv[0]);
    return 1;
  }

  t = atoi(argv[1]);
  k = atoi(argv[2]);
  v = atoi(argv[3]);
  method = argv[4];
  output_folder = (argc > 5) ? argv[5] : "./output_test";

  if (t <= 0 || k <= 0 || v <= 0) {
    fprintf(stderr, "Error: t, k, v must be positive\n");
    return 1;
  }

  for (int i = 0; i < t; i++) {
    min_N *= (size_t)v;
  }

  if (strcmp(method, "position") == 0 || strcmp(method, "full") == 0) {
    N = k;
  } else {
    N = (int)min_N;
  }

  printf("Parameters: t=%d, k=%d, v=%d\n", t, k, v);
  printf("Method: %s\n", method);
  printf("Optimization: first-row pair diversity\n");
  printf("N: %d (v^t = %zu for random)\n", N, min_N);

  srand((unsigned int)time(NULL));

  ca = ca_create(N, k, v, t);
  if (ca == NULL) {
    fprintf(stderr, "Error: failed to create covering array\n");
    return 1;
  }

  if (strcmp(method, "random") == 0) {
    result = init_random_optimized(ca);
  } else if (strcmp(method, "position") == 0) {
    result = init_position_optimized(ca);
  } else if (strcmp(method, "full") == 0) {
    result = init_full_optimized(ca);
  } else {
    fprintf(stderr, "Error: unknown method '%s'\n", method);
    ca_destroy(ca);
    return 1;
  }

  if (result != 0) {
    fprintf(stderr, "Error: optimized initialization failed\n");
    ca_destroy(ca);
    return 1;
  }

  valid = ca_validate(ca);
  printf("Coverage after full validation: %zu / %zu (%.1f%%) - %s\n",
         ca->covered, ca->total,
         100.0 * (double)ca->covered / (double)ca->total,
         valid ? "VALID" : "INVALID");

  missing = (int)(ca->total - ca->covered);
  snprintf(comment, sizeof(comment),
           "creation method: %s, optimized: pair-diversity", method);
  if (ca_save(output_folder, ca, comment, missing) != 0) {
    fprintf(stderr, "Warning: failed to save covering array\n");
  } else {
    printf("Saved to: %s\n", output_folder);
  }

  ca_destroy(ca);
  printf("Done.\n");
  return 0;
}
