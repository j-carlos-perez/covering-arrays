#ifndef PRECOMPUTE_H
#define PRECOMPUTE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  size_t k;
  size_t t;
  size_t change_sets;
  size_t affected_per_change;
  uint16_t *indices;

  size_t entries_per_col;
  size_t *col_offsets;
  uint16_t *col_indices;
} ca_affected_t;

ca_affected_t *precompute_create(size_t k, size_t t);
void precompute_destroy(ca_affected_t *pre);
const uint16_t *precompute_get_affected(const ca_affected_t *pre,
                                        size_t change_set_idx);
size_t precompute_get_affected_count(const ca_affected_t *pre);

const uint16_t *precompute_get_col_affected(const ca_affected_t *pre,
                                            size_t column);
size_t precompute_get_col_affected_count(const ca_affected_t *pre);

#endif