#include "lib/combinatorial.h"
#include "lib/covering_array.h"
#include "lib/memory.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char PATH[100], ARCHIVE[100];
int ROWIN, ROWOUT, EXCLUDE;
#define line_size 1024
char cadena[line_size];

int N, t, k, v;
int **pCA;
int R, C;
int **P;

int **IToR, **IToC;

int covered = 0, nocovered;
int **M;
int KC, NC;
int **current, **best;

void readme(char const *name) {
  fprintf(stderr,
          "USAGE: %s -PATH 'out_folder' -FILE 'partialCA' -ROWIN 'int > 0' "
          "-ROWOUT 'int > 0' [-EXCLUDE 'int']\n",
          name);
}

int parsing(int argc, char const *argv[]) {
  if (argc % 2 == 0) {
    return 0;
  }
  ROWIN = 0;
  ROWOUT = 0;
  EXCLUDE = 0;
  sprintf(PATH, "");
  sprintf(ARCHIVE, "");
  while (argc > 1) {
    if (strcmp(argv[argc - 2], "-PATH") == 0) {
      sprintf(PATH, "%s", argv[argc - 1]);
    } else if (strcmp(argv[argc - 2], "-FILE") == 0) {
      sprintf(ARCHIVE, "%s", argv[argc - 1]);
    } else if (strcmp(argv[argc - 2], "-ROWIN") == 0) {
      ROWIN = atoi(argv[argc - 1]);
    } else if (strcmp(argv[argc - 2], "-ROWOUT") == 0) {
      ROWOUT = atoi(argv[argc - 1]);
    } else if (strcmp(argv[argc - 2], "-EXCLUDE") == 0) {
      EXCLUDE = atoi(argv[argc - 1]);
    } else {
      fprintf(stderr, "bad parameter %s\n", argv[argc - 2]);
      return 0;
    }
    argc -= 2;
  }
  if (ROWOUT == 0 || ROWIN == 0) {
    fprintf(stderr, "Missing rows\n");
    return 0;
  }

  if (strcmp(PATH, "") == 0) {
    fprintf(stderr, "Missing path\n");
    return 0;
  } else if (strcmp(ARCHIVE, "") == 0) {
    fprintf(stderr, "Missing file\n");
    return 0;
  }
  return 1;
}

int evaluate(int selRow, int nocovered) {
  int i, j, h, c, delta = 0;
  int alph[t];
  for (i = 0; i < ROWIN; ++i) {

    for (j = 0; j < KC; ++j) {
      c = get_col(pCA[IToR[selRow][i] + EXCLUDE], IToC, j, t, v);
      if (c != -1) {
        if (P[j][c] == 1) {
          delta++;
          M[nocovered][0] = j;
          M[nocovered][1] = c;
          nocovered++;
        }
        P[j][c]--;
      }
    }
  }

  fprintf(stderr, "\nDelta:%d Nocover:%d\n", delta, nocovered);

  int idx[nocovered];
  int flag = 0;
  shuffle(idx, nocovered);

  for (i = 0; i < ROWOUT; ++i) {
    for (j = 0; j < k; ++j) {
      current[i][j] = v;
    }
  }

  fprintf(stderr, "Target Fill\n");
  for (i = 0; i < nocovered; ++i) {
    fprintf(stderr, "[%d %d]\n", M[idx[i]][0], M[idx[i]][1]);
  }

  int new = 0;
  for (i = 0; i < nocovered; ++i) {
    for (j = 0; j < ROWOUT; ++j) {
      inv_ruffini(alph, M[idx[i]][1], v, t);
      flag = 1;
      for (h = 0; h < t; ++h) {
        if (current[j][IToC[M[idx[i]][0]][h]] != alph[h] &&
            current[j][IToC[M[idx[i]][0]][h]] != v) {
          flag = 0;
          break;
        }
      }
      if (flag == 1) {
        for (h = 0; h < t; ++h) {
          current[j][IToC[M[idx[i]][0]][h]] = alph[h];
        }
        new ++;
        fprintf(stderr, "%d setted in %d \n", i, j);
        break;
      }
    }
  }

  fprintf(stderr, "%d / %d\n", new, nocovered);

  for (i = 0; i < ROWIN; ++i) {
    for (j = 0; j < KC; ++j) {
      c = get_col(pCA[IToR[selRow][i] + EXCLUDE], IToC, j, t, v);
      if (c != -1) {
        if (P[j][c] == 0) {
          delta--;
        }
        P[j][c]++;
      }
    }
  }

  return nocovered - new;
}

