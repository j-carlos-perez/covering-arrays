#include <stdio.h>
#include <stdlib.h>
#include "../utl/memory.h"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("Covering array validator\n\n");

    int rows = 3;
    int cols = 4;

    int *v = get_vector(rows);
    for (int i = 0; i < rows; i++) {
        v[i] = i;
    }

    int **m = get_matrix(rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            m[i][j] = i * cols + j;
        }
    }

    printf("Vector: ");
    for (int i = 0; i < rows; i++) {
        printf("%d ", v[i]);
    }

    printf("\nMatrix:\n");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", m[i][j]);
        }
        printf("\n");
    }

    free_vector(v);
    free_matrix(m, rows);

    return EXIT_SUCCESS;
}
