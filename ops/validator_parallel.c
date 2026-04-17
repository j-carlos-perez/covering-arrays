#include "../lib/parallel_validator.h"
#include "../lib/covering_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s -f <input_file> [-o <output_folder>] [-s]\n", prog);
    fprintf(stderr, "\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -f, --file <path>      Covering array file to validate (required)\n");
    fprintf(stderr, "  -o, --output <folder>  Save validated array to folder (optional)\n");
    fprintf(stderr, "  -s, --silent           Suppress matrix printing\n");
    fprintf(stderr, "  -h, --help             Show this help message\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s -f ./output_test/N4k3v2^3t2.ca\n", prog);
    fprintf(stderr, "  %s --file test.ca --output ./output_test\n", prog);
}

int main(int argc, char *argv[]) {
    covering_array_t *ca = NULL;
    const char *input_file = NULL;
    const char *output_folder = NULL;
    int show_help = 0;
    int silent = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            show_help = 1;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--silent") == 0) {
            silent = 1;
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

    ca = ca_load(input_file);
    if (ca == NULL) {
        fprintf(stderr, "Failed to load covering array from '%s'\n", input_file);
        return EXIT_FAILURE;
    }

    if (!silent) {
        ca_print(ca);
    }

    pv_validate(ca);

    int valid = (ca->covered == ca->total);
    double coverage_pct = (ca->total > 0) ? (100.0 * (double)ca->covered / (double)ca->total) : 0.0;
    printf("Coverage: %zu / %zu (%.1f%%)\n", ca->covered, ca->total, coverage_pct);
    printf("\nValidation: %s\n", valid ? "PASSED" : "FAILED");

    if (output_folder != NULL) {
        int result = ca_save(output_folder, ca, "Validated by validator_parallel", 0);
        if (result == 0) {
            printf("Saved to %s\n", output_folder);
        } else {
            printf("Failed to save\n");
        }
    }

    ca_destroy(ca);

    return EXIT_SUCCESS;
}
