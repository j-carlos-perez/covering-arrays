#include <stdlib.h>
#include <stdio.h>
#include "combinatorial.h"
#include "memory.h"

static void swap_int(int *x, int *y)
{
    int tmp = *x;
    *x = *y;
    *y = tmp;
}

void shuffle(int *array, int n)
{
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

int binomial(int k, int r)
{
    if (k < r) {
        return 0;
    }
    int b = 1;
    for (int i = 1; i <= r; ++i) {
        b = (b * (k - i + 1)) / i;
    }
    return b;
}

int t_wise(int **GTP, int k, int t)
{
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

int t_wise_visit(int k, int t, t_combination_callback cb, void *user_data)
{
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

void inv_ruffini(int *V, int num, int v, int t)
{
    for (int i = 0; i < t; ++i) {
        V[i] = 0;
    }
    int i = t;
    while (num > 0) {
        V[--i] = num % v;
        num /= v;
    }
}

int get_col(int *line, int **IToC, int j, int t, int v)
{
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

int **generate_t_combinations(int k, int t, int *out_n)
{
    int n = binomial(k, t);
    int **GTP = get_matrix(n, t);
    t_wise(GTP, k, t);
    if (out_n != NULL) {
        *out_n = n;
    }
    return GTP;
}
