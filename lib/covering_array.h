#ifndef COVERING_ARRAY_H
#define COVERING_ARRAY_H

#include "combinatorial.h"
#include "memory.h"

/*
 * A covering array CA(N; t, k, v) together with its coverage state.
 *
 * The array itself is the N x k matrix. Everything else records which t-way
 * combinations that matrix covers, indexed by the scheme described in
 * docs/README.md and summarised here:
 *
 *   j  is a COLUMN-SET index in [0, C(k,t)). It names t of the k columns, in
 *      the lexicographic order that t_wise() produces. The same j indexes P,
 *      tcomb_counter, and the change-set arguments in t_columns_delta.h.
 *
 *   c  is a TUPLE index in [0, v^t). It names one assignment of symbols to
 *      those t columns, encoded by get_col() as a base-v number with the
 *      first column most significant. inv_ruffini() decodes it.
 *
 * So "column-set j is covered for tuple c" is exactly P[j][c] > 0, and the
 * array is a valid covering array when that holds for every (j, c).
 */
typedef struct covering_array {
  /* Set at creation, then stable except N, which ca_add_row() increases. */
  int N;        /* rows in use */
  int capacity; /* rows allocated in matrix; >= N */
  int k;        /* columns */
  int v;        /* alphabet size; symbols are [0, v-1], and v itself is the
                   wildcard marker that get_col() reports as -1 */
  int t;        /* strength */

  /* matrix[row][col], row < N, col < k. Owned by the array; ca_destroy()
     frees it. Writing here directly invalidates everything below until the
     next ca_validate() or pv_validate(). */
  int **matrix;

  /* ---- Coverage state. NULL until the first validate; see ca_validate(). ---- */

  /* P[j][c] = how many rows cover tuple c of column-set j. Dimensions are
     C(k,t) x v^t. Counts, not flags: a row that stops covering a tuple
     decrements rather than clearing, which is what makes the incremental
     delta layer possible. Capped by CA_COUNT_MAX (see memory.h), which is why
     N is limited to the same bound. */
  ca_count_t **P;

  /* tcomb_counter[j] = how many of the v^t tuples of column-set j are still
     uncovered. Zero means column-set j is fully covered. Useful for steering
     a search toward the worst column-sets without scanning P. */
  size_t *tcomb_counter;

  size_t covered; /* (j, c) pairs with P[j][c] > 0 */
  size_t total;   /* C(k,t) * v^t; covered == total means fully covering */
} covering_array_t;

/*
 * Reports whether these parameters describe a usable array.
 *
 * Checks N >= 1, k >= 1, v >= 2, 1 <= t <= k, N <= CA_COUNT_MAX (a coverage
 * counter must be able to hold the row count), and that C(k,t), v^t and their
 * product are representable and allocatable. Because column-set and tuple
 * indices are ints, both C(k,t) and v^t must be <= INT_MAX.
 *
 * Call this before ca_create() when the parameters come from user input, so
 * you can report the specific problem; ca_create() applies the same test but
 * only tells you it failed.
 *
 * Returns: 1 if usable, 0 if not. (Note the polarity: 1 is the good case.)
 */
int ca_params_valid(int N, int k, int v, int t);

/*
 * Allocates an array with N rows and k columns.
 *
 * The matrix contents are uninitialised -- fill them with one of the ca_init_*
 * functions or by writing ca->matrix directly. P and tcomb_counter start NULL
 * and are allocated by the first validate.
 *
 * Ownership: caller must release with ca_destroy().
 * Returns:   the array, or NULL if the parameters fail ca_params_valid() or
 *            allocation fails.
 */
covering_array_t *ca_create(int N, int k, int v, int t);

/*
 * Releases the array and everything it owns: matrix, P and tcomb_counter.
 *
 * Does NOT free anything the caller built alongside it -- an IToC table or a
 * ca_affected_t are yours to free. Accepts NULL.
 */
void ca_destroy(covering_array_t *ca);

/*
 * Recomputes the entire coverage state from the matrix, single-threaded.
 *
 * Allocates P and tcomb_counter on first call, then fills P, tcomb_counter,
 * covered and total from scratch. Idempotent: calling it twice gives the same
 * answer as calling it once, and it is the definition the incremental delta
 * layer is checked against.
 *
 * Call this after any direct write to ca->matrix, and at least once before
 * using anything in local_calculation.h or t_columns_delta.h -- those need P
 * to exist and silently do nothing when it is NULL.
 *
 * Returns: 1 if the array is fully covering (covered == total), 0 otherwise
 *          OR on failure. covered and total are cleared before work begins, so
 *          total == 0 distinguishes failure from a successfully checked,
 *          incomplete array.
 * Cost:    N * C(k,t) get_col() evaluations, plus one C(k,t) x t allocation
 *          for its internal column-set table.
 */
