/*
 * Regression tests for the bug audit.
 *
 * Every test in this file was written to fail against the code as it stood
 * before the audit, and to pass afterwards. Each test names the audit item it
 * pins down so a future failure points straight at the defect it guards.
 */

#include "../lib/combinatorial.h"
#include "../lib/covering_array.h"
#include "../lib/local_calculation.h"
#include "../lib/memory.h"
#include "../lib/pair_diversity.h"
#include "../lib/parallel_validator.h"
#include "../lib/precompute.h"
#include "../lib/set64.h"
#include "../lib/t_columns_delta.h"
#include "unity.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* ---------------------------------------------------------------- helpers */

static covering_array_t *make_random_ca(int N, int k, int v, int t,
                                        unsigned seed) {
  covering_array_t *ca = ca_create(N, k, v, t);
  TEST_ASSERT_NOT_NULL(ca);
  srand(seed);
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < k; j++) {
      ca->matrix[i][j] = rand() % v;
    }
  }
  return ca;
}

/* Recompute P/covered from scratch on a copy of ca's matrix. */
static covering_array_t *reference_validate(const covering_array_t *ca) {
  covering_array_t *ref = ca_create(ca->N, ca->k, ca->v, ca->t);
  TEST_ASSERT_NOT_NULL(ref);
  for (int i = 0; i < ca->N; i++) {
    memcpy(ref->matrix[i], ca->matrix[i], (size_t)ca->k * sizeof(int));
  }
  ca_validate(ref);
  return ref;
}

static void assert_coverage_matches_reference(covering_array_t *ca) {
  covering_array_t *ref = reference_validate(ca);
  size_t R = (size_t)binomial(ca->k, ca->t);
  size_t C = 1;
  for (int i = 0; i < ca->t; i++) {
    C *= (size_t)ca->v;
  }

  TEST_ASSERT_EQUAL_size_t(ref->covered, ca->covered);
  TEST_ASSERT_EQUAL_size_t(ref->total, ca->total);
  for (size_t j = 0; j < R; j++) {
    TEST_ASSERT_EQUAL_size_t(ref->tcomb_counter[j], ca->tcomb_counter[j]);
    for (size_t c = 0; c < C; c++) {
      TEST_ASSERT_EQUAL_UINT(ref->P[j][c], ca->P[j][c]);
    }
  }
  ca_destroy(ref);
}

/* ------------------------------------------------- 1.3  t_wise with t > k */

static void test_binomial_rejects_r_greater_than_k(void) {
  TEST_ASSERT_EQUAL_UINT64(0, binomial(3, 5));
  TEST_ASSERT_EQUAL_UINT64(0, binomial(0, 1));
  TEST_ASSERT_EQUAL_UINT64(0, binomial(-1, 2));
  TEST_ASSERT_EQUAL_UINT64(0, binomial(5, -1));
  TEST_ASSERT_EQUAL_UINT64(1, binomial(5, 0));
  TEST_ASSERT_EQUAL_UINT64(10, binomial(5, 2));
}

static void test_binomial_is_exact_near_the_uint64_ceiling(void) {
  /* These fit in a uint64_t but their naive intermediate b*(k-i+1) does not,
     so a running product that is not reduced first reports a false overflow
     (or, in the original int version, silently went negative). */
  TEST_ASSERT_EQUAL_UINT64(759510004936100355ULL, binomial(63, 29));
  TEST_ASSERT_EQUAL_UINT64(916312070471295267ULL, binomial(63, 31));
  TEST_ASSERT_EQUAL_UINT64(1832624140942590534ULL, binomial(64, 32));
  TEST_ASSERT_EQUAL_UINT64(155117520ULL, binomial(30, 15));
  TEST_ASSERT_EQUAL_UINT64(1ULL, binomial(64, 64));

  /* Genuine overflow must be reported, never wrapped. */
  TEST_ASSERT_EQUAL_UINT64(BINOMIAL_OVERFLOW, binomial(70, 35));
  TEST_ASSERT_FALSE(binomial_is_usable(binomial(70, 35)));
  TEST_ASSERT_TRUE(binomial_is_usable(binomial(63, 29)));
}

