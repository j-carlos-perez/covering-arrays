#include "covering_array.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CA_GROWTH_FACTOR 10

/*
 * Computes balanced symbol quotas for a row.
 * Each of the k positions gets a symbol [0, v-1].
 * Quotients are balanced: floor(k/v) or ceil(k/v) per symbol.
 */
static int ca_init_balanced_quotas(int *quotas, int v, int k) {
  int base = k / v;
  int remainder = k % v;

  for (int i = 0; i < v; i++) {
    quotas[i] = base;
  }

  int *symbols = malloc((size_t)v * sizeof(int));
  if (symbols == NULL) {
    return -1;
  }

  for (int i = 0; i < v; i++) {
    symbols[i] = i;
  }

  for (int i = v - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    int tmp = symbols[i];
    symbols[i] = symbols[j];
    symbols[j] = tmp;
  }

  for (int i = 0; i < remainder; i++) {
    quotas[symbols[i]]++;
  }

  free(symbols);
  return 0;
}

/*
 * Fills a row with balanced symbol distribution.
 * Uses quotas to ensure each symbol appears floor(k/v) or ceil(k/v) times.
 */
static int ca_fill_balanced_row(int *row, int k, int v) {
  int *remaining = malloc((size_t)v * sizeof(int));
  if (remaining == NULL) {
    return -1;
  }

  if (ca_init_balanced_quotas(remaining, v, k) != 0) {
    free(remaining);
    return -1;
  }

  for (int col = 0; col < k; col++) {
    int slots_left = k - col;
    int pick = rand() % slots_left;
    int cumulative = 0;
    int selected_symbol = -1;

    for (int symbol = 0; symbol < v; symbol++) {
      cumulative += remaining[symbol];
      if (pick < cumulative) {
        selected_symbol = symbol;
        break;
      }
    }

    if (selected_symbol < 0) {
      free(remaining);
      return -1;
    }

    row[col] = selected_symbol;
    remaining[selected_symbol]--;
  }

  free(remaining);
  return 0;
}

/*
 * Creates a new covering array with N rows and k columns.
 * Each cell is initialized to 0.
 * 
 * v: vocabulary size (symbols in [0, v-1]).
 * t: strength (t-way combinations to cover).
 * 
 * P matrix and tcomb_counter are initialized to NULL (lazy allocation).
 * Caller must free with ca_destroy().
 */
covering_array_t *ca_create(int N, int k, int v, int t) {
  covering_array_t *ca = malloc(sizeof(covering_array_t));
  if (ca == NULL) {
    return NULL;
  }

  ca->N = N;
  ca->capacity = N;
  ca->k = k;
  ca->v = v;
  ca->t = t;

  ca->matrix = malloc(N * sizeof(int *));
  if (ca->matrix == NULL) {
    free(ca);
    return NULL;
  }

  for (int i = 0; i < N; i++) {
    ca->matrix[i] = malloc(k * sizeof(int));
    if (ca->matrix[i] == NULL) {
      for (int j = 0; j < i; j++) {
        free(ca->matrix[j]);
      }
      free(ca->matrix);
      free(ca);
      return NULL;
    }
  }

  ca->P = NULL;
  ca->tcomb_counter = NULL;
  ca->covered = 0;
  ca->total = 0;

  return ca;
}

/*
 * Frees all memory associated with the covering array.
 * Handles NULL gracefully.
 */
void ca_destroy(covering_array_t *ca) {
  if (ca == NULL) {
    return;
  }

  if (ca->matrix != NULL) {
    free_matrix(ca->matrix, ca->N);
  }

  if (ca->P != NULL) {
    size_t R = binomial(ca->k, ca->t);
    free_matrix_uint8(ca->P, R);
  }

  if (ca->tcomb_counter != NULL) {
    free_vector_size_t(ca->tcomb_counter);
  }

  free(ca);
}

/*
 * Validates a covering array and computes its coverage matrix P.
 * 
 * For each t-combination of columns, counts how many rows cover each v^t symbol combo.
 * Updates ca->P, ca->covered, and ca->total.
 * 
 * R = C(k,t) = number of column combinations.
 * C = v^t = number of symbol combinations per set.
 * 
 * Returns 1 if fully covering (covered == total), 0 otherwise.
 */
