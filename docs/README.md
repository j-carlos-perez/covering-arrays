# Library guide

How the pieces in `lib/` fit together, so you can assemble new covering-array
algorithms out of them instead of re-deriving the wiring each time.

Per-function contracts live in the headers (`lib/*.h`). This document covers
what spans them: the index scheme, the required call order, who owns what, and
which invariants you can build on.

---

## 1. The model

A covering array `CA(N; t, k, v)` is an `N x k` matrix over the alphabet
`[0, v-1]` in which every choice of `t` columns contains all `v^t` symbol
tuples somewhere in its rows.

`covering_array_t` (`lib/covering_array.h`) holds the matrix plus the
bookkeeping that answers "what is still missing?":

| Field | Meaning |
| --- | --- |
| `matrix[row][col]` | the array itself, `N x k` |
| `P[j][c]` | how many rows cover tuple `c` of column-set `j` |
| `tcomb_counter[j]` | how many of the `v^t` tuples of column-set `j` are still uncovered |
| `covered` / `total` | `(j,c)` pairs with `P[j][c] > 0`, out of `C(k,t) * v^t` |

After successful validation (`total > 0`), the array is a valid covering array
exactly when `covered == total`.

`P` stores **counts, not flags**. That is what makes incremental search
possible: when a move stops a row from covering a tuple, the count drops by one
and the combination only becomes uncovered if it hits zero.

---

## 2. The two index spaces

This is the part that makes everything else legible. Two integers appear all
over the API as bare `int` / `uint16_t`, and knowing which is which is most of
the battle.

### Column-set index `j`, in `[0, C(k,t))`

Names **which `t` of the `k` columns**. The correspondence is defined by
`t_wise()`, which enumerates combinations in **strict lexicographic ascending
order**. For `k=4, t=2`:

| `j` | columns |
| --- | --- |
| 0 | {0,1} |
| 1 | {0,2} |
| 2 | {0,3} |
| 3 | {1,2} |
| 4 | {1,3} |
| 5 | {2,3} |

This order is a **stable contract**, not an implementation detail — it is
pinned by `test_t_wise_generates_all_combinations` in
`unittests/test_combinatorial.c`. The same `j` indexes all of:

- `IToC[j]` — the `t` column indices themselves
- `ca->P[j]` — that column-set's coverage row
- `ca->tcomb_counter[j]` — its uncovered count
- `precompute_get_affected(pre, j)` — treating `j` as a *change-set*
- the **values** inside `precompute_get_col_affected(pre, col)`

### Tuple index `c`, in `[0, v^t)`

Names **which assignment of symbols** to those `t` columns. `get_col()` encodes
it as a base-`v` number with the first column most significant:

```
c = s[0]*v^(t-1) + s[1]*v^(t-2) + ... + s[t-1]
```

`inv_ruffini()` decodes it back, digits in the same order. The symbol value `v`
(one past the alphabet) is a wildcard meaning "unassigned"; a tuple containing
one has no encoding and `get_col()` returns `-1`.

### Worked example

Take `k=3, t=3, v=3`, so there is a single column-set `j=0` = {0,1,2} and
`v^t = 27` tuples. For the row `[1, 2, 0]`:

| step | value |
| --- | --- |
| symbols at `IToC[0]` = {0,1,2} | `1, 2, 0` |
| `get_col(row, IToC, 0, 3, 3)` | `1*9 + 2*3 + 0` = **15** |
| coverage cell touched | `P[0][15]` |
| `inv_ruffini(V, 15, 3, 3)` | `V = {1, 2, 0}` — back where we started |

---

## 3. Lifecycle

Each step exists for a reason; skipping one is usually silent rather than loud.

1. **Create or load** — `ca_create(N, k, v, t)` or `ca_load(path)`.
   Screen user-supplied parameters with `ca_params_valid()` first if you want
   to report *which* limit was hit.

2. **Fill the matrix** — one of `ca_init_*`, or write `ca->matrix` directly.

3. **Validate** — `ca_validate(ca)` or `pv_validate(ca)`.
   **Required.** This allocates `P` and `tcomb_counter` and fills them. The
   delta layer checks for valid coverage state and returns `0` when it is
   missing, so skipping this produces a search that appears to run and achieves
   nothing.
   Re-validate after any direct write to `ca->matrix`.

