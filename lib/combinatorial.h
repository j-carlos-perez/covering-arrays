#ifndef COMBINATORIAL_H
#define COMBINATORIAL_H

void shuffle(int *array, int n);
int binomial(int k, int r);
int t_wise(int **GTP, int k, int t);
void inv_ruffini(int *V, int num, int v, int t);
int get_col(const int *line, int **IToC, int j, int t, int v);

int **generate_t_combinations(int k, int t, int *out_n);

typedef void (*t_combination_callback)(int *combination, int index, int k,
                                       int t, void *user_data);
int t_wise_visit(int k, int t, t_combination_callback cb, void *user_data);

void init_permutation(int *arr, int n);
int next_permutation(int *arr, int n);

void init_gray_code(int *arr, int n);
int next_gray_code(int *arr, int n, int v);

#endif