int ca_validate(covering_array_t *ca) {
  if (ca == NULL || ca->matrix == NULL) {
    return 0;
  }

  size_t R = binomial(ca->k, ca->t);
  size_t C = 1;
  for (int i = 0; i < ca->t; i++) {
    C *= (size_t)ca->v;
  }

  int **IToC = get_matrix((int)R, ca->t);
  t_wise(IToC, ca->k, ca->t);

  if (ca->P == NULL) {
    ca->P = get_matrix_uint8(R, C);
    if (ca->P == NULL) {
      free_matrix(IToC, (int)R);
      return 0;
    }
  }

  if (ca->tcomb_counter == NULL) {
    ca->tcomb_counter = get_vector_size_t(R);
    if (ca->tcomb_counter == NULL) {
      free_matrix(IToC, (int)R);
      free_matrix_uint8(ca->P, (int)R);
      return 0;
    }
  }

  for (size_t i = 0; i < R; i++) {
    ca->tcomb_counter[i] = C;
  }

  for (size_t i = 0; i < R; i++) {
    for (size_t j = 0; j < C; j++) {
      ca->P[i][j] = 0;
    }
  }

  for (int i = 0; i < ca->N; i++) {
    for (size_t j = 0; j < R; j++) {
      int c = get_col(ca->matrix[i], IToC, (int)j, ca->t, ca->v);
      if (c != -1) {
        if (ca->P[j][c] == 0) {
          ca->covered++;
          ca->tcomb_counter[j]--;
        }
        ca->P[j][c]++;
      }
    }
  }

  size_t covered = 0;
  for (size_t i = 0; i < R; i++) {
    for (size_t j = 0; j < C; j++) {
      if (ca->P[i][j] > 0) {
        covered++;
      }
    }
  }

  ca->covered = covered;
  ca->total = R * C;
  int valid = (covered == ca->total);

  free_matrix(IToC, (int)R);

  return valid;
}

/*
 * Loads a covering array from a file.
 * 
 * File format:
 *   - Optional comment lines starting with 'C' or 'c'.
 *   - Header: N k v ^ k t (e.g., "4 3 2 ^ 3 2")
 *   - N rows of k integers each.
 * 
 * Returns a new covering_array_t, or NULL on failure.
 */
covering_array_t *ca_load(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error: cannot open file '%s'\n", filename);
    return NULL;
  }

  char line[1024];
  long pointer = ftell(fp);
  while (fgets(line, sizeof(line), fp) != NULL) {
    if (line[0] != 'C' && line[0] != 'c') {
      break;
    }
    pointer = ftell(fp);
  }
  fseek(fp, pointer, SEEK_SET);

  int N, k, v, t;
  if (fscanf(fp, "%d %d %d ^ %d %d", &N, &k, &v, &k, &t) != 5) {
    fprintf(stderr, "Error: invalid file format\n");
    fclose(fp);
    return NULL;
  }

  covering_array_t *ca = ca_create(N, k, v, t);
  if (ca == NULL) {
    fclose(fp);
    return NULL;
  }

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < k; j++) {
      if (fscanf(fp, "%d", &ca->matrix[i][j]) != 1) {
        fprintf(stderr, "Error: failed to read matrix element at (%d,%d)\n", i,
                j);
        ca_destroy(ca);
        fclose(fp);
        return NULL;
      }
    }
  }

  fclose(fp);
  return ca;
}

/*
 * Saves a covering array to a file in the output folder.
 * 
 * File naming: N{N}k{k}v{v}^{k}t{t}[.ca|.ca.missing{missing}]
 * The .missing suffix is added if missing > 0.
 * 
 * Returns 0 on success, -1 on failure.
 */
