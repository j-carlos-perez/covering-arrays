#ifndef MEMORY_H
#define MEMORY_H

int **get_matrix(int r, int c);
int *get_vector(int r);
void free_matrix(int **m, int r);
void free_vector(int *v);

#endif
