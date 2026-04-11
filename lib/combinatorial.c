#include "combinatorial.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>

static void swap_int(int *x, int *y) {
  int tmp = *x;
  *x = *y;
  *y = tmp;
}

void shuffle(int *array, int n) {
  for (int i = 0; i < n; ++i) {
    array[i] = i;
  }
  for (int i = 0; i < n; ++i) {
    int r = rand() % n;
    swap_int(&array[i], &array[r]);
  }
  for (int i = 0; i < n; ++i) {
    fprintf(stderr, "%d ", array[i]);
  }
  fprintf(stderr, "\n");
}

int binomial(int k, int r) {
  if (k < r) {
    return 0;
  }
  int b = 1;
  for (int i = 1; i <= r; ++i) {
    b = (b * (k - i + 1)) / i;
  }
  return b;
}

int t_wise(int **GTP, int k, int t) {
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

int t_wise_visit(int k, int t, t_combination_callback cb, void *user_data) {
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

void inv_ruffini(int *V, int num, int v, int t) {
  for (int i = 0; i < t; ++i) {
    V[i] = 0;
  }
  int i = t;
  while (num > 0) {
    V[--i] = num % v;
    num /= v;
  }
}

int get_col(int *line, int **IToC, int j, int t, int v) {
  int i, res = line[IToC[j][0]];
  if (res == v) {
    return -1;
  }
  for (i = 1; i < t; ++i) {
    if (line[IToC[j][i]] == v) {
      return -1;
    }
    res = res * v + line[IToC[j][i]];
  }
  return res;
}

int **generate_t_combinations(int k, int t, int *out_n) {
  int n = binomial(k, t);
  int **GTP = get_matrix(n, t);
  t_wise(GTP, k, t);
  if (out_n != NULL) {
    *out_n = n;
  }
  return GTP;
}

static int compare_int(const void *a, const void *b) {
  int ia = *(const int *)a;
  int ib = *(const int *)b;
  if (ia < ib)
    return -1;
  if (ia > ib)
    return 1;
  return 0;
}

void init_permutation(int *arr, int n) {
  qsort(arr, n, sizeof(int), compare_int);
}

int next_permutation(int *arr, int n) {
  if (n <= 1) {
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

void init_gray_code(int *arr, int n) {
  for (int i = 0; i < n; i++) {
    arr[i] = 0;
  }
}

int next_gray_code(int *arr, int n, int v) {
  static int *dir = NULL;
  static int dir_capacity = 0;
  static int initialized = 0;

  // Initialize direction array if needed
  if (dir == NULL || dir_capacity < n) {
    if (dir != NULL) {
      free(dir);
    }
    dir = (int *)malloc(n * sizeof(int));
    dir_capacity = n;
    initialized = 0;
  }

  // Initialize directions on first use
  if (!initialized) {
    // Initialize all directions to +1 (moving upward)
    for (int i = 0; i < n; i++) {
      dir[i] = +1;
    }
    initialized = 1;
  }

  // Find the RIGHTMOST mobile element
  // An element at position i is mobile if:
  //   arr[i] + dir[i] is within bounds [0, v-1]
  int mobile_pos = -1;

  for (int i = n - 1; i >= 0; i--) {
    int next_val = arr[i] + dir[i];
    if (next_val >= 0 && next_val < v) {
      mobile_pos = i;
      break;
    }
  }

  // If no mobile element found, we've generated all codes
  if (mobile_pos == -1) {
    // Reset state for next use
    free(dir);
    dir = NULL;
    dir_capacity = 0;
    initialized = 0;
    return 0;
  }

  // Move the mobile element
  arr[mobile_pos] += dir[mobile_pos];

  // Reverse direction of all elements to the RIGHT of mobile_pos
  for (int i = mobile_pos + 1; i < n; i++) {
    dir[i] = -dir[i];
  }

  return 1;
}