int ca_save(const char *folder_path, covering_array_t *ca, const char *comment,
            int missing) {
  if (ca == NULL || ca->matrix == NULL) {
    return -1;
  }

  struct stat st;
  if (stat(folder_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
    fprintf(stderr, "Error: folder '%s' does not exist\n", folder_path);
    return -1;
  }

  char filename[1024];
  if (missing > 0) {
    snprintf(filename, sizeof(filename), "%s/N%dk%dv%d^%dt%d.ca.missing%d",
             folder_path, ca->N, ca->k, ca->v, ca->k, ca->t, missing);
  } else {
    snprintf(filename, sizeof(filename), "%s/N%dk%dv%d^%dt%d.ca", folder_path,
             ca->N, ca->k, ca->v, ca->k, ca->t);
  }

  FILE *fp = fopen(filename, "w");
  if (fp == NULL) {
    fprintf(stderr, "Error: cannot create file '%s'\n", filename);
    return -1;
  }

  if (comment != NULL && comment[0] != '\0') {
    fprintf(fp, "C %s\n", comment);
  } else {
    fprintf(fp, "C Generated by covering-array library\n");
  }

  fprintf(fp, "%d %d %d ^ %d %d\n", ca->N, ca->k, ca->v, ca->k, ca->t);

  for (int i = 0; i < ca->N; i++) {
    for (int j = 0; j < ca->k; j++) {
      fprintf(fp, "%d ", ca->matrix[i][j]);
    }
    fprintf(fp, "\n");
  }

  fclose(fp);
  return 0;
}

/*
 * Prints the covering array parameters and matrix to stdout.
 * Format: CA(N; t, k, v) followed by N rows of k integers.
 */
void ca_print(covering_array_t *ca) {
  printf("CA(%d; %d, %d, %d)\n", ca->N, ca->t, ca->k, ca->v);
  printf("Matrix:\n");
  for (int i = 0; i < ca->N; i++) {
    for (int j = 0; j < ca->k; j++) {
      printf("%d ", ca->matrix[i][j]);
    }
    printf("\n");
  }
}

/*
 * Appends a row to the covering array matrix.
 * Allocates additional rows if at capacity.
 * Does NOT update the coverage matrix P.
 * 
 * Returns 0 on success, -1 on failure.
 */
int ca_add_row(covering_array_t *ca, const int *row) {
  if (ca == NULL || ca->matrix == NULL) {
    return -1;
  }

  if (ca->N >= ca->capacity) {
    ca->capacity += CA_GROWTH_FACTOR;
    int **new_matrix = realloc(ca->matrix, ca->capacity * sizeof(int *));
    if (new_matrix == NULL) {
      return -1;
    }
    ca->matrix = new_matrix;
  }

  ca->matrix[ca->N] = malloc(ca->k * sizeof(int));
  if (ca->matrix[ca->N] == NULL) {
    return -1;
  }

  for (int j = 0; j < ca->k; j++) {
    ca->matrix[ca->N][j] = row[j];
  }

  ca->N++;
  return 0;
}

/*
 * Adds coverage for a single row to the P matrix.
 * 
 * Requires ca->P and ca->tcomb_counter to be already allocated.
 * Increments P[j][c] for each t-combination j that the row covers.
 * Updates covered count and tcomb_counter when new combos are covered.
 * 
 * Does NOT modify ca->matrix or ca->N.
 * Use after pv_validate() or ca_validate() has initialized P.
 * 
 * Returns 0 on success, -1 if P or tcomb_counter is NULL.
 */
int ca_add_row_coverage(covering_array_t *ca, const int *row) {
  if (ca == NULL || row == NULL) {
    return -1;
  }

  size_t R = binomial(ca->k, ca->t);

  if (ca->P == NULL) {
    return -1;
  }

  if (ca->tcomb_counter == NULL) {
    return -1;
  }

  int **IToC = get_matrix((int)R, ca->t);
  t_wise(IToC, ca->k, ca->t);

  for (size_t j = 0; j < R; j++) {
    int c = get_col(row, IToC, (int)j, ca->t, ca->v);
    if (c != -1) {
      if (ca->P[j][c] == 0) {
        ca->covered++;
        ca->tcomb_counter[j]--;
      }
      ca->P[j][c]++;
    }
  }

  free_matrix(IToC, (int)R);

  return 0;
}

/*
 * Fills the matrix with uniformly random values [0, v-1].
 */
int ca_init_random(covering_array_t *ca) {
  if (ca == NULL || ca->matrix == NULL) {
    return -1;
  }

  for (int i = 0; i < ca->N; i++) {
    for (int j = 0; j < ca->k; j++) {
      ca->matrix[i][j] = rand() % ca->v;
    }
  }

  return 0;
}

/*
 * Fills each row with balanced symbol distribution.
 * Uses ca_fill_balanced_row() for each row.
 */
int ca_init_random_balanced(covering_array_t *ca) {
  if (ca == NULL || ca->matrix == NULL) {
    return -1;
  }

  for (int i = 0; i < ca->N; i++) {
    if (ca_fill_balanced_row(ca->matrix[i], ca->k, ca->v) != 0) {
      return -1;
    }
  }

  return 0;
}

/*
 * Initializes using position rotation with random first row.
 * 
 * First row is random; each subsequent row is the previous row
 * shifted right by one position (with wraparound).
 * Requires N == k.
 */
int ca_init_rotation_position(covering_array_t *ca) {
  if (ca == NULL || ca->matrix == NULL) {
    return -1;
  }

  if (ca->N != ca->k) {
    fprintf(stderr, "Error: N (%d) must equal k (%d) for position rotation\n",
            ca->N, ca->k);
    return -1;
  }

  for (int j = 0; j < ca->k; j++) {
    ca->matrix[0][j] = rand() % ca->v;
  }

  for (int i = 1; i < ca->N; i++) {
    for (int j = 0; j < ca->k; j++) {
      ca->matrix[i][j] = ca->matrix[0][(j - i + ca->k) % ca->k];
    }
  }

  return 0;
}

/*
 * Position rotation with balanced first row.
 * Same as ca_init_rotation_position() but first row is balanced.
 */
int ca_init_rotation_position_balanced(covering_array_t *ca) {
  if (ca == NULL || ca->matrix == NULL) {
    return -1;
  }

  if (ca->N != ca->k) {
    fprintf(stderr, "Error: N (%d) must equal k (%d) for position rotation\n",
            ca->N, ca->k);
    return -1;
  }

  if (ca_fill_balanced_row(ca->matrix[0], ca->k, ca->v) != 0) {
    return -1;
  }

  for (int i = 1; i < ca->N; i++) {
    for (int j = 0; j < ca->k; j++) {
      ca->matrix[i][j] = ca->matrix[0][(j - i + ca->k) % ca->k];
    }
  }

  return 0;
}

/*
 * Full rotation: each row shifts previous row and adds row index to each position.
 * 
 * matrix[i][j] = (matrix[0][(j-i)%k] + i) % v
 * This creates a Latin square pattern.
 * Requires N == k.
 */
int ca_init_rotation_full(covering_array_t *ca) {
  if (ca == NULL || ca->matrix == NULL) {
    return -1;
  }

  if (ca->N != ca->k) {
    fprintf(stderr, "Error: N (%d) must equal k (%d) for full rotation\n",
            ca->N, ca->k);
    return -1;
  }

  for (int j = 0; j < ca->k; j++) {
    ca->matrix[0][j] = rand() % ca->v;
  }

  for (int i = 1; i < ca->N; i++) {
    for (int j = 0; j < ca->k; j++) {
      int shifted = ca->matrix[0][(j - i + ca->k) % ca->k];
      ca->matrix[i][j] = (shifted + i) % ca->v;
    }
  }

  return 0;
}

/*
 * Full rotation with balanced first row.
 * Same as ca_init_rotation_full() but first row is balanced.
 */
int ca_init_rotation_full_balanced(covering_array_t *ca) {
  if (ca == NULL || ca->matrix == NULL) {
    return -1;
  }

  if (ca->N != ca->k) {
    fprintf(stderr, "Error: N (%d) must equal k (%d) for full rotation\n",
            ca->N, ca->k);
    return -1;
  }

  if (ca_fill_balanced_row(ca->matrix[0], ca->k, ca->v) != 0) {
    return -1;
  }

  for (int i = 1; i < ca->N; i++) {
    for (int j = 0; j < ca->k; j++) {
      int shifted = ca->matrix[0][(j - i + ca->k) % ca->k];
      ca->matrix[i][j] = (shifted + i) % ca->v;
    }
  }

  return 0;
}
