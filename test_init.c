#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "lib/covering_array.h"

int main() {
    srand(42);
    
    covering_array_t *ca = ca_create(4, 4, 4, 2);
    ca_init_rotation_position(ca);
    printf("Position rotation:\n");
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) printf("%d ", ca->matrix[i][j]);
        printf("\n");
    }
    ca_destroy(ca);
    
    srand(42);
    ca = ca_create(4, 4, 4, 2);
    ca_init_rotation_full(ca);
    printf("\nFull rotation:\n");
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) printf("%d ", ca->matrix[i][j]);
        printf("\n");
    }
    ca_destroy(ca);
    
    return 0;
}