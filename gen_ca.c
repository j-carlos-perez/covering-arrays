#include "lib/combinatorial.h"
#include "lib/covering_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void print_usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s <t> <k> <v> <method> [output_folder] [--balanced]\n",
          prog);
  fprintf(stderr, "  method: random | position | full\n");
  fprintf(
      stderr,
      "  --balanced: optional, balances symbol frequency in generated rows\n");
  fprintf(stderr, "  output_folder: optional, defaults to ./output_test\n");
  fprintf(stderr, "\nExamples:\n");
  fprintf(stderr, "  %s 2 10 4 random\n", prog);
  fprintf(stderr, "  %s 2 10 4 random --balanced\n", prog);
  fprintf(stderr, "  %s 2 10 4 position ./output_test --balanced\n", prog);
}

int main(int argc, char *argv[]) {
  if (argc < 5) {
    print_usage(argv[0]);
    return 1;
  }

  int t = atoi(argv[1]);
  int k = atoi(argv[2]);
  int v = atoi(argv[3]);
  const char *method = argv[4];
  const char *output_folder = "./output_test";
  int balanced = 0;

  for (int i = 5; i < argc; i++) {
    if (strcmp(argv[i], "--balanced") == 0) {
      balanced = 1;
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
      print_usage(argv[0]);
      return 1;
    } else if (strcmp(output_folder, "./output_test") == 0) {
      output_folder = argv[i];
    } else {
      fprintf(stderr, "Error: too many positional arguments\n");
      print_usage(argv[0]);
      return 1;
    }
  }

  if (t <= 0 || k <= 0 || v <= 0) {
    fprintf(stderr, "Error: t, k, v must be positive\n");
    return 1;
  }

  size_t min_N = 1;
  for (int i = 0; i < t; i++) {
    min_N *= (size_t)v;
  }

  int N;
  if (strcmp(method, "position") == 0 || strcmp(method, "full") == 0) {
    N = k;
  } else {
    N = (int)min_N;
  }
  printf("Parameters: t=%d, k=%d, v=%d\n", t, k, v);
  printf("Method: %s\n", method);
  printf("Balanced: %s\n", balanced ? "yes" : "no");
  printf("N: %d (v^t = %zu for random)\n", N, min_N);

  srand((unsigned int)time(NULL));

  covering_array_t *ca = ca_create(N, k, v, t);
  if (ca == NULL) {
    fprintf(stderr, "Error: failed to create covering array\n");
    return 1;
  }

  int result;
  if (strcmp(method, "random") == 0) {
    result = balanced ? ca_init_random_balanced(ca) : ca_init_random(ca);
  } else if (strcmp(method, "position") == 0) {
    result = balanced ? ca_init_rotation_position_balanced(ca)
                      : ca_init_rotation_position(ca);
  } else if (strcmp(method, "full") == 0) {
    result = balanced ? ca_init_rotation_full_balanced(ca)
                      : ca_init_rotation_full(ca);
  } else {
    fprintf(stderr, "Error: unknown method '%s'\n", method);
    ca_destroy(ca);
    return 1;
  }

  if (result != 0) {
    fprintf(stderr, "Error: initialization failed\n");
    ca_destroy(ca);
    return 1;
  }

  printf("\nInitial coverage (%s): %zu / %zu (%.1f%%)\n", method, ca->covered,
         ca->total, 100.0 * (double)ca->covered / (double)ca->total);

  int valid = ca_validate(ca);
  printf("Coverage after full validation: %zu / %zu (%.1f%%) - %s\n",
         ca->covered, ca->total,
         100.0 * (double)ca->covered / (double)ca->total,
         valid ? "VALID" : "INVALID");

  int missing = (int)(ca->total - ca->covered);
  char comment[96];
  snprintf(comment, sizeof(comment), "creation method: %s, balanced: %s",
           method, balanced ? "yes" : "no");
  if (ca_save(output_folder, ca, comment, missing) != 0) {
    fprintf(stderr, "Warning: failed to save covering array\n");
  } else {
    printf("Saved to: %s\n", output_folder);
  }

  ca_destroy(ca);
  printf("Done.\n");
  return 0;
}
