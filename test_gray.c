#include "lib/combinatorial.h"
#include <stdio.h>
#include <stdlib.h>

static int hamming_distance(int *a, int *b, int n) {
  int dist = 0;
  for (int i = 0; i < n; i++) {
    if (a[i] != b[i])
      dist++;
  }
  return dist;
}

int main(void) {
  int passed = 0;
  int failed = 0;

  printf("=== Gray Code Tests ===\n\n");

  /* Test 1: Binary Gray code (n=3, v=2) */
  {
    printf("Test 1: Binary Gray code (length=3)\n");
    int n = 3;
    int v = 2;
    int *arr = malloc(n * sizeof(int));
    int *prev = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
      arr[i] = 0;

    int expected = 8; /* 2^3 = 8 */
    int count = 0;
    int all_hamming_1 = 1;

    init_gray_code(arr, n);
    for (int i = 0; i < n; i++)
      prev[i] = arr[i];

    do {
      count++;
      int dist = hamming_distance(prev, arr, n);
      if (dist != 1 && count > 1) {
        all_hamming_1 = 0;
      }
      for (int i = 0; i < n; i++)
        prev[i] = arr[i];
    } while (next_gray_code(arr, n, v));

    free(arr);
    free(prev);

    if (count == expected && all_hamming_1) {
      printf("  PASSED: %d codes, all hamming distances = 1\n", count);
      passed++;
    } else {
      printf("  FAILED: %d codes (expected %d), hamming check: %s\n", count,
             expected, all_hamming_1 ? "PASS" : "FAIL");
      failed++;
    }
  }

  /* Test 2: 3-ary Gray code (n=2, v=3) */
  {
    printf("Test 2: 3-ary Gray code (length=2, base=3)\n");
    int n = 2;
    int v = 3;
    int *arr = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
      arr[i] = 0;

    int expected = 9; /* 3^2 = 9 */
    int count = 0;

    init_gray_code(arr, n);
    do {
      count++;
    } while (next_gray_code(arr, n, v));

    free(arr);

    if (count == expected) {
      printf("  PASSED: %d codes (expected %d)\n", count, expected);
      passed++;
    } else {
      printf("  FAILED: %d codes (expected %d)\n", count, expected);
      failed++;
    }
  }

  /* Test 3: Binary Gray code (n=8, v=2) */
  {
    printf("Test 3: Binary Gray code (length=8)\n");
    int n = 8;
    int v = 2;
    int *arr = malloc(n * sizeof(int));
    int *prev = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
      arr[i] = 0;

    int expected = 256; /* 2^8 = 256 */
    int count = 0;
    int all_hamming_1 = 1;

    init_gray_code(arr, n);
    for (int i = 0; i < n; i++)
      prev[i] = arr[i];

    do {
      count++;
      int dist = hamming_distance(prev, arr, n);
      if (dist != 1 && count > 1) {
        all_hamming_1 = 0;
      }
      for (int i = 0; i < n; i++)
        prev[i] = arr[i];
    } while (next_gray_code(arr, n, v));

    free(arr);
    free(prev);

    if (count == expected && all_hamming_1) {
      printf("  PASSED: %d codes, all hamming distances = 1\n", count);
      passed++;
    } else {
      printf("  FAILED: %d codes (expected %d), hamming check: %s\n", count,
             expected, all_hamming_1 ? "PASS" : "FAIL");
      failed++;
    }
  }

  /* Test 4: Binary Gray code (n=16, v=2) */
  {
    printf("Test 4: Binary Gray code (length=16)\n");
    int n = 16;
    int v = 2;
    int *arr = malloc(n * sizeof(int));
    int *prev = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
      arr[i] = 0;

    int expected = 65536; /* 2^16 = 65536 */
    int count = 0;
    int all_hamming_1 = 1;

    init_gray_code(arr, n);
    for (int i = 0; i < n; i++)
      prev[i] = arr[i];

    do {
      count++;
      int dist = hamming_distance(prev, arr, n);
      if (dist != 1 && count > 1) {
        all_hamming_1 = 0;
      }
      for (int i = 0; i < n; i++)
        prev[i] = arr[i];
    } while (next_gray_code(arr, n, v));

    free(arr);
    free(prev);

    if (count == expected && all_hamming_1) {
      printf("  PASSED: %d codes, all hamming distances = 1\n", count);
      passed++;
    } else {
      printf("  FAILED: %d codes (expected %d), hamming check: %s\n", count,
             expected, all_hamming_1 ? "PASS" : "FAIL");
      failed++;
    }
  }

  /* Test 5: Single position Gray code */
  {
    printf("Test 5: Gray code (length=1)\n");
    int n = 1;
    int v = 2;
    int *arr = malloc(n * sizeof(int));
    for (int i = 0; i < n; i++)
      arr[i] = 0;

    int expected = 2; /* 2^1 = 2 */
    int count = 0;

    init_gray_code(arr, n);
    do {
      count++;
    } while (next_gray_code(arr, n, v));

    free(arr);

    if (count == expected) {
      printf("  PASSED: %d codes (expected %d)\n", count, expected);
      passed++;
    } else {
      printf("  FAILED: %d codes (expected %d)\n", count, expected);
      failed++;
    }
  }

  printf("\n=== Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);

  return failed > 0 ? 1 : 0;
}
