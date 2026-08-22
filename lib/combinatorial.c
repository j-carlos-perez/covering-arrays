#include "combinatorial.h"
#include "memory.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Swaps two integers in place.
 */
static void swap_int(int *x, int *y) {
  int tmp = *x;
  *x = *y;
  *y = tmp;
}

/*
 * Uniform random integer in [0, bound), by rejection sampling.
 * See the header for why plain modulo is not good enough here.
 */
int rand_below(int bound) {
  if (bound <= 0) {
    return 0;
  }
  /* Largest multiple of bound that fits; draws at or above it are rejected. */
  unsigned int limit = (unsigned int)RAND_MAX + 1u;
  limit -= limit % (unsigned int)bound;

  unsigned int r;
  do {
    r = (unsigned int)rand();
  } while (r >= limit);

  return (int)(r % (unsigned int)bound);
}

/*
 * Fills array with [0, n-1] and shuffles it into a uniformly random
 * permutation (Fisher-Yates).
 *
 * The swap partner must be drawn from the *unvisited* suffix [i, n). Drawing
 * from the whole range produces n^n equally likely execution paths mapped onto
 * n! permutations, which cannot be uniform.
 */
void shuffle(int *array, int n) {
  if (array == NULL || n <= 0) {
    return;
  }
  for (int i = 0; i < n; ++i) {
    array[i] = i;
  }
  for (int i = 0; i < n - 1; ++i) {
    int r = i + rand_below(n - i);
    swap_int(&array[i], &array[r]);
  }
}

/*
 * Non-zero if n can be used as an element count on this platform.
 */
int binomial_is_usable(uint64_t n) {
  return n != BINOMIAL_OVERFLOW && n <= (uint64_t)SIZE_MAX;
}

/*
 * Computes binomial coefficient C(k, r) = k! / (r! * (k-r)!).
 *
 * Returns 0 if r < 0, k < 0, or k < r, and BINOMIAL_OVERFLOW if the result
 * exceeds a uint64_t. The running product is checked before every multiply:
 * the naive `b = b * (k - i + 1) / i` overflows its intermediate long before
 * the answer itself gets large, and the old int-returning version silently
 * handed callers a negative number that they then widened into a huge size_t.
 */
static uint64_t gcd_u64(uint64_t a, uint64_t b) {
  while (b != 0) {
    uint64_t tmp = a % b;
    a = b;
    b = tmp;
  }
  return a;
}

uint64_t binomial(int k, int r) {
  if (k < 0 || r < 0 || k < r) {
    return 0;
  }

  /* C(k,r) == C(k,k-r); the smaller r keeps the loop short. */
  if (r > k - r) {
    r = k - r;
  }

  uint64_t b = 1;
  for (int i = 1; i <= r; ++i) {
    /* b is C(k,i-1) here and the exact next value is b * (k-i+1) / i.
       Forming that product first overflows long before the answer does --
       C(63,29) fits in a uint64_t but its intermediate does not. Cancel the
       common factor before multiplying instead: with g = gcd(num, i),
       gcd(num/g, i/g) == 1 and (i/g) divides b*(num/g), so (i/g) must divide
       b and the division below is exact. */
    uint64_t num = (uint64_t)(k - i + 1);
    uint64_t den = (uint64_t)i;
    uint64_t g = gcd_u64(num, den);
    num /= g;
    den /= g;

    b /= den;
    uint64_t product;
    if (__builtin_mul_overflow(b, num, &product)) {
      return BINOMIAL_OVERFLOW;
    }
    b = product;
  }
  return b;
}

/*
 * Generates all C(k,t) combinations of k columns taken t at a time.
 * Each row of GTP contains t column indices (sorted ascending).
 *
 * Returns 0 on success, -1 if the parameters are out of range. Callers used to
 * pass t > k, for which C(k,t) is 0 and GTP therefore has no rows -- the
 * generator then wrote through GTP[0] and ran away.
 */
