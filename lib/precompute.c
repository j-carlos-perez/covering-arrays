#include "precompute.h"
#include "combinatorial.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MEMORY_GB 16
#define MAX_MEMORY_BYTES (MAX_MEMORY_GB * 1024UL * 1024UL * 1024UL)

/*
 * Precomputes t-combination indices affected by column changes.
 * 
 * For each of the R = C(k,t) possible t-column sets, stores which IToC row
 * indices are affected when those columns change values.
 * 
 * Also precomputes per-column mappings: for each column, which IToC rows contain it.
 * This enables efficient incremental coverage updates.
 * 
 * Memory limit is 16 GB.
 * Caller must free with precompute_destroy().
 */
ca_affected_t *precompute_create(size_t k, size_t t) {
  size_t R = binomial(k, t);
  size_t not_affected = binomial(k - t, t);
  size_t affected_per_change = R - not_affected;

  if (R > UINT16_MAX) {
    fprintf(stderr,
            "Error: C(k,t) = %zu exceeds uint16_t max (%u). "
            "k=%zu, t=%zu produces %zu IToC indices.\n",
            R, UINT16_MAX, k, t, R);
    return NULL;
  }

  size_t total_indices = R * affected_per_change;
  size_t memory_needed = total_indices * sizeof(uint16_t);

  if (memory_needed > MAX_MEMORY_BYTES) {
    fprintf(stderr, "Error: memory required (%zu GB) exceeds limit of %d GB\n",
            memory_needed / (1024UL * 1024UL * 1024UL), MAX_MEMORY_GB);
    return NULL;
  }

  size_t itoc_memory = R * t * sizeof(int);
  size_t total_memory = memory_needed + itoc_memory;

  if (total_memory > MAX_MEMORY_BYTES) {
    fprintf(stderr,
            "Error: total memory required (~%.1f GB) exceeds limit of %d GB\n",
            (double)total_memory / (1024.0 * 1024.0 * 1024.0), MAX_MEMORY_GB);
    return NULL;
  }

  ca_affected_t *pre = malloc(sizeof(ca_affected_t));
  if (pre == NULL)
    return NULL;

  pre->k = k;
  pre->t = t;
  pre->change_sets = R;
  pre->affected_per_change = affected_per_change;

  pre->indices = calloc(total_indices, sizeof(uint16_t));
  if (pre->indices == NULL) {
    free(pre);
    return NULL;
  }

  size_t entries_per_col = binomial(k - 1, t - 1);

  if (entries_per_col > UINT16_MAX) {
    fprintf(stderr,
            "Error: C(k-1,t-1) = %zu exceeds uint16_t max (%u). "
            "k=%zu, t=%zu produces %zu entries per column.\n",
            entries_per_col, UINT16_MAX, k, t, entries_per_col);
    free(pre->indices);
    free(pre);
    return NULL;
  }

  size_t col_mapping_memory =
      (k + 1) * sizeof(size_t) + k * entries_per_col * sizeof(uint16_t);
  if (total_memory + col_mapping_memory > MAX_MEMORY_BYTES) {
    fprintf(stderr,
            "Error: total memory required (~%.1f GB) exceeds limit of %d GB\n",
            (double)(total_memory + col_mapping_memory) /
                (1024.0 * 1024.0 * 1024.0),
            MAX_MEMORY_GB);
    free(pre->indices);
    free(pre);
    return NULL;
  }

  int **IToC = get_matrix((int)R, (int)t);
  t_wise(IToC, (int)k, (int)t);

  size_t *col_offsets = malloc((k + 1) * sizeof(size_t));
  uint16_t *col_indices = malloc(k * entries_per_col * sizeof(uint16_t));

  col_offsets[0] = 0;
  for (size_t col = 0; col < k; col++) {
    size_t count = 0;
    for (size_t row = 0; row < R; row++) {
      for (size_t j = 0; j < t; j++) {
        if ((size_t)IToC[row][j] == col) {
          col_indices[col * entries_per_col + count++] = (uint16_t)row;
          break;
        }
      }
    }
    col_offsets[col + 1] = col_offsets[col] + count;
  }

  pre->entries_per_col = entries_per_col;
  pre->col_offsets = col_offsets;
  pre->col_indices = col_indices;

  uint8_t *visited = calloc(R, sizeof(uint8_t));

  for (size_t change_idx = 0; change_idx < R; change_idx++) {
    int *changed_row = IToC[change_idx];

    memset(visited, 0, R * sizeof(uint8_t));

    size_t out_idx = change_idx * affected_per_change;

    for (size_t c = 0; c < t; c++) {
      size_t col = (size_t)changed_row[c];
      size_t start = col_offsets[col];
      size_t end = col_offsets[col + 1];

      for (size_t i = start; i < end; i++) {
        uint16_t itoc_idx = col_indices[i];
        if (!visited[itoc_idx]) {
          visited[itoc_idx] = 1;
          pre->indices[out_idx++] = itoc_idx;
        }
      }
    }
  }

  free(visited);
  free_matrix(IToC, (int)R);

  return pre;
}

/*
 * Frees all memory associated with the precomputation.
 * Handles NULL gracefully.
 */
void precompute_destroy(ca_affected_t *pre) {
  if (pre == NULL)
    return;
  free(pre->indices);
  free(pre->col_indices);
  free(pre->col_offsets);
  free(pre);
}

/*
 * Returns a pointer to the affected IToC indices for a change set.
 * change_set_idx selects which t columns (from IToC matrix).
 * Returns NULL if index is out of bounds.
 */
const uint16_t *precompute_get_affected(const ca_affected_t *pre,
                                        size_t change_set_idx) {
  if (pre == NULL || change_set_idx >= pre->change_sets)
    return NULL;
  return &pre->indices[change_set_idx * pre->affected_per_change];
}

/*
 * Returns the number of affected indices per change set.
 * Equal to C(k,t) - C(k-t,t).
 */
size_t precompute_get_affected_count(const ca_affected_t *pre) {
  return pre ? pre->affected_per_change : 0;
}

/*
 * Returns a pointer to the IToC indices affected by a single column.
 * column is the column index [0, k-1].
 * Returns NULL if column is out of bounds.
 */
const uint16_t *precompute_get_col_affected(const ca_affected_t *pre,
                                            size_t column) {
  if (pre == NULL || column >= pre->k)
    return NULL;
  return &pre->col_indices[pre->col_offsets[column]];
}

/*
 * Returns the number of IToC indices affected per column.
 * Equal to C(k-1, t-1).
 */
size_t precompute_get_col_affected_count(const ca_affected_t *pre) {
  if (pre == NULL)
    return 0;
  return pre->col_offsets[pre->k] /
         pre->k; // or: pre->col_offsets[column+1] - pre->col_offsets[column]
}