static void test_t_wise_refuses_t_greater_than_k(void) {
  /* Before the fix this wrote through GTP[0] of a zero-row matrix. */
  int **GTP = get_matrix(1, 5);
  TEST_ASSERT_NOT_NULL(GTP);
  TEST_ASSERT_EQUAL_INT(-1, t_wise(GTP, 3, 5));
  TEST_ASSERT_EQUAL_INT(0, t_wise_visit(3, 5, NULL, NULL));
  free_matrix(GTP, 1);
}

static void test_ca_create_rejects_t_greater_than_k(void) {
  TEST_ASSERT_NULL(ca_create(8, 3, 2, 5));
  TEST_ASSERT_NULL(ca_create(8, 3, 2, 0));
  TEST_ASSERT_NULL(ca_create(0, 3, 2, 2));
  TEST_ASSERT_NULL(ca_create(8, 3, 1, 2));
}

static void test_allocation_helpers_reject_size_overflow(void) {
  TEST_ASSERT_NULL(get_vector(SIZE_MAX / sizeof(int) + 1));
  TEST_ASSERT_NULL(get_vector_size_t(SIZE_MAX / sizeof(size_t) + 1));
  TEST_ASSERT_NULL(get_matrix(SIZE_MAX / sizeof(int *) + 1, 1));
  TEST_ASSERT_NULL(
      get_matrix_count_calloc(SIZE_MAX / sizeof(ca_count_t *) + 1, 1));
}

static void test_int_indexed_spaces_reject_overflow(void) {
  TEST_ASSERT_FALSE(ca_params_valid(1, 2, 50000, 2));
  TEST_ASSERT_NULL(ca_create(1, 2, 50000, 2));

  int row[2] = {49999, 49999};
  int cols_data[2] = {0, 1};
  int *cols[1] = {cols_data};
  TEST_ASSERT_EQUAL_INT(-1, get_col(row, cols, 0, 2, 50000));

  /* C(35,17) exceeds INT_MAX, the public callback/count index type. */
  TEST_ASSERT_EQUAL_INT(0, t_wise_visit(35, 17, NULL, NULL));
  int count = 123;
  TEST_ASSERT_NULL(generate_t_combinations(35, 17, &count));
  TEST_ASSERT_EQUAL_INT(0, count);
}

/* ------------------------------------------- 1.6  inv_ruffini range guard */

static void test_inv_ruffini_rejects_out_of_range(void) {
  int V[3] = {9, 9, 9};
  /* v^t = 27, so 27 does not fit in 3 base-3 digits. */
  TEST_ASSERT_EQUAL_INT(-1, inv_ruffini(V, 27, 3, 3));
  TEST_ASSERT_EQUAL_INT(-1, inv_ruffini(V, -1, 3, 3));
  TEST_ASSERT_EQUAL_INT(0, inv_ruffini(V, 26, 3, 3));
  TEST_ASSERT_EQUAL_INT(2, V[0]);
  TEST_ASSERT_EQUAL_INT(2, V[1]);
  TEST_ASSERT_EQUAL_INT(2, V[2]);
}

/* ------------------------------- 2.1  apply must apply even when delta==0 */

static void test_apply_cell_change_applies_when_delta_is_zero(void) {
  covering_array_t *ca = make_random_ca(8, 5, 2, 2, 12345);
  ca_validate(ca);

  ca_affected_t *pre = precompute_create(5, 2);
  TEST_ASSERT_NOT_NULL(pre);
  int R = (int)binomial(5, 2);
  int **IToC = get_matrix(R, 2);
  t_wise(IToC, 5, 2);

  int found = 0;
  for (int r = 0; r < ca->N && !found; r++) {
    for (int c = 0; c < ca->k && !found; c++) {
      int old_val = ca->matrix[r][c];
      for (int nv = 0; nv < ca->v; nv++) {
        if (nv == old_val) {
          continue;
        }
        if (ca_compute_cell_delta(ca, pre, IToC, r, c, nv) == 0) {
          ca_apply_cell_change(ca, pre, IToC, r, c, nv);
          /* The cell must actually hold the new value... */
          TEST_ASSERT_EQUAL_INT(nv, ca->matrix[r][c]);
          /* ...and P/covered/tcomb_counter must describe the new matrix. */
          assert_coverage_matches_reference(ca);
          found = 1;
          break;
        }
      }
    }
  }
  TEST_ASSERT_TRUE_MESSAGE(found, "no delta==0 cell change found to exercise");

  free_matrix(IToC, R);
  precompute_destroy(pre);
  ca_destroy(ca);
}

