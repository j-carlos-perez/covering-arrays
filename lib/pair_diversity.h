#ifndef PAIR_DIVERSITY_H
#define PAIR_DIVERSITY_H

#include <stddef.h>

typedef struct pd_score {
  int min_unique_pairs;
  int sum_unique_pairs;
  size_t collision_penalty;
} pd_score_t;

int pd_evaluate_seed(const int *seed, int k, int v, pd_score_t *out_score);
int pd_generate_balanced_seed(int k, int v, int restarts, int iterations,
                              int *out_seed, pd_score_t *out_score);

#endif
