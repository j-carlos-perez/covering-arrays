#ifndef T_COLUMNS_DELTA_H
#define T_COLUMNS_DELTA_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include "covering_array.h"
#include "precompute.h"

ssize_t ca_compute_tcolumns_delta(covering_array_t *ca, const ca_affected_t *pre,
                                   int **IToC, int row_idx, 
                                   uint16_t change_set_idx, const int *new_vals);

ssize_t ca_apply_tcolumns_change(covering_array_t *ca, const ca_affected_t *pre,
                                  int **IToC, int row_idx,
                                  uint16_t change_set_idx, const int *new_vals);

#endif