static void test_apply_cell_change_keeps_p_in_sync_over_many_moves(void) {
  covering_array_t *ca = make_random_ca(10, 6, 3, 2, 999);
  ca_validate(ca);

  ca_affected_t *pre = precompute_create(6, 2);
  TEST_ASSERT_NOT_NULL(pre);
  int R = (int)binomial(6, 2);
  int **IToC = get_matrix(R, 2);
  t_wise(IToC, 6, 2);

  srand(4242);
  for (int step = 0; step < 60; step++) {
    int r = rand() % ca->N;
    int c = rand() % ca->k;
    int nv = rand() % ca->v;
    ca_apply_cell_change(ca, pre, IToC, r, c, nv);
    TEST_ASSERT_EQUAL_INT(nv, ca->matrix[r][c]);
  }
  assert_coverage_matches_reference(ca);

  free_matrix(IToC, R);
  precompute_destroy(pre);
  ca_destroy(ca);
}

static void test_delta_functions_reject_invalid_arguments_without_mutating(void) {
  covering_array_t *ca = make_random_ca(8, 6, 3, 2, 31415);
  ca_validate(ca);
  ca_affected_t *pre = precompute_create(6, 2);
  ca_affected_t *wrong_pre = precompute_create(5, 2);
  size_t R = (size_t)binomial(6, 2);
  int **IToC = get_matrix(R, 2);
  t_wise(IToC, 6, 2);
  int original = ca->matrix[0][0];
  size_t covered = ca->covered;

  TEST_ASSERT_EQUAL_INT(0,
                        ca_apply_cell_change(ca, pre, IToC, -1, 0, 1));
  TEST_ASSERT_EQUAL_INT(0,
                        ca_apply_cell_change(ca, pre, IToC, 0, 6, 1));
  TEST_ASSERT_EQUAL_INT(0,
                        ca_apply_cell_change(ca, pre, IToC, 0, 0, 4));
  TEST_ASSERT_EQUAL_INT(
      0, ca_apply_cell_change(ca, wrong_pre, IToC, 0, 0, 1));
  TEST_ASSERT_EQUAL_INT(original, ca->matrix[0][0]);
  TEST_ASSERT_EQUAL_size_t(covered, ca->covered);

  int bad_vals[2] = {-1, 0};
  TEST_ASSERT_EQUAL_INT(
      0, ca_apply_tcolumns_change(ca, pre, IToC, 0, 0, bad_vals));
  TEST_ASSERT_EQUAL_INT(original, ca->matrix[0][0]);
  TEST_ASSERT_EQUAL_size_t(covered, ca->covered);

  precompute_destroy(wrong_pre);
  precompute_destroy(pre);
  free_matrix(IToC, R);
  ca_destroy(ca);
}

static void test_apply_tcolumns_change_applies_when_delta_is_zero(void) {
  covering_array_t *ca = make_random_ca(10, 6, 2, 2, 555);
  ca_validate(ca);

  ca_affected_t *pre = precompute_create(6, 2);
  TEST_ASSERT_NOT_NULL(pre);
  int R = (int)binomial(6, 2);
  int **IToC = get_matrix(R, 2);
  t_wise(IToC, 6, 2);

  int found = 0;
  for (int r = 0; r < ca->N && !found; r++) {
    for (int cs = 0; cs < R && !found; cs++) {
      for (int a = 0; a < ca->v && !found; a++) {
        for (int b = 0; b < ca->v; b++) {
          int nv[2] = {a, b};
          int c0 = IToC[cs][0], c1 = IToC[cs][1];
          if (ca->matrix[r][c0] == a && ca->matrix[r][c1] == b) {
            continue;
          }
          if (ca_compute_tcolumns_delta(ca, pre, IToC, r, (uint16_t)cs, nv) == 0) {
            ca_apply_tcolumns_change(ca, pre, IToC, r, (uint16_t)cs, nv);
            TEST_ASSERT_EQUAL_INT(a, ca->matrix[r][c0]);
            TEST_ASSERT_EQUAL_INT(b, ca->matrix[r][c1]);
            assert_coverage_matches_reference(ca);
            found = 1;
            break;
          }
        }
      }
    }
  }
  TEST_ASSERT_TRUE_MESSAGE(found, "no delta==0 t-column change found");

  free_matrix(IToC, R);
  precompute_destroy(pre);
  ca_destroy(ca);
}

