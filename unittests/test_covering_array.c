#include "../lib/covering_array.h"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char test_directory[] = "/tmp/covering-array-test-XXXXXX";

void setUp(void) {}
void tearDown(void) {}

static covering_array_t *create_array(const int *values, int N, int k, int v,
                                      int t) {
  covering_array_t *ca = ca_create(N, k, v, t);
  TEST_ASSERT_NOT_NULL(ca);

  for (int i = 0; i < N; i++) {
    memcpy(ca->matrix[i], &values[i * k], (size_t)k * sizeof(int));
  }

  return ca;
}

static int path_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0;
}

void test_ca_save_uses_complete_filename_for_covering_array(void) {
  const int values[] = {0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0};
  covering_array_t *ca = create_array(values, 4, 3, 2, 2);
  TEST_ASSERT_TRUE(ca_validate(ca));

  TEST_ASSERT_EQUAL_INT(0, ca_save(test_directory, ca, "complete test"));

  char path[1024];
  snprintf(path, sizeof(path), "%s/N4k3v2^3t2.ca", test_directory);
  TEST_ASSERT_TRUE(path_exists(path));

  unlink(path);
  ca_destroy(ca);
}

void test_ca_save_derives_missing_count_for_incomplete_array(void) {
  const int values[] = {0, 0, 0};
  covering_array_t *ca = create_array(values, 1, 3, 2, 2);
  TEST_ASSERT_FALSE(ca_validate(ca));
  TEST_ASSERT_EQUAL_size_t(9, ca->total - ca->covered);

  TEST_ASSERT_EQUAL_INT(0, ca_save(test_directory, ca, "incomplete test"));

  char path[1024];
  snprintf(path, sizeof(path), "%s/N1k3v2^3t2.ca.missing9", test_directory);
  TEST_ASSERT_TRUE(path_exists(path));

  unlink(path);
  ca_destroy(ca);
}

void test_incremental_row_coverage_matches_full_validation(void) {
  const int initial_values[] = {0, 0, 0};
  const int full_values[] = {0, 0, 0, 0, 1, 1};
  const int new_row[] = {0, 1, 1};
  covering_array_t *incremental = create_array(initial_values, 1, 3, 2, 2);
  covering_array_t *full = create_array(full_values, 2, 3, 2, 2);

  ca_validate(incremental);
  TEST_ASSERT_EQUAL_INT(0, ca_add_row(incremental, new_row));
  TEST_ASSERT_EQUAL_INT(0, ca_add_row_coverage(incremental, new_row));
  ca_validate(full);

  TEST_ASSERT_EQUAL_size_t(full->covered, incremental->covered);
  TEST_ASSERT_EQUAL_size_t(full->total, incremental->total);

  size_t group_count = binomial(full->k, full->t);
  size_t combinations_per_group = 1;
  for (int i = 0; i < full->t; i++) {
    combinations_per_group *= (size_t)full->v;
  }

  for (size_t group = 0; group < group_count; group++) {
    TEST_ASSERT_EQUAL_size_t(full->tcomb_counter[group],
                             incremental->tcomb_counter[group]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(full->P[group], incremental->P[group],
                                  combinations_per_group);
  }

  ca_destroy(incremental);
  ca_destroy(full);
}

void test_io_rejects_null_paths(void) {
  const int values[] = {0};
  covering_array_t *ca = create_array(values, 1, 1, 2, 1);
  TEST_ASSERT_EQUAL_INT(-1, ca_save(NULL, ca, NULL));
  TEST_ASSERT_NULL(ca_load(NULL));
  ca_destroy(ca);
}

int main(void) {
  if (mkdtemp(test_directory) == NULL) {
    return EXIT_FAILURE;
  }

  UNITY_BEGIN();
  RUN_TEST(test_ca_save_uses_complete_filename_for_covering_array);
  RUN_TEST(test_ca_save_derives_missing_count_for_incomplete_array);
  RUN_TEST(test_incremental_row_coverage_matches_full_validation);
  RUN_TEST(test_io_rejects_null_paths);
  int result = UNITY_END();

  rmdir(test_directory);
  return result;
}
