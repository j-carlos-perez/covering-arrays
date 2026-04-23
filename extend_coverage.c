#include "lib/combinatorial.h"
#include "lib/covering_array.h"
#include "lib/parallel_validator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s -f <input_file> -o <output_folder>\n", prog);
  fprintf(stderr, "\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -f, --file <path>    Covering array file to extend (required)\n");
  fprintf(stderr, "  -o, --output <folder> Output folder for extended array (required)\n");
  fprintf(stderr, "  -h, --help           Show this help message\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Examples:\n");
  fprintf(stderr, "  %s -f ./input.ca -o ./output\n", prog);
  fprintf(stderr, "  %s --file test.ca --output ./output\n", prog);
}

int main(int argc, char *argv[]) {
  const char *input_file = NULL;
  const char *output_folder = NULL;
  int show_help = 0;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      show_help = 1;
    } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: missing value for %s\n\n", argv[i]);
        print_usage(argv[0]);
        return EXIT_FAILURE;
      }
      input_file = argv[++i];
    } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: missing value for %s\n\n", argv[i]);
        print_usage(argv[0]);
        return EXIT_FAILURE;
      }
      output_folder = argv[++i];
    } else {
      fprintf(stderr, "Error: unknown option '%s'\n\n", argv[i]);
      print_usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (show_help) {
    print_usage(argv[0]);
    return EXIT_SUCCESS;
  }

  if (input_file == NULL) {
    fprintf(stderr, "Error: input file is required\n\n");
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (output_folder == NULL) {
    fprintf(stderr, "Error: output folder is required\n\n");
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  srand((unsigned int)time(NULL));

  printf("Loading covering array from: %s\n", input_file);
  covering_array_t *ca = ca_load(input_file);
  if (ca == NULL) {
    fprintf(stderr, "Error: failed to load covering array from '%s'\n", input_file);
    return EXIT_FAILURE;
  }

  printf("Loaded: N=%d, k=%d, v=%d, t=%d\n", ca->N, ca->k, ca->v, ca->t);

  printf("Validating covering array in parallel...\n");
  pv_validate(ca);
  printf("Coverage before: %zu / %zu (%.1f%%)\n",
         ca->covered, ca->total,
         ca->total > 0 ? 100.0 * ca->covered / ca->total : 0.0);

  int *new_row = malloc(ca->k * sizeof(int));
  if (new_row == NULL) {
    fprintf(stderr, "Error: failed to allocate new row\n");
    ca_destroy(ca);
    return EXIT_FAILURE;
  }

  for (int j = 0; j < ca->k; j++) {
    new_row[j] = rand() % ca->v;
  }

  printf("Adding new random row: ");
  for (int j = 0; j < ca->k; j++) {
    printf("%d ", new_row[j]);
  }
  printf("\n");

  if (ca_add_row(ca, new_row) != 0) {
    fprintf(stderr, "Error: failed to add new row\n");
    free(new_row);
    ca_destroy(ca);
    return EXIT_FAILURE;
  }
  free(new_row);

  printf("New row count: %d\n", ca->N);

  printf("Recalculating coverage...\n");
  pv_validate(ca);
  printf("Coverage after: %zu / %zu (%.1f%%)\n",
         ca->covered, ca->total,
         ca->total > 0 ? 100.0 * ca->covered / ca->total : 0.0);

  int missing = (ca->covered < ca->total) ? 1 : 0;
  printf("Saving to: %s\n", output_folder);
  if (ca_save(output_folder, ca, "Extended by adding random row", missing) != 0) {
    fprintf(stderr, "Error: failed to save covering array\n");
    ca_destroy(ca);
    return EXIT_FAILURE;
  }

  printf("Done.\n");
  ca_destroy(ca);
  return EXIT_SUCCESS;
}