static void test_apply_tcolumns_change_keeps_state_in_sync_over_many_moves(void) {
  covering_array_t *ca = make_random_ca(10, 7, 3, 3, 2718);
  ca_validate(ca);
  ca_affected_t *pre = precompute_create(7, 3);
  TEST_ASSERT_NOT_NULL(pre);
  size_t R = (size_t)binomial(7, 3);
  int **IToC = get_matrix(R, 3);
  t_wise(IToC, 7, 3);

  srand(1618);
  for (int move = 0; move < 100; move++) {
    int values[3] = {rand() % 3, rand() % 3, rand() % 3};
    ca_apply_tcolumns_change(ca, pre, IToC, rand() % ca->N,
                             (uint16_t)(rand() % R), values);
    assert_coverage_matches_reference(ca);
  }

  free_matrix(IToC, R);
  precompute_destroy(pre);
  ca_destroy(ca);
}

/* ----------------------------------- 2.2  coverage counters must not wrap */

static void test_coverage_counters_survive_more_than_255_rows(void) {
  /* 256 identical rows: every column pair covers exactly tuple (0,0), and it
     is covered by all 256 rows. A uint8_t counter wraps to 0 and the array is
     then reported as covering nothing at all. */
  int N = 256, k = 4, v = 2, t = 2;
  covering_array_t *ca = ca_create(N, k, v, t);
  TEST_ASSERT_NOT_NULL(ca);
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < k; j++) {
      ca->matrix[i][j] = 0;
    }
  }
  ca_validate(ca);

  size_t R = (size_t)binomial(k, t);
  TEST_ASSERT_EQUAL_size_t(R, ca->covered);
  TEST_ASSERT_EQUAL_size_t(R * 4, ca->total);
  TEST_ASSERT_EQUAL_UINT(256, ca->P[0][0]);
  for (size_t j = 0; j < R; j++) {
    TEST_ASSERT_EQUAL_size_t(3, ca->tcomb_counter[j]);
  }
  ca_destroy(ca);
}

/* ------------------------------------------- 2.3  pv_validate idempotence */

static void test_pv_validate_is_idempotent(void) {
  covering_array_t *ca = make_random_ca(12, 6, 2, 2, 7);

  pv_validate(ca);
  size_t R = (size_t)binomial(ca->k, ca->t);
  size_t cov1 = ca->covered;
  size_t *tc1 = malloc(R * sizeof(size_t));
  TEST_ASSERT_NOT_NULL(tc1);
  memcpy(tc1, ca->tcomb_counter, R * sizeof(size_t));
  unsigned p1 = ca->P[0][0];

  pv_validate(ca);

  TEST_ASSERT_EQUAL_size_t(cov1, ca->covered);
  TEST_ASSERT_EQUAL_UINT(p1, ca->P[0][0]);
  for (size_t j = 0; j < R; j++) {
    TEST_ASSERT_EQUAL_size_t(tc1[j], ca->tcomb_counter[j]);
  }

  free(tc1);
  ca_destroy(ca);
}