int t_wise(int **GTP, int k, int t) {
  if (GTP == NULL || t < 1 || k < t) {
    return -1;
  }
  uint64_t count = binomial(k, t);
  if (!binomial_is_usable(count) || count > INT_MAX) {
    return -1;
  }

  int J[t];
  long long i, iMax, actual = 0;
  for (i = 0; i < t; i++) {
    J[i] = i;
  }
  for (iMax = t - 1, i = 0; i < t; i++) {
    if (J[i] == k - t + i) {
      iMax = i;
      break;
    }
  }
  do {
    for (i = 0; i < t; i++) {
      GTP[actual][i] = J[i];
    }
    actual++;
    J[t - 1]++;
    if (J[t - 1] == k) {
      if (iMax == 0) {
        break;
      }
      J[iMax - 1]++;
      for (i = iMax; i < t; i++) {
        J[i] = J[i - 1] + 1;
      }
      if (J[iMax - 1] == k - t + iMax - 1) {
        iMax = iMax - 1;
      } else {
        iMax = t - 1;
      }
    }
  } while (1);
  return 0;
}

/*
 * Visits all C(k,t) column combinations, calling cb(J, actual, k, t, user_data)
 * for each. Same as t_wise but uses a callback instead of filling a matrix.
 * Returns the number of combinations generated, or 0 for out-of-range input.
 */
int t_wise_visit(int k, int t, t_combination_callback cb, void *user_data) {
  if (t < 1 || k < t) {
    return 0;
  }
  uint64_t count = binomial(k, t);
  if (!binomial_is_usable(count) || count > INT_MAX) {
    return 0;
  }

  int J[t];
  long long i, iMax, actual = 0;
  for (i = 0; i < t; i++) {
    J[i] = i;
  }
  for (iMax = t - 1, i = 0; i < t; i++) {
    if (J[i] == k - t + i) {
      iMax = i;
      break;
    }
  }
  do {
    if (cb != NULL) {
      cb(J, actual, k, t, user_data);
    }
    actual++;
    J[t - 1]++;
    if (J[t - 1] == k) {
      if (iMax == 0) {
        break;
      }
      J[iMax - 1]++;
      for (i = iMax; i < t; i++) {
        J[i] = J[i - 1] + 1;
      }
      if (J[iMax - 1] == k - t + iMax - 1) {
        iMax = iMax - 1;
      } else {
        iMax = t - 1;
      }
    }
  } while (1);
  return actual;
}

/*
 * Computes the base-v representation of num with t digits.
 * Stores digits in V[0..t-1] (most significant first, matching get_col()).
 *
 * Returns 0 on success, -1 if the parameters are out of range or num needs
 * more than t digits. The unguarded version walked its index below zero and
 * wrote in front of V.
 */
int inv_ruffini(int *V, int num, int v, int t) {
  if (V == NULL || t < 1 || v < 2 || num < 0) {
    return -1;
  }

  for (int i = 0; i < t; ++i) {
    V[i] = 0;
  }

  int i = t;
  while (num > 0) {
    if (i == 0) {
      return -1; /* num >= v^t: does not fit in t digits */
    }
    V[--i] = num % v;
    num /= v;
  }
  return 0;
}

/*
 * Encodes a t-tuple from line using columns IToC[j].
 *
 * Reads t symbols from line at positions specified by IToC row j.
 * Encodes as mixed-radix number: s[0]*v^(t-1) + ... + s[t-1].
 * Returns -1 for a wildcard, invalid symbol/argument, or int overflow.
 */
int get_col(const int *line, int **IToC, int j, int t, int v) {
  if (line == NULL || IToC == NULL || j < 0 || t < 1 || v < 2 ||
      IToC[j] == NULL) {
    return -1;
  }

  int res = line[IToC[j][0]];
  if (res < 0 || res >= v) {
    return -1;
  }
  for (int i = 1; i < t; ++i) {
    int symbol = line[IToC[j][i]];
    if (symbol < 0 || symbol >= v) {
      return -1;
    }
    int64_t next = (int64_t)res * v + symbol;
    if (next > INT_MAX) {
      return -1;
    }
    res = (int)next;
  }
  return res;
}

/*
 * Generates all C(k,t) column combinations and stores in a matrix.
 * Calls t_wise() internally.
 * Sets *out_n to the number of combinations if out_n != NULL.
 * Returns NULL if k, t are out of range or C(k,t) is not allocatable.
 */
int **generate_t_combinations(int k, int t, int *out_n) {
  uint64_t n = binomial(k, t);
  if (!binomial_is_usable(n) || n == 0 || n > INT_MAX) {
    if (out_n != NULL) {
      *out_n = 0;
    }
    return NULL;
  }

  int **GTP = get_matrix((size_t)n, (size_t)t);
  if (GTP == NULL) {
    if (out_n != NULL) {
      *out_n = 0;
    }
    return NULL;
  }

  if (t_wise(GTP, k, t) != 0) {
    free_matrix(GTP, (size_t)n);
    if (out_n != NULL) {
      *out_n = 0;
    }
    return NULL;
  }

  if (out_n != NULL) {
    *out_n = (int)n;
  }
  return GTP;
}