4. **Build the search tables** — only if you are using the delta layer:
   - `precompute_create(k, t)` — **must** use the same `k` and `t` as the array.
     Delta calls reject mismatched tables with a `0` no-op result.
   - your own `IToC`: `get_matrix(binomial(k,t), t)` then `t_wise(IToC, k, t)`,
     or `generate_t_combinations(k, t, &n)` which does both.

5. **Search** — `ca_compute_*_delta()` to score candidates without touching
   anything, `ca_apply_*_change()` to commit. See §5 for what stays true.

6. **Save** — `ca_save(dir, ca, comment)`. The filename encodes the missing
   count from the struct, so validate before saving or the name will describe a
   stale state.

7. **Destroy** — `ca_destroy(ca)`, plus your own `precompute_destroy(pre)` and
   `free_matrix(IToC, R)`.

---

## 4. Ownership

| Owned by the library, freed by `ca_destroy()` | Owned by you |
| --- | --- |
| `ca->matrix` | `IToC` — free with `free_matrix(IToC, C(k,t))` |
| `ca->P` | the `ca_affected_t` — free with `precompute_destroy()` |
| `ca->tcomb_counter` | any `Set64` — free with `set64_free()` |

Matrix frees take the **row count** and nothing records it for you, so pass the
same value you allocated with.

Pointers returned by `precompute_get_affected()` and
`precompute_get_col_affected()` point *into* the `ca_affected_t`. They are
`const`, they are not yours to free, and they dangle after
`precompute_destroy()`.

---

## 5. Invariants you can build on

**After any valid `ca_apply_cell_change()` or `ca_apply_tcolumns_change()`, the
coverage state is exactly what a fresh `ca_validate()` on the same matrix would
produce** — `P`, `covered` and `tcomb_counter` all agree. Enforced by
`test_apply_cell_change_keeps_p_in_sync_over_many_moves` in
`unittests/test_regression.c`, which walks 60 random moves and compares against
a full recompute.

**A returned delta of `0` means "no net coverage change", not "nothing
happened".** The move is always applied unless the new value already equals the
current one. A move can cover one combination while uncovering another, or
shift counts between tuples that stay covered either way — all of those score
zero and all of them change the array. This matters for any search that accepts
plateau moves, which is most of them.

**`ca_validate()` and `pv_validate()` are idempotent and agree with each
other.** Calling either twice gives the same answer as calling it once
(`test_pv_validate_is_idempotent`, `test_pv_validate_agrees_with_ca_validate`).

**`t_wise()` ordering is stable** — see §2.

---

## 6. Limits

| Limit | Value | Enforced by |
| --- | --- | --- |
| Rows | `N <= 65535` (`CA_COUNT_MAX`) | `ca_params_valid()`, `ca_add_row()` |
| Alphabet | `v >= 2` | `ca_params_valid()` |
| Strength | `1 <= t <= k` | `ca_params_valid()`, `t_wise()` |
| Tuple indices | `v^t <= INT_MAX` | `ca_params_valid()`, checked by `get_col()` |
| Materialised/visited column-sets | `C(k,t) <= INT_MAX` | `ca_params_valid()`, `t_wise*()` |
| Column-sets, for `precompute` | `C(k,t) <= 65535` | `precompute_create()` — indices are `uint16_t` |
| Precompute tables | 16 GB | `precompute_create()` |
| `binomial()` | signals `BINOMIAL_OVERFLOW` | screen with `binomial_is_usable()` |

The row cap follows from `P` holding `ca_count_t` (`uint16_t`) counts: a
coverage count can be as large as `N`, so `N` cannot exceed what the counter
holds.

---

## 7. Return conventions

They are **not uniform**. Check this table rather than guessing.