static void test_pv_validate_agrees_with_ca_validate(void) {
  for (int t = 2; t <= 3; t++) {
    for (int v = 2; v <= 4; v++) {
      for (int k = 5; k <= 9; k++) {
        covering_array_t *par = make_random_ca(14, k, v, t, (unsigned)(k * 31 + v * 7 + t));
        pv_validate(par);

        covering_array_t *seq = reference_validate(par);
        size_t R = (size_t)binomial(k, t);
        size_t C = 1;
        for (int i = 0; i < t; i++) {
          C *= (size_t)v;
        }

        TEST_ASSERT_EQUAL_size_t(seq->covered, par->covered);
        TEST_ASSERT_EQUAL_size_t(seq->total, par->total);
        for (size_t j = 0; j < R; j++) {
          TEST_ASSERT_EQUAL_size_t(seq->tcomb_counter[j], par->tcomb_counter[j]);
          for (size_t c = 0; c < C; c++) {
            TEST_ASSERT_EQUAL_UINT(seq->P[j][c], par->P[j][c]);
          }
        }
        ca_destroy(seq);
        ca_destroy(par);
      }
    }
  }
}

static void test_failed_parallel_validation_invalidates_summary(void) {
  covering_array_t *ca = make_random_ca(8, 5, 2, 2, 99);
  pv_validate(ca);
  TEST_ASSERT_TRUE(ca->total > 0);
  int **matrix = ca->matrix;
  ca->matrix = NULL;
  pv_validate(ca);
  TEST_ASSERT_EQUAL_size_t(0, ca->covered);
  TEST_ASSERT_EQUAL_size_t(0, ca->total);
  ca->matrix = matrix;

  ca_validate(ca);
  TEST_ASSERT_TRUE(ca->total > 0);
  ca->matrix = NULL;
  TEST_ASSERT_EQUAL_INT(0, ca_validate(ca));
  TEST_ASSERT_EQUAL_size_t(0, ca->covered);
  TEST_ASSERT_EQUAL_size_t(0, ca->total);
  ca->matrix = matrix;
  ca_destroy(ca);
}

/* ------------------------------------------------------- 2.6  gray codes */

static void test_gray_code_sequences_do_not_leak_state(void) {
  int arr5[5], arr3[3];
  int count;

  /* Run a length-5 sequence to exhaustion. */
  init_gray_code(arr5, 5);
  count = 0;
  do {
    count++;
  } while (next_gray_code(arr5, 5, 2));
  TEST_ASSERT_EQUAL_INT(32, count);

  /* A shorter sequence afterwards must start from a clean slate. */
  init_gray_code(arr3, 3);
  count = 0;
  do {
    count++;
  } while (next_gray_code(arr3, 3, 3));
  TEST_ASSERT_EQUAL_INT(27, count);

  /* Abandoning a sequence part-way must not poison the next one. Ten steps is
     past the point where a direction flip reaches index < 3, which is what
     made the stale directions observable in the shorter walk that follows;
     stopping after only one or two steps leaves them all still +1 and hides
     the defect. */
  init_gray_code(arr5, 5);
  for (int i = 0; i < 10; i++) {
    next_gray_code(arr5, 5, 2);
  }

  init_gray_code(arr3, 3);
  count = 0;
  do {
    count++;
  } while (next_gray_code(arr3, 3, 2));
  TEST_ASSERT_EQUAL_INT(8, count);
}

static void test_gray_code_stays_exhausted_until_reinitialized(void) {
  int arr[2];
  init_gray_code(arr, 2);
  while (next_gray_code(arr, 2, 2)) {
  }
  TEST_ASSERT_EQUAL_INT(0, next_gray_code(arr, 2, 2));
  TEST_ASSERT_EQUAL_INT(0, next_gray_code(arr, 2, 2));
  init_gray_code(arr, 2);
  TEST_ASSERT_EQUAL_INT(1, next_gray_code(arr, 2, 2));
}

static void test_pair_diversity_rejects_arithmetic_overflow(void) {
  int seed[2] = {0, 0};
  pd_score_t score;
  TEST_ASSERT_EQUAL_INT(-1, pd_evaluate_seed(seed, 2, 50000, &score));

  int *large_seed = get_vector(20000);
  TEST_ASSERT_NOT_NULL(large_seed);
  TEST_ASSERT_EQUAL_INT(
      -1, pd_generate_balanced_seed(20000, 1, 1, 0, large_seed, NULL));
  free_vector(large_seed);
}

