#ifndef PAIR_DIVERSITY_H
#define PAIR_DIVERSITY_H

#include <stddef.h>

/*
 * Scoring and search for a single "seed" row with good cyclic pair diversity.
 *
 * The rotation initialisers in covering_array.h build an entire array by
 * rotating one row, so that row's quality decides the whole result. A row
 * rotates well when, for every shift (lag), the pairs it forms with its own
 * shifted copy are as varied as possible -- that is what this scores.
 *
 * For a seed of length k and each lag in [1, k-1], the k pairs
 * (seed[i], seed[(i+lag) % k]) are collected and scored.
 */
typedef struct pd_score {
  /* Across all lags, the fewest distinct pairs any single lag achieved.
     Higher is better. This is the primary criterion: it is the worst case,
     and a rotation is only as good as its weakest lag. Ranges [1, v*v]. */
  int min_unique_pairs;

  /* Distinct pairs summed over every lag. Higher is better. Tie-breaker. */
  int sum_unique_pairs;

  /* Sum of squared pair frequencies over every lag. Lower is better: it is
     minimised when the pairs that do occur are evenly spread rather than
     concentrated. Second tie-breaker. */
  size_t collision_penalty;
} pd_score_t;

/*
 * Scores an existing seed row.
 *
 * Ownership: reads k ints from seed; writes the score into out_score.
 * Returns:   0 on success; -1 if seed or out_score is NULL, k < 2, v < 1, or
 *            any symbol falls outside [0, v-1], or the pair space or score
 *            bounds do not fit their public integer types.
 * Cost:      (k-1) lags x k pairs, plus a v*v sweep per lag.
 */
int pd_evaluate_seed(const int *seed, int k, int v, pd_score_t *out_score);

/*
 * Searches for a good seed row by repeated hill climbing.
 *
 * Each restart begins from a fresh symbol-balanced row and repeatedly swaps
 * two positions, keeping a swap when it improves the score under the ordering
 * above (min_unique_pairs, then sum_unique_pairs, then collision_penalty).
 * Swapping preserves the symbol balance, so every candidate stays balanced.
 * The best row across all restarts is returned.
 *
 * restarts <= 0 selects 64. iterations <= 0 selects max(400, 20*k*k) swaps per
 * restart, or fails if that value exceeds INT_MAX. Note the total work scales
 * steeply with k: each of the
 * restarts * iterations steps rescores the whole seed.
 *
 * Ownership:     writes k ints into out_seed, which the caller provides.
 *                out_score may be NULL if the score is not needed.
 * Returns:       0 on success; -1 if out_seed is NULL, k < 2, v < 1, or
 *                allocation fails, or score/default-iteration arithmetic is
 *                not representable.
 * Thread safety: NOT reentrant -- draws from the global rand() sequence.
 */
int pd_generate_balanced_seed(int k, int v, int restarts, int iterations,
                              int *out_seed, pd_score_t *out_score);

#endif