| Convention | Functions |
| --- | --- |
| Pointer, `NULL` on failure | `ca_create`, `ca_load`, `precompute_create`, `set64_create`, `generate_t_combinations`, `get_matrix*`, `get_vector*`, `precompute_get_*_affected` |
| `0` success / `-1` failure | `ca_save`, `ca_add_row`, `ca_add_row_coverage`, `ca_init_*`, `t_wise`, `inv_ruffini`, `pd_evaluate_seed`, `pd_generate_balanced_seed` |
| `1` good / `0` bad — **inverted** | `ca_validate` (1 = fully covering), `ca_params_valid` (1 = usable), `binomial_is_usable` |
| `1` more / `0` exhausted | `next_permutation`, `next_gray_code` |
| Value with `-1` sentinel | `get_col` (`-1` = wildcard, invalid input, or unrepresentable tuple) |
| Value with a sentinel constant | `binomial` (`0` out of range, `BINOMIAL_OVERFLOW` too large) |
| Count, `0` if invalid | `t_wise_visit` |
| Signed delta, `0` = no net change | `ca_compute_*_delta`, `ca_apply_*_change` |
| `bool` | `set64_delete` |
| `void` | `pv_validate` (clears `total` on failure), `set64_insert` (silent no-op on bad input/allocation failure) |

The last row is the trap. `pv_validate()` reports nothing directly — it clears
`total` before starting, so `total == 0` means validation failed. Otherwise,
compare `covered` to `total` for the verdict `ca_validate()` returns directly.

---

## 8. Thread safety

Everything is reentrant **except**:

| Not reentrant | Why |
| --- | --- |
| `rand_below`, `shuffle`, `ca_init_*`, `pd_generate_balanced_seed` | global `rand()` sequence |
| `init_gray_code`, `next_gray_code` | file-scope direction state; only one walk per process |
| `set64_random` | file-scope PRNG state |
| `pv_validate` | internally OpenMP-parallel — never call it concurrently on the same array, and do not read the array's coverage fields while it runs |

---

## 9. Module map

| Header | What it is for | Reach for it instead of |
| --- | --- | --- |
| `covering_array.h` | The array type, its coverage state, I/O, and whole-matrix initialisers. | — the entry point. |
| `combinatorial.h` | Column-set enumeration (`t_wise`), tuple encode/decode (`get_col`, `inv_ruffini`), counting (`binomial`), permutation and Gray-code iterators. | hand-rolling nested loops over column subsets. |
| `precompute.h` | "Which column-sets does this change touch?", computed once. | rescanning all `C(k,t)` column-sets per move. |
| `local_calculation.h` | Score and apply a **single-cell** change incrementally. | calling `ca_validate()` inside a search loop. |
| `t_columns_delta.h` | Score and apply a **whole `t`-column** change at once — the move that can fix a specific missing combination outright. | `t` separate single-cell moves, which cannot see the combined effect. |
| `parallel_validator.h` | Full revalidation across threads. | `ca_validate()` on large arrays, when you have OpenMP linked. |
| `pair_diversity.h` | Find a good seed row for the rotation initialisers. | a random first row, when the whole array is built by rotating it. |
| `set64.h` | A set you can sample uniformly in O(1). | scanning `P` to pick a random uncovered combination. |
| `memory.h` | Matrix/vector allocation, and the `ca_count_t` coverage counter type. | raw `malloc` for the row-of-pointers layout. |

---

## 10. Recipes

Three worked programs under `examples/`, built by `make examples`. They are
real code and compile against the current headers, so they cannot drift.

| Program | Shows |
| --- | --- |
| `examples/recipe_enumerate.c` | The pure combinatorics, no array involved: `t_wise_visit`, `get_col`/`inv_ruffini` round-trips, `binomial` overflow screening, permutations, Gray codes. Start here to see §2 concretely. |
| `examples/recipe_greedy_rows.c` | Building an array from nothing: generate candidate rows, score each with `ca_add_row_coverage` bookkeeping, keep the best, and use `tcomb_counter` to steer toward the worst-covered column-sets. |
| `examples/recipe_hill_climb.c` | Local search over cells: `precompute` + `IToC`, `ca_compute_cell_delta` to score, `ca_apply_cell_change` to commit, a `Set64` holding the uncovered `(j,c)` pairs for random target selection, plateau moves, and a final check that the incremental state matches a full recompute. |

Run them with small parameters to watch the state evolve:

```sh
make examples
./examples/recipe_enumerate 4 2 3
./examples/recipe_greedy_rows 2 6 3 ./output_test
./examples/recipe_hill_climb 12 2 6 3
```