static void test_ca_add_row_uses_geometric_growth_and_validates_input(void) {
  covering_array_t *ca = ca_create(1, 2, 2, 1);
  TEST_ASSERT_NOT_NULL(ca);
  int row[2] = {0, 1};
  for (int i = 0; i < 100; i++) {
    TEST_ASSERT_EQUAL_INT(0, ca_add_row(ca, row));
  }
  TEST_ASSERT_EQUAL_INT(101, ca->N);
  TEST_ASSERT_EQUAL_INT(128, ca->capacity);
  TEST_ASSERT_EQUAL_INT(-1, ca_add_row(ca, NULL));
  int bad_row[2] = {0, 2};
  TEST_ASSERT_EQUAL_INT(-1, ca_add_row(ca, bad_row));
  ca_destroy(ca);
}

static void test_ca_add_row_coverage_refuses_counter_overflow(void) {
  covering_array_t *ca = ca_create(1, 1, 2, 1);
  TEST_ASSERT_NOT_NULL(ca);
  ca->matrix[0][0] = 0;
  ca_validate(ca);
  ca->P[0][0] = CA_COUNT_MAX;
  int row[1] = {0};
  TEST_ASSERT_EQUAL_INT(-1, ca_add_row_coverage(ca, row));
  TEST_ASSERT_EQUAL_UINT(CA_COUNT_MAX, ca->P[0][0]);
  ca_destroy(ca);
}

/* ------------------------------------------------------ 2.8  shuffle bias */

static void test_shuffle_is_unbiased(void) {
  /* Every one of the 4! orderings of [0,3] must show up at a plausible rate.
     The biased n^n version cannot reach a uniform distribution. */
  int counts[24];
  int arr[4];
  const int trials = 240000;
  const int expected = trials / 24;

  memset(counts, 0, sizeof(counts));
  srand(20240521);
  for (int i = 0; i < trials; i++) {
    shuffle(arr, 4);
    /* Lehmer code -> index in [0,24) */
    int idx = 0, factorial = 6;
    for (int a = 0; a < 3; a++) {
      int smaller = 0;
      for (int b = a + 1; b < 4; b++) {
        if (arr[b] < arr[a]) {
          smaller++;
        }
      }
      idx += smaller * factorial;
      factorial /= (3 - a);
    }
    TEST_ASSERT_TRUE(idx >= 0 && idx < 24);
    counts[idx]++;
  }

  for (int i = 0; i < 24; i++) {
    /* +-15% of the expected share; the biased shuffle misses by far more. */
    TEST_ASSERT_TRUE_MESSAGE(counts[i] > expected * 85 / 100 &&
                                 counts[i] < expected * 115 / 100,
                             "shuffle is not uniform");
  }
}

/* --------------------------------------------------------- 1.1/1.2/2.7 set64 */

static void test_set64_grows_dense_array(void) {
  Set64 *s = set64_create(4);
  TEST_ASSERT_NOT_NULL(s);
  const uint32_t n = 4000;
  for (uint32_t i = 0; i < n; i++) {
    set64_insert(s, set64_make_key(i % 32768, i / 32768));
  }
  TEST_ASSERT_EQUAL_UINT32(n, s->size);
  TEST_ASSERT_TRUE_MESSAGE(s->size <= s->cap,
                           "dense[] smaller than size: heap overflow");
  set64_free(s);
}

static void test_set64_insert_ignores_duplicates(void) {
  Set64 *s = set64_create(16);
  TEST_ASSERT_NOT_NULL(s);
  for (int rep = 0; rep < 5; rep++) {
    for (uint32_t i = 0; i < 40; i++) {
      set64_insert(s, set64_make_key(i, 3));
    }
  }
  TEST_ASSERT_EQUAL_UINT32(40, s->size);
  set64_free(s);
}

static void test_set64_delete_finds_every_present_key(void) {
  Set64 *s = set64_create(8);
  TEST_ASSERT_NOT_NULL(s);
  const uint32_t n = 500;
  for (uint32_t i = 0; i < n; i++) {
    set64_insert(s, set64_make_key(i, 0));
  }
  /* Interleave deletes and inserts so tombstones accumulate. */
  for (uint32_t i = 0; i < n; i += 2) {
    TEST_ASSERT_TRUE_MESSAGE(set64_delete(s, set64_make_key(i, 0)),
                             "present key reported missing");
  }
  for (uint32_t i = 0; i < n; i += 2) {
    set64_insert(s, set64_make_key(i, 1));
  }
  for (uint32_t i = 1; i < n; i += 2) {
    TEST_ASSERT_TRUE_MESSAGE(set64_delete(s, set64_make_key(i, 0)),
                             "present key reported missing after reinsert");
  }
  TEST_ASSERT_EQUAL_UINT32(n / 2, s->size);
  set64_free(s);
}

