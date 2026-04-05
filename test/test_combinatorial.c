#include "unity.h"
#include "../lib/combinatorial.h"
#include "../lib/memory.h"
#include <stdlib.h>

void setUp(void) {}
void tearDown(void) {}

void test_binomial_returns_correct_values(void) {
    TEST_ASSERT_EQUAL_INT(1, binomial(5, 0));
    TEST_ASSERT_EQUAL_INT(1, binomial(5, 5));
    TEST_ASSERT_EQUAL_INT(10, binomial(5, 2));
    TEST_ASSERT_EQUAL_INT(120, binomial(10, 3));
    TEST_ASSERT_EQUAL_INT(0, binomial(3, 5));
    TEST_ASSERT_EQUAL_INT(6, binomial(4, 2));
    TEST_ASSERT_EQUAL_INT(1, binomial(1, 0));
    TEST_ASSERT_EQUAL_INT(0, binomial(0, 1));
}

void test_t_wise_generates_all_combinations(void) {
    int k = 4, t = 2;
    int n = binomial(k, t);
    TEST_ASSERT_EQUAL_INT(6, n);

    int **GTP = get_matrix(n, t);
    t_wise(GTP, k, t);

    int expected[6][2] = {
        {0, 1}, {0, 2}, {0, 3},
        {1, 2}, {1, 3},
        {2, 3}
    };

    for (int i = 0; i < n; i++) {
        TEST_ASSERT_EQUAL_INT_ARRAY(expected[i], GTP[i], t);
    }

    free_matrix(GTP, n);
}

static int visit_count;
static int **visit_store;
static int visit_n;

static void t_wise_callback(int *combination, int index, int k, int t, void *user_data) {
    (void)k;
    (void)t;
    int *counter = (int *)user_data;
    if (counter != NULL)
        (*counter)++;
    for (int i = 0; i < visit_n; i++) {
        visit_store[index][i] = combination[i];
    }
}

void test_t_wise_visit_calls_callback_correct_number_of_times(void) {
    int k = 4, t = 2;
    visit_n = 2;
    int n = binomial(k, t);
    visit_count = 0;

    visit_store = get_matrix(n, t);

    int visited = t_wise_visit(k, t, t_wise_callback, &visit_count);

    TEST_ASSERT_EQUAL_INT(n, visited);
    TEST_ASSERT_EQUAL_INT(n, visit_count);

    int expected[6][2] = {
        {0, 1}, {0, 2}, {0, 3},
        {1, 2}, {1, 3},
        {2, 3}
    };

    for (int i = 0; i < n; i++) {
        TEST_ASSERT_EQUAL_INT_ARRAY(expected[i], visit_store[i], t);
    }

    free_matrix(visit_store, n);
}

void test_generate_t_combinations_allocates_and_fills_correctly(void) {
    int k = 4, t = 2;
    int n;
    int **GTP = generate_t_combinations(k, t, &n);

    TEST_ASSERT_EQUAL_INT(6, n);

    int expected[6][2] = {
        {0, 1}, {0, 2}, {0, 3},
        {1, 2}, {1, 3},
        {2, 3}
    };

    for (int i = 0; i < n; i++) {
        TEST_ASSERT_EQUAL_INT_ARRAY(expected[i], GTP[i], t);
    }

    free_matrix(GTP, n);
}

void test_generate_t_combinations_with_null_out_n(void) {
    int k = 4, t = 2;
    int **GTP = generate_t_combinations(k, t, NULL);
    TEST_ASSERT_NOT_NULL(GTP);
    TEST_ASSERT_EQUAL_INT(6, binomial(k, t));
    free_matrix(GTP, binomial(k, t));
}

void test_inv_ruffini_decodes_base_values(void) {
    int V[3];

    inv_ruffini(V, 0, 3, 3);
    TEST_ASSERT_EQUAL_INT(0, V[0]);
    TEST_ASSERT_EQUAL_INT(0, V[1]);
    TEST_ASSERT_EQUAL_INT(0, V[2]);

    inv_ruffini(V, 12, 3, 3);
    TEST_ASSERT_EQUAL_INT(1, V[0]);
    TEST_ASSERT_EQUAL_INT(1, V[1]);
    TEST_ASSERT_EQUAL_INT(0, V[2]);

    inv_ruffini(V, 26, 3, 3);
    TEST_ASSERT_EQUAL_INT(2, V[0]);
    TEST_ASSERT_EQUAL_INT(2, V[1]);
    TEST_ASSERT_EQUAL_INT(2, V[2]);

    inv_ruffini(V, 1, 3, 3);
    TEST_ASSERT_EQUAL_INT(0, V[0]);
    TEST_ASSERT_EQUAL_INT(0, V[1]);
    TEST_ASSERT_EQUAL_INT(1, V[2]);
}

