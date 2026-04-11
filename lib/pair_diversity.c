#include <stdlib.h>
#include <string.h>
#include "pair_diversity.h"

static int pd_score_is_better(const pd_score_t *a, const pd_score_t *b)
{
    if (a->min_unique_pairs != b->min_unique_pairs) {
        return a->min_unique_pairs > b->min_unique_pairs;
    }
    if (a->sum_unique_pairs != b->sum_unique_pairs) {
        return a->sum_unique_pairs > b->sum_unique_pairs;
    }
    return a->collision_penalty < b->collision_penalty;
}

static int pd_fill_balanced_random_row(int *row, int k, int v)
{
    int *remaining = malloc((size_t)v * sizeof(int));
    int *symbols = malloc((size_t)v * sizeof(int));
    if (remaining == NULL || symbols == NULL) {
        free(remaining);
        free(symbols);
        return -1;
    }

    int base = k / v;
    int remainder = k % v;
    for (int s = 0; s < v; s++) {
        remaining[s] = base;
        symbols[s] = s;
    }

    for (int i = v - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = symbols[i];
        symbols[i] = symbols[j];
        symbols[j] = tmp;
    }
    for (int i = 0; i < remainder; i++) {
        remaining[symbols[i]]++;
    }

    for (int col = 0; col < k; col++) {
        int slots_left = k - col;
        int pick = rand() % slots_left;
        int cumulative = 0;
        int chosen = -1;

        for (int s = 0; s < v; s++) {
            cumulative += remaining[s];
            if (pick < cumulative) {
                chosen = s;
                break;
            }
        }

        if (chosen < 0) {
            free(remaining);
            free(symbols);
            return -1;
        }

        row[col] = chosen;
        remaining[chosen]--;
    }

    free(remaining);
    free(symbols);
    return 0;
}

int pd_evaluate_seed(const int *seed, int k, int v, pd_score_t *out_score)
{
    if (seed == NULL || out_score == NULL || k < 2 || v < 1) {
        return -1;
    }

    int pair_space = v * v;
    int *freq = malloc((size_t)pair_space * sizeof(int));
    if (freq == NULL) {
        return -1;
    }

    pd_score_t score;
    score.min_unique_pairs = pair_space;
    score.sum_unique_pairs = 0;
    score.collision_penalty = 0;

    for (int lag = 1; lag < k; lag++) {
        memset(freq, 0, (size_t)pair_space * sizeof(int));
        for (int i = 0; i < k; i++) {
            int a = seed[i];
            int b = seed[(i + lag) % k];
            if (a < 0 || a >= v || b < 0 || b >= v) {
                free(freq);
                return -1;
            }
            freq[a * v + b]++;
        }

        int unique = 0;
        size_t lag_penalty = 0;
        for (int p = 0; p < pair_space; p++) {
            if (freq[p] > 0) {
                unique++;
                lag_penalty += (size_t)freq[p] * (size_t)freq[p];
            }
        }

        if (unique < score.min_unique_pairs) {
            score.min_unique_pairs = unique;
        }
        score.sum_unique_pairs += unique;
        score.collision_penalty += lag_penalty;
    }

    free(freq);
    *out_score = score;
    return 0;
}

int pd_generate_balanced_seed(int k, int v, int restarts, int iterations, int *out_seed, pd_score_t *out_score)
{
    if (out_seed == NULL || k < 2 || v < 1) {
        return -1;
    }

    if (restarts <= 0) {
        restarts = 64;
    }
    if (iterations <= 0) {
        iterations = k * k * 20;
        if (iterations < 400) {
            iterations = 400;
        }
    }

    int *candidate = malloc((size_t)k * sizeof(int));
    int *best = malloc((size_t)k * sizeof(int));
    if (candidate == NULL || best == NULL) {
        free(candidate);
        free(best);
        return -1;
    }

    pd_score_t best_score = {0, 0, (size_t)-1};
    int have_best = 0;

    for (int r = 0; r < restarts; r++) {
        if (pd_fill_balanced_random_row(candidate, k, v) != 0) {
            free(candidate);
            free(best);
            return -1;
        }

        pd_score_t current_score;
        if (pd_evaluate_seed(candidate, k, v, &current_score) != 0) {
            free(candidate);
            free(best);
            return -1;
        }

        for (int step = 0; step < iterations; step++) {
            int i = rand() % k;
            int j = rand() % k;
            if (i == j) {
                continue;
            }

            int tmp = candidate[i];
            candidate[i] = candidate[j];
            candidate[j] = tmp;

            pd_score_t trial_score;
            if (pd_evaluate_seed(candidate, k, v, &trial_score) != 0) {
                free(candidate);
                free(best);
                return -1;
            }

            if (pd_score_is_better(&trial_score, &current_score)) {
                current_score = trial_score;
            } else {
                int undo = candidate[i];
                candidate[i] = candidate[j];
                candidate[j] = undo;
            }
        }

        if (!have_best || pd_score_is_better(&current_score, &best_score)) {
            memcpy(best, candidate, (size_t)k * sizeof(int));
            best_score = current_score;
            have_best = 1;
        }
    }

    memcpy(out_seed, best, (size_t)k * sizeof(int));
    if (out_score != NULL) {
        *out_score = best_score;
    }

    free(candidate);
    free(best);
    return 0;
}
