#include <stdio.h>
#include <stdlib.h>
#include "../covering_array.h"
#include "../utl/combinatorial.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("Covering array validator\n\n");

    covering_array_t *ca = ca_create(4, 3, 2, 2);
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

    printf("CA(%d; %d, %d, %d)\n", ca->N, ca->t, ca->k, ca->v);
    printf("Matrix:\n");
    for (int i = 0; i < ca->N; i++) {
        for (int j = 0; j < ca->k; j++) {
            printf("%d ", ca->matrix[i][j]);
        }
        printf("\n");
    }

    int valid = ca_validate(ca);
    printf("\nValidation: %s\n", valid ? "PASSED" : "FAILED");

    ca_destroy(ca);

    return EXIT_SUCCESS;
}