static void test_set64_random_reaches_every_element(void) {
  Set64 *s = set64_create(8);
  TEST_ASSERT_NOT_NULL(s);
  /* 6 is not a power of two: the masking version can only ever return 4 of
     these, no matter how many draws. */
  for (uint32_t i = 0; i < 6; i++) {
    set64_insert(s, set64_make_key(i + 1, 0));
  }
  int seen[6] = {0};
  for (int i = 0; i < 20000; i++) {
    uint64_t key = set64_random(s);
    TEST_ASSERT_NOT_EQUAL(SET64_EMPTY_KEY, key);
    uint32_t a = set64_key_get_a(key);
    TEST_ASSERT_TRUE(a >= 1 && a <= 6);
    seen[a - 1] = 1;
  }
  for (int i = 0; i < 6; i++) {
    TEST_ASSERT_TRUE_MESSAGE(seen[i], "set64_random cannot reach every element");
  }
  set64_free(s);
}

static void test_set64_make_key_roundtrips(void) {
  uint64_t k = set64_make_key(32767, 12345);
  TEST_ASSERT_EQUAL_UINT32(32767, set64_key_get_a(k));
  TEST_ASSERT_EQUAL_UINT32(12345, set64_key_get_b(k));
  TEST_ASSERT_TRUE(k > SET64_DELETED_KEY);
}

static void test_set64_rejects_unrepresentable_capacity(void) {
  TEST_ASSERT_NULL(set64_create(UINT32_MAX));
  TEST_ASSERT_NULL(set64_create(UINT32_MAX / 2 + 1));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_binomial_rejects_r_greater_than_k);
  RUN_TEST(test_binomial_is_exact_near_the_uint64_ceiling);
  RUN_TEST(test_t_wise_refuses_t_greater_than_k);
  RUN_TEST(test_ca_create_rejects_t_greater_than_k);
  RUN_TEST(test_allocation_helpers_reject_size_overflow);
  RUN_TEST(test_int_indexed_spaces_reject_overflow);
  RUN_TEST(test_inv_ruffini_rejects_out_of_range);
  RUN_TEST(test_apply_cell_change_applies_when_delta_is_zero);
  RUN_TEST(test_apply_cell_change_keeps_p_in_sync_over_many_moves);
  RUN_TEST(test_delta_functions_reject_invalid_arguments_without_mutating);
  RUN_TEST(test_apply_tcolumns_change_applies_when_delta_is_zero);
  RUN_TEST(test_apply_tcolumns_change_keeps_state_in_sync_over_many_moves);
  RUN_TEST(test_coverage_counters_survive_more_than_255_rows);
  RUN_TEST(test_pv_validate_is_idempotent);
  RUN_TEST(test_pv_validate_agrees_with_ca_validate);
  RUN_TEST(test_failed_parallel_validation_invalidates_summary);
  RUN_TEST(test_gray_code_sequences_do_not_leak_state);
  RUN_TEST(test_gray_code_stays_exhausted_until_reinitialized);
  RUN_TEST(test_pair_diversity_rejects_arithmetic_overflow);
  RUN_TEST(test_ca_add_row_uses_geometric_growth_and_validates_input);
  RUN_TEST(test_ca_add_row_coverage_refuses_counter_overflow);
  RUN_TEST(test_shuffle_is_unbiased);
  RUN_TEST(test_set64_grows_dense_array);
  RUN_TEST(test_set64_insert_ignores_duplicates);
  RUN_TEST(test_set64_delete_finds_every_present_key);
  RUN_TEST(test_set64_random_reaches_every_element);
  RUN_TEST(test_set64_make_key_roundtrips);
  RUN_TEST(test_set64_rejects_unrepresentable_capacity);
  return UNITY_END();
}
