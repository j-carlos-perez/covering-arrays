#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/covering_array.h"

int main(int argc, char *argv[])
{
    covering_array_t *ca = NULL;
    char *save_folder = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            save_folder = argv[++i];
        } else if (ca == NULL) {
            ca = ca_load(argv[i]);
            if (ca == NULL) {
                fprintf(stderr, "Failed to load covering array from '%s'\n", argv[i]);
                return EXIT_FAILURE;
            }
        }
    }

    if (ca == NULL) {
        ca = ca_create(4, 3, 2, 2);
        if (ca == NULL) {
            fprintf(stderr, "Failed to create covering array\n");
            return EXIT_FAILURE;
        }

        int matrix[4][3] = {
            {0, 0, 0},
            {0, 1, 1},
            {1, 0, 1},
            {1, 1, 0}
        };

        for (int i = 0; i < ca->N; i++) {
            for (int j = 0; j < ca->k; j++) {
                ca->matrix[i][j] = matrix[i][j];
            }
        }
    }

    ca_print(ca);

    int valid = ca_validate(ca);
    printf("\nValidation: %s\n", valid ? "PASSED" : "FAILED");

    if (save_folder != NULL) {
        int result = ca_save(save_folder, ca, "Test save", 0);
        if (result == 0) {
            printf("Saved to %s\n", save_folder);
        } else {
            printf("Failed to save\n");
        }
    }

    ca_destroy(ca);

    return EXIT_SUCCESS;
}