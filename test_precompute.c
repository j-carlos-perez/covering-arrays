#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/precompute.h"
#include "lib/combinatorial.h"
#include "lib/memory.h"

void print_usage(const char *prog)
{
    printf("Usage: %s [k] [t] [options]\n", prog);
    printf("  k: number of columns (default: 7)\n");
    printf("  t: strength (default: 3)\n");
    printf("  --all: print all affected combinations for t-column changes\n");
    printf("  --col: print all affected combinations for single column changes\n");
    printf("\nExamples:\n");
    printf("  %s           # k=7, t=3 (summary only)\n", prog);
    printf("  %s 5 3 --all # k=5, t=3 (show all t-change)\n", prog);
    printf("  %s 7 3 --col # k=7, t=3 (show all single-col)\n", prog);
}

int main(int argc, char *argv[])
{
    size_t k = 7;
    size_t t = 3;
    int show_all = 0;
    int show_col = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--all") == 0) {
            show_all = 1;
        } else if (strcmp(argv[i], "--col") == 0) {
            show_col = 1;
        } else if (i < 3) {
            if (i == 1) k = (size_t)atoi(argv[i]);
            if (i == 2) t = (size_t)atoi(argv[i]);
        }
    }

    printf("=== Precompute Test ===\n");
    printf("k = %zu, t = %zu\n\n", k, t);

    printf("Creating precomputation...\n");
    ca_affected_t *pre = precompute_create(k, t);
    
    if (pre == NULL) {
        printf("FAILED: Could not create precomputation\n");
        return 1;
    }

    printf("SUCCESS!\n\n");

    printf("Stats:\n");
    printf("  change_sets (C(k,t)):     %zu\n", pre->change_sets);
    printf("  affected_per_change:      %zu (for t-column changes)\n", pre->affected_per_change);
    printf("  entries_per_col:          %zu (for single-column changes)\n", pre->entries_per_col);
    printf("  total indices stored:      %zu\n", pre->change_sets * pre->affected_per_change);
    printf("  memory used (indices):     %zu bytes\n", pre->change_sets * pre->affected_per_change * sizeof(uint16_t));
    printf("\n");

    int **IToC = get_matrix((int)pre->change_sets, (int)pre->t);
    t_wise(IToC, (int)pre->k, (int)pre->t);

    if (show_col) {
        printf("=== Single Column Affected Combinations ===\n\n");
        
        for (size_t col = 0; col < pre->k; col++) {
            const uint16_t *affected = precompute_get_col_affected(pre, col);
            size_t count = precompute_get_col_affected_count(pre);
            
            printf("Column %zu affects %zu IToC indices:\n", col, count);
            for (size_t i = 0; i < count; i++) {
                uint16_t itoc_idx = affected[i];
                printf("  IToC[%u] = {", itoc_idx);
                for (size_t j = 0; j < pre->t; j++) {
                    printf("%d%s", IToC[itoc_idx][j], j < pre->t - 1 ? "," : "");
                }
                printf("}\n");
            }
            printf("\n");
        }
    } else if (show_all) {
        printf("=== All Affected Combinations (t-column changes) ===\n\n");
        
        for (size_t change_idx = 0; change_idx < pre->change_sets; change_idx++) {
            printf("Change set %zu: columns {", change_idx);
            for (size_t j = 0; j < pre->t; j++) {
                printf("%d%s", IToC[change_idx][j], j < pre->t - 1 ? "," : "");
            }
            printf("} affects %zu IToC indices:\n", pre->affected_per_change);

            const uint16_t *affected = precompute_get_affected(pre, change_idx);
            size_t count = precompute_get_affected_count(pre);
            
            for (size_t i = 0; i < count; i++) {
                uint16_t itoc_idx = affected[i];
                printf("  IToC[%u] = {", itoc_idx);
                for (size_t j = 0; j < pre->t; j++) {
                    printf("%d%s", IToC[itoc_idx][j], j < pre->t - 1 ? "," : "");
                }
                printf("}\n");
            }
            printf("\n");
        }
    } else {
        printf("=== Summary ===\n\n");
        printf("t-column changes:\n");
        printf("  change_sets: %zu, affected_per_change: %zu\n\n", pre->change_sets, pre->affected_per_change);
        
        printf("single column changes:\n");
        printf("  columns: %zu, affected_per_col: %zu\n\n", pre->k, pre->entries_per_col);
        
        printf("Options:\n");
        printf("  --all  : show all affected for t-column changes\n");
        printf("  --col  : show all affected for single column changes\n");
        printf("\nExample: %s %zu %zu --col\n", argv[0], k, t);
    }

    free_matrix(IToC, (int)pre->change_sets);
    precompute_destroy(pre);
    printf("Done!\n");
    return 0;
}