/*
 * Comparison function for qsort.
 */
static int compare_int(const void *a, const void *b) {
  int ia = *(const int *)a;
  int ib = *(const int *)b;
  if (ia < ib)
    return -1;
  if (ia > ib)
    return 1;
  return 0;
}

/*
 * Initializes an array as a sorted permutation [0, 1, ..., n-1].
 */
void init_permutation(int *arr, int n) {
  if (arr == NULL || n <= 0) {
    return;
  }
  qsort(arr, n, sizeof(int), compare_int);
}

/*
 * Generates the next lexicographic permutation in place.
 * Modifies arr to the next permutation.
 * Returns 1 if more permutations exist, 0 if exhausted.
 */
int next_permutation(int *arr, int n) {
  if (arr == NULL || n <= 1) {
    return 0;
  }

  int j = n - 2;
  while (j >= 0 && arr[j] >= arr[j + 1]) {
    j--;
  }
  if (j < 0) {
    return 0;
  }

  int l = n - 1;
  while (arr[j] >= arr[l]) {
    l--;
  }

  int tmp = arr[j];
  arr[j] = arr[l];
  arr[l] = tmp;

  int lo = j + 1;
  int hi = n - 1;
  while (lo < hi) {
    tmp = arr[lo];
    arr[lo] = arr[hi];
    arr[hi] = tmp;
    lo++;
    hi--;
  }

  return 1;
}

/*
 * Shared direction state for the Gray code walk.
 *
 * NOT thread-safe: one sequence at a time per process. init_gray_code() is the
 * start-of-sequence marker and resets this state, so a sequence that is
 * abandoned part-way no longer poisons the next one.
 */
static int *gray_dir = NULL;
static int gray_capacity = 0;
static int gray_length = 0;
static int gray_exhausted = 1;

static void gray_reset(int n) {
  if (n <= 0) {
    gray_length = 0;
    gray_exhausted = 1;
    return;
  }
  if (n > gray_capacity) {
    free(gray_dir);
    gray_dir = (int *)malloc((size_t)n * sizeof(int));
    gray_capacity = (gray_dir != NULL) ? n : 0;
  }
  if (gray_dir == NULL) {
    gray_length = 0;
    gray_exhausted = 1;
    return;
  }
  for (int i = 0; i < n; i++) {
    gray_dir[i] = +1;
  }
  gray_length = n;
  gray_exhausted = 0;
}

/*
 * Initializes a Gray code sequence to all zeros and resets the walk state.
 */
void init_gray_code(int *arr, int n) {
  if (arr == NULL || n <= 0) {
    gray_length = 0;
    gray_exhausted = 1;
    return;
  }
  for (int i = 0; i < n; i++) {
    arr[i] = 0;
  }
  gray_reset(n);
}

/*
 * Generates the next Gray code in the sequence.
 * Uses the standard algorithm: find rightmost mobile element, move it,
 * then reverse direction of all elements to its right.
 *
 * Returns 1 if generated, 0 if all codes exhausted.
 * Not thread-safe: see gray_dir above.
 */
int next_gray_code(int *arr, int n, int v) {
  if (arr == NULL || n <= 0 || v < 2 || gray_exhausted) {
    return 0;
  }

  if (gray_dir == NULL || gray_length != n) {
    return 0; /* every sequence must begin with init_gray_code() */
  }

  // Find the RIGHTMOST mobile element
  // An element at position i is mobile if:
  //   arr[i] + dir[i] is within bounds [0, v-1]
  int mobile_pos = -1;

  for (int i = n - 1; i >= 0; i--) {
    int next_val = arr[i] + gray_dir[i];
    if (next_val >= 0 && next_val < v) {
      mobile_pos = i;
      break;
    }
  }

  // If no mobile element found, we've generated all codes
  if (mobile_pos == -1) {
    gray_exhausted = 1;
    return 0;
  }

  // Move the mobile element
  arr[mobile_pos] += gray_dir[mobile_pos];

  // Reverse direction of all elements to the RIGHT of mobile_pos
  for (int i = mobile_pos + 1; i < n; i++) {
    gray_dir[i] = -gray_dir[i];
  }

  return 1;
}