void copy_matrix(int **dst, int **src) {
  for (int i = 0; i < ROWOUT; ++i) {
    for (int j = 0; j < k; ++j) {
      dst[i][j] = src[i][j];
    }
  }
}

int main(int argc, char const *argv[]) {
  int i, j, r, c, min, selected;
  srand(time(NULL));
  if (!parsing(argc, argv)) {
    readme(argv[0]);
    exit(-1);
  }

  covering_array_t *ca = ca_load(ARCHIVE);
  if (ca == NULL) {
    fprintf(stderr, "Error reading '%s' file\n", ARCHIVE);
    exit(-1);
  }

  N = ca->N;
  k = ca->k;
  v = ca->v;
  t = ca->t;
  pCA = ca->matrix;

  fprintf(stderr, "CA(%d;%d,%d,%d)\n", N, t, k, v);

  R = binomial(k, t);
  C = pow(v, t);
  P = get_matrix(R, C);
  NC = binomial(N - EXCLUDE, ROWIN);
  KC = R;
  IToC = get_matrix(KC, t);
  IToR = get_matrix(NC, ROWIN);

  t_wise(IToC, k, t);
  t_wise(IToR, N - EXCLUDE, ROWIN);

  for (i = 0; i < R; ++i) {
    for (j = 0; j < C; ++j) {
      P[i][j] = 0;
    }
  }

  for (i = 0; i < N; ++i) {
    for (j = 0; j < KC; ++j) {
      c = get_col(pCA[i], IToC, j, t, v);
      if (c != -1) {
        if (P[j][c] == 0) {
          covered++;
        }
        P[j][c]++;
      }
    }
  }
  fprintf(stderr, "\n%d / %d  covered \n", covered, R * C);
  nocovered = R * C - covered;
  M = get_matrix(nocovered + ROWIN * KC, 2);
  r = 0;
  for (i = 0; i < R; ++i) {
    for (j = 0; j < C; ++j) {
      if (P[i][j] == 0) {
        M[r][0] = i;
        M[r][1] = j;
        r++;
      }
    }
  }

  for (i = 0; i < nocovered; ++i) {
    fprintf(stderr, "%d %d\n", M[i][0], M[i][1]);
  }

  current = get_matrix(ROWOUT, k);
  best = get_matrix(ROWOUT, k);

  min = evaluate(0, nocovered);
  copy_matrix(best, current);
  selected = 0;
  int curr;
  if (min != 0) {
    for (i = 1; i < NC; ++i) {
      curr = evaluate(i, nocovered);
      if (curr < min) {
        min = curr;
        selected = i;
        copy_matrix(best, current);
      }
      fprintf(stderr, "%d\n", curr);
      if (min == 0) {
        break;
      }
    }
  }

  int print[N];
  for (i = 0; i < N; ++i) {
    print[i] = 1;
  }

  for (i = 0; i < ROWIN; ++i) {
    print[IToR[selected][i] + EXCLUDE] = 0;
  }

  N = N + ROWOUT - ROWIN;

  covering_array_t *ca_out = ca_create(N, ca->k, ca->v, ca->t);
  if (ca_out == NULL) {
    fprintf(stderr, "Error: failed to create output covering array\n");
    ca_destroy(ca);
    exit(-1);
  }

  int out_row = 0;
  for (i = 0; i < ca->N; ++i) {
    if (print[i] == 1) {
      for (j = 0; j < ca->k; ++j) {
        ca_out->matrix[out_row][j] = ca->matrix[i][j];
      }
      out_row++;
    }
  }

  for (i = 0; i < ROWOUT; ++i) {
    for (j = 0; j < ca->k; ++j) {
      ca_out->matrix[out_row][j] = best[i][j];
    }
    out_row++;
  }

  int save_result =
      ca_save(PATH, ca_out, "Generado mediante merge ROW greedy", min);
  if (save_result != 0) {
    fprintf(stderr, "Error: failed to save output file\n");
  }

  fprintf(stderr, "minimal is %d using %d gtp\n", min, selected);

  ca_destroy(ca_out);
  ca_destroy(ca);

  free_matrix(current, ROWOUT);
  free_matrix(best, ROWOUT);
  free_matrix(M, nocovered + ROWIN * KC);
  free_matrix(IToR, NC);
  free_matrix(IToC, KC);
  free_matrix(P, R);

  return 0;
}
