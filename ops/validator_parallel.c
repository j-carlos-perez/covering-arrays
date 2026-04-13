#include "../lib/parallel_validator.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];

    covering_array_t *ca = ca_load(input_file);
    if (ca == NULL) {
        fprintf(stderr, "Failed to load covering array from '%s'\n", input_file);
        return 1;
    }

    pv_validate(ca);

    printf("Covered: %zu\n", ca->covered);
    printf("Total: %zu\n", ca->total);

    ca_destroy(ca);

    return 0;
}