#ifndef LOCAL_CALCULATION_H
#define LOCAL_CALCULATION_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include "covering_array.h"
#include "precompute.h"

ssize_t ca_compute_cell_delta(covering_array_t *ca, const ca_affected_t *pre,
                               int **IToC, int row_idx, int col_idx, int new_val);

ssize_t ca_apply_cell_change(covering_array_t *ca, const ca_affected_t *pre,
                              int **IToC, int row_idx, int col_idx, int new_val);

#endif