int ca_validate(covering_array_t *ca);

/*
 * Reads an array from a .ca file.
 *
 * Format: optional comment lines beginning with 'C' or 'c', then a header
 * "N k v ^ k t" (k appears twice and the two must agree), then N rows of k
 * integers.
 *
 * Every field is validated: the parameters must pass ca_params_valid() and
 * every symbol must lie in [0, v] (v itself being the wildcard). A file that
 * fails any check is rejected with a message on stderr rather than loaded --
 * an out-of-range symbol would otherwise encode past the end of a P row.
 *
 * The returned array has no coverage state yet; call ca_validate() or
 * pv_validate() before using it.
 *
 * Ownership: caller must release with ca_destroy().
 * Returns:   the array, or NULL if the file cannot be opened, parsed, or
 *            validated.
 */
covering_array_t *ca_load(const char *filename);

/*
 * Writes the array into an existing directory.
 *
 * The file is named from the parameters and the current coverage state:
 * "N{N}k{k}v{v}^{k}t{t}.ca" when nothing is missing, or
 * "...ca.missing{n}" when total - covered is n > 0. That count comes from the
 * struct, so validate before saving or the name will describe a stale state.
 *
 * comment is written as a leading "C " line; pass NULL or "" for a default.
 *
 * Returns: 0 on success, -1 if an argument is invalid, folder_path is not an
 *          existing directory, the generated path is too long, or any create,
 *          write, flush, or close operation fails.
 */
int ca_save(const char *folder_path, covering_array_t *ca, const char *comment);

/*
 * Prints "CA(N; t, k, v)" and the full matrix to stdout. Diagnostics only.
 */
void ca_print(covering_array_t *ca);

/*
 * Appends a row, growing the matrix if needed. Does NOT touch coverage state.
 *
 * Pair it with ca_add_row_coverage() to keep P current, or re-validate
 * afterwards. Splitting the two lets you evaluate a candidate row's coverage
 * before committing to storing it.
 *
 * row must hold k symbols in [0, v-1]; the function validates them.
 *
 * Returns: 0 on success, -1 if an argument or symbol is invalid, N has reached
 *          CA_COUNT_MAX, or allocation fails (the array is left untouched).
 * Cost:    amortised O(k); pointer capacity grows geometrically.
 */
int ca_add_row(covering_array_t *ca, const int *row);

/*
 * Folds one row's contribution into P, covered and tcomb_counter.
 *
 * Independent of ca_add_row(): this only updates coverage bookkeeping and
 * never reads or writes ca->matrix or ca->N. Call it with the same row you
 * passed to ca_add_row() to keep the two in step.
 *
 * Preconditions: valid P/tcomb_counter/total state must exist -- validate
 *                first. Symbols may be in [0,v], where v is the wildcard.
 * Returns:       0 on success, -1 if ca or row is NULL, the coverage state has
 *                not been allocated, parameters/symbols are unusable, an
 *                allocation fails, or a coverage counter would overflow.
 * Cost:          C(k,t) get_col() evaluations, plus one C(k,t) x t allocation
 *                for its internal column-set table -- so prefer re-validating
 *                once over calling this in a tight loop over many rows.
 */
int ca_add_row_coverage(covering_array_t *ca, const int *row);

/*
 * ---- Matrix initialisers -------------------------------------------------
 *
 * Each fills ca->matrix and returns 0 on success, -1 on failure. None of them
 * update coverage state; validate afterwards.
 *
 * The "balanced" variants give every symbol either floor(k/v) or ceil(k/v)
 * positions per row instead of drawing each cell independently, which keeps a
 * row from being dominated by one symbol.
 *
 * The rotation initialisers build every row from row 0 and REQUIRE N == k;
 * they fail with a message on stderr otherwise. "position" rotates row 0 right
 * by the row index; "full" also adds the row index to each symbol modulo v,
 * giving a cyclic position-and-symbol pattern (not generally a Latin square).
 * Per sweep_summary.md these do well for v=2
 * and poorly for v >= 3, where random initialisation is the better default.
 *
 * All of these draw from the global rand() sequence; call srand() first.
 */
int ca_init_random(covering_array_t *ca);
int ca_init_random_balanced(covering_array_t *ca);
int ca_init_rotation_position(covering_array_t *ca);
int ca_init_rotation_position_balanced(covering_array_t *ca);
int ca_init_rotation_full(covering_array_t *ca);
int ca_init_rotation_full_balanced(covering_array_t *ca);

#endif