void test_get_col_encodes_tuples_correctly(void) {
    int k = 5, t = 2, v = 3;
    int n_comb = binomial(k, t);
    int **IToC = get_matrix(n_comb, t);
    t_wise(IToC, k, t);

    int line[] = {0, 1, 0, 1, 2};

    TEST_ASSERT_EQUAL_INT(1, get_col(line, IToC, 0, t, v));
    TEST_ASSERT_EQUAL_INT(0, get_col(line, IToC, 1, t, v));
    TEST_ASSERT_EQUAL_INT(1, get_col(line, IToC, 2, t, v));
    TEST_ASSERT_EQUAL_INT(2, get_col(line, IToC, 3, t, v));
    TEST_ASSERT_EQUAL_INT(3, get_col(line, IToC, 4, t, v));
    TEST_ASSERT_EQUAL_INT(4, get_col(line, IToC, 5, t, v));
    TEST_ASSERT_EQUAL_INT(5, get_col(line, IToC, 6, t, v));
    TEST_ASSERT_EQUAL_INT(1, get_col(line, IToC, 7, t, v));
    TEST_ASSERT_EQUAL_INT(2, get_col(line, IToC, 8, t, v));
    TEST_ASSERT_EQUAL_INT(5, get_col(line, IToC, 9, t, v));

    free_matrix(IToC, n_comb);
}

void test_get_col_returns_minus_one_when_wildcard_in_tuple(void) {
    int k = 4, t = 2, v = 3;
    int n_comb = binomial(k, t);
    int **IToC = get_matrix(n_comb, t);
    t_wise(IToC, k, t);

    int line[] = {0, 3, 2, 1};
    TEST_ASSERT_EQUAL_INT(-1, get_col(line, IToC, 0, t, v));
    TEST_ASSERT_EQUAL_INT(2, get_col(line, IToC, 1, t, v));
    TEST_ASSERT_EQUAL_INT(1, get_col(line, IToC, 2, t, v));

    int line2[] = {3, 0, 1, 2};
    TEST_ASSERT_EQUAL_INT(-1, get_col(line2, IToC, 0, t, v));
    TEST_ASSERT_EQUAL_INT(-1, get_col(line2, IToC, 1, t, v));

    int line3[] = {0, 1, 2, 3};
    TEST_ASSERT_EQUAL_INT(1, get_col(line3, IToC, 0, t, v));
    TEST_ASSERT_EQUAL_INT(2, get_col(line3, IToC, 1, t, v));
    TEST_ASSERT_EQUAL_INT(-1, get_col(line3, IToC, 2, t, v));

    free_matrix(IToC, n_comb);
}

void test_get_col_and_inv_ruffini_are_inverses(void) {
    int k = 4, t = 2, v = 3;
    int n_comb = binomial(k, t);
    int **IToC = get_matrix(n_comb, t);
    t_wise(IToC, k, t);

    int line[] = {2, 1, 0, 2};

    int col_idx = get_col(line, IToC, 0, t, v);
    TEST_ASSERT_TRUE(col_idx >= 0);

    int decoded[2];
    inv_ruffini(decoded, col_idx, v, t);

    TEST_ASSERT_EQUAL_INT(line[IToC[0][0]], decoded[0]);
    TEST_ASSERT_EQUAL_INT(line[IToC[0][1]], decoded[1]);

    free_matrix(IToC, n_comb);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_binomial_returns_correct_values);
    RUN_TEST(test_t_wise_generates_all_combinations);
    RUN_TEST(test_t_wise_visit_calls_callback_correct_number_of_times);
    RUN_TEST(test_generate_t_combinations_allocates_and_fills_correctly);
    RUN_TEST(test_generate_t_combinations_with_null_out_n);
    RUN_TEST(test_inv_ruffini_decodes_base_values);
    RUN_TEST(test_get_col_encodes_tuples_correctly);
    RUN_TEST(test_get_col_returns_minus_one_when_wildcard_in_tuple);
    RUN_TEST(test_get_col_and_inv_ruffini_are_inverses);
    return UNITY_END();
}
