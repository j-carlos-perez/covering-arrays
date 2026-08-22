#ifndef COMBINATORIAL_H
#define COMBINATORIAL_H

#include <stdint.h>

/*
 * Combinatorial primitives: column-set enumeration, tuple encoding, counting,
 * and standalone iterators over permutations and Gray codes.
 *
 * t_wise() and get_col() together define the index scheme the whole coverage
 * layer is built on -- see docs/README.md.
 */

/* Returned by binomial() when C(k,r) does not fit in a uint64_t. */
#define BINOMIAL_OVERFLOW UINT64_MAX

/*
 * Uniform random integer in [0, bound), by rejection sampling.
 *
 * `rand() % bound` is biased whenever bound does not divide RAND_MAX+1: the
 * low residues occur once more often than the high ones. That skew works
 * directly against the balanced initialisers, whose whole purpose is to
 * control the symbol distribution.
 *
 * Returns:       a value in [0, bound), or 0 for bound <= 0.
 * Thread safety: NOT reentrant -- draws from the global rand() sequence.
 */
int rand_below(int bound);

/*
 * Fills array[0..n-1] with 0..n-1 in a uniformly random order (Fisher-Yates).
 *
 * Ownership:     writes into caller-provided storage of at least n ints.
 *                No-op for n <= 0 or a NULL array.
 * Thread safety: NOT reentrant -- uses rand_below().
 */
void shuffle(int *array, int n);

/*
 * C(k, r), computed without overflowing an intermediate.
 *
 * The running value has its common factor cancelled before each multiply, so
 * results that fit are always produced: C(63,29) is returned exactly even
 * though the naive product b * (k-i+1) would overflow first.
 *
 * Returns: C(k,r); 0 when k < 0, r < 0, or k < r; BINOMIAL_OVERFLOW when the
 *          true result exceeds a uint64_t. Callers using the result as an
 *          allocation size must screen it with binomial_is_usable(), since
 *          BINOMIAL_OVERFLOW is a valid-looking huge number.
 */
uint64_t binomial(int k, int r);

/*
 * Non-zero if n is a usable count: not the overflow marker, and not larger
 * than the platform can index.
 */
int binomial_is_usable(uint64_t n);

/*
 * Fills GTP with every combination of t columns drawn from k, one per row.
 *
 * The order is strict lexicographic ascending, and it is a stable contract
 * other modules depend on, not an implementation detail -- P rows,
 * tcomb_counter entries and precompute indices are all keyed by position in
 * this enumeration. For k=4, t=2 the rows are:
 *     {0,1} {0,2} {0,3} {1,2} {1,3} {2,3}
 * Within each row the column indices ascend.
 *
 * Preconditions: GTP must have at least C(k,t) rows of t ints -- allocate with
 *                get_matrix(binomial(k,t), t), or use
 *                generate_t_combinations() to do both together.
 * Returns:       0 on success, -1 if GTP is NULL, t < 1, k < t, or C(k,t)
 *                exceeds INT_MAX (column-set indices are ints).
 */
int t_wise(int **GTP, int k, int t);

/*
 * Decodes tuple index num into its t base-v digits, most significant first.
 *
 * Exact inverse of get_col(): feeding get_col()'s result back through this
 * recovers the symbols it encoded, in the same column order.
 *
 * Ownership: writes t ints into caller-provided V.
 * Returns:   0 on success; -1 if V is NULL, t < 1, v < 2, num < 0, or num does
 *            not fit in t digits (num >= v^t). V is zeroed before the range
 *            failure is detected, so its contents are not meaningful on -1.
 */
int inv_ruffini(int *V, int num, int v, int t);

/*
 * Encodes the symbols that row `line` places in column-set j as a tuple index.
 *
 * Reads the t columns listed in IToC[j] and packs them as a base-v number with
 * the FIRST column most significant:
 *     c = s[0]*v^(t-1) + s[1]*v^(t-2) + ... + s[t-1]
 * The result is the c that indexes ca->P[j].
 *
 * The symbol value v (one past the alphabet) is a wildcard meaning "not yet
 * assigned"; a tuple containing one has no encoding. All column indices must
 * be valid for line, and a fully assigned tuple must fit in an int.
 *
 * Returns: the tuple index in [0, v^t), or -1 for a wildcard, an invalid
 *          symbol/argument, or an encoding larger than INT_MAX.
 * Cost:    t array reads and t-1 multiply-adds. This is the innermost
 *          operation of every validate and every delta.
 */
int get_col(const int *line, int **IToC, int j, int t, int v);

/*
 * Allocates a C(k,t) x t table and fills it with t_wise().
 *
 * Convenience wrapper for the common allocate-then-enumerate pair.
 *
 * Ownership: caller must release with free_matrix(result, *out_n).
 * Returns:   the table, or NULL if k, t are out of range, C(k,t) exceeds
 *            INT_MAX, or the table cannot be allocated. out_n receives the
 *            row count, or 0 on failure; it may be NULL.
 */
int **generate_t_combinations(int k, int t, int *out_n);

/*
 * Callback for t_wise_visit(). `combination` holds t column indices and is
 * reused between calls -- copy it if you need it to outlive the call. `index`
 * is the column-set index j, counting from 0 in enumeration order.
 */
typedef void (*t_combination_callback)(int *combination, int index, int k,
                                       int t, void *user_data);

/*
 * Walks every t-column combination, calling cb for each, without ever
 * materialising the table. Use when C(k,t) rows would be wasteful to store.
 *
 * Same enumeration order as t_wise().
 *
 * Returns: the number of combinations visited, or 0 if t < 1, k < t, or the
 *          count exceeds INT_MAX. cb may be NULL, which just counts.
 */
int t_wise_visit(int k, int t, t_combination_callback cb, void *user_data);

/*
 * ---- Permutation iterator ------------------------------------------------
 *
 * Sorts arr ascending, which is the first permutation in lexicographic order.
 * Works on any values, including duplicates. No-op for NULL or n <= 0.
 */
void init_permutation(int *arr, int n);

/*
 * Advances arr to the next lexicographic permutation in place.
 *
 * With duplicate values it visits each DISTINCT arrangement once, so a full
 * walk yields the multinomial count rather than n!.
 *
 * Returns: 1 if arr now holds the next permutation, 0 if it was already the
 *          last one (arr is left descending).
 */
int next_permutation(int *arr, int n);

/*
 * ---- Gray code iterator --------------------------------------------------
 *
 * Enumerates all v^n strings over [0, v-1] so that consecutive strings differ
 * in exactly one position by exactly one step.
 *
 * init_gray_code() zeroes arr AND resets the shared walk state, so it must be
 * called at the start of every sequence -- including after abandoning a
 * previous one part-way.
 *
 * Thread safety: NOT reentrant, and only one walk can be in flight per process
 *                -- the direction state is a file-scope static. NULL or a
 *                non-positive length leaves the iterator exhausted.
 */
void init_gray_code(int *arr, int n);

/*
 * Advances arr to the next Gray code in place.
 *
 * Returns: 1 if arr now holds the next code, 0 when the sequence is exhausted
 *          (after v^n codes) or the arguments are out of range. Once exhausted,
 *          it keeps returning 0 until init_gray_code() starts a new sequence.
 */
int next_gray_code(int *arr, int n, int v);

#endif
