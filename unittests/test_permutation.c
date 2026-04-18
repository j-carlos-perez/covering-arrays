#include "lib/combinatorial.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  int passed = 0;
  int failed = 0;

  printf("=== Permutation Tests ===\n\n");

  /* Test 1: Basic permutation with distinct elements */
  {
    printf("Test 1: Permutation of {1,2,3,4}\n");
    int arr[] = {1, 2, 3, 4};
    int n = 4;
    int expected = 24; /* 4! = 24 */
    int count = 0;

    init_permutation(arr, n);
    do {
      count++;
    } while (next_permutation(arr, n));

    if (count == expected) {
      printf("  PASSED: %d permutations (expected %d)\n", count, expected);
      passed++;
    } else {
      printf("  FAILED: %d permutations (expected %d)\n", count, expected);
      failed++;
    }
  }

  /* Test 2: Permutation with duplicates - should generate unique permutations
   */
  {
    printf("Test 2: Permutation of {1,1,2} (with duplicates)\n");
    int arr[] = {1, 1, 2};
    int n = 3;
    int expected = 3; /* 3! / 2! = 3 unique permutations */
    int count = 0;

    init_permutation(arr, n);
    do {
      count++;
    } while (next_permutation(arr, n));

    if (count == expected) {
      printf("  PASSED: %d unique permutations (expected %d)\n", count,
             expected);
      passed++;
    } else {
      printf("  FAILED: %d unique permutations (expected %d)\n", count,
             expected);
      failed++;
    }
  }

  /* Test 3: Permutation with 10 zeros and 6 ones */
  {
    printf("Test 3: Permutation of {0,...,0,1,...,1} (10 zeros, 6 ones)\n");
    int n = 16;
    int *arr = malloc(n * sizeof(int));
    for (int i = 0; i < 10; i++)
      arr[i] = 0;
    for (int i = 10; i < 16; i++)
      arr[i] = 1;

    int expected = 8008; /* 16! / (10! * 6!) = 8008 */
    int count = 0;

    init_permutation(arr, n);
    do {
      count++;
    } while (next_permutation(arr, n));

    if (count == expected) {
      printf("  PASSED: %d unique permutations (expected %d)\n", count,
             expected);
      passed++;
    } else {
      printf("  FAILED: %d unique permutations (expected %d)\n", count,
             expected);
      failed++;
    }
    free(arr);
  }

  /* Test 4: Single element */
  {
    printf("Test 4: Permutation of {1} (single element)\n");
    int arr[] = {1};
    int n = 1;
    int expected = 1;
    int count = 0;

    init_permutation(arr, n);
    do {
      count++;
    } while (next_permutation(arr, n));

    if (count == expected) {
      printf("  PASSED: %d permutation (expected %d)\n", count, expected);
      passed++;
    } else {
      printf("  FAILED: %d permutation (expected %d)\n", count, expected);
      failed++;
    }
  }

  /* Test 5: Empty array */
  {
    printf("Test 5: Permutation of empty array\n");
    int *arr = NULL;
    int n = 0;
    int expected = 1; /* Single empty permutation */
    int count = 0;

    init_permutation(arr, n);
    do {
      count++;
    } while (next_permutation(arr, n));

    if (count == expected) {
      printf("  PASSED: %d permutations (expected %d)\n", count, expected);
      passed++;
    } else {
      printf("  FAILED: %d permutations (expected %d)\n", count, expected);
      failed++;
    }
  }

  /* Test 6: Already sorted array */
  {
    printf("Test 6: Permutation of already sorted {1,2,3}\n");
    int arr[] = {1, 2, 3};
    int n = 3;
    int expected = 6; /* 3! = 6 */
    int count = 0;

    init_permutation(arr, n);
    do {
      count++;
    } while (next_permutation(arr, n));

    if (count == expected) {
      printf("  PASSED: %d permutations (expected %d)\n", count, expected);
      passed++;
    } else {
      printf("  FAILED: %d permutations (expected %d)\n", count, expected);
      failed++;
    }
  }

  printf("\n=== Summary ===\n");
  printf("Passed: %d\n", passed);
  printf("Failed: %d\n", failed);

  return failed > 0 ? 1 : 0;
}
