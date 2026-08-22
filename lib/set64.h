#ifndef SET64_H
#define SET64_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * A set of 64-bit keys that can be SAMPLED uniformly at random in O(1).
 *
 * That sampling is the point -- a plain hash set cannot do it. Membership sits
 * in an open-addressed table (Robin Hood probing, backward-shift deletion),
 * while every live key is also held contiguously in `dense`, so picking a
 * random member is one index into `dense`.
 *
 * The intended use in this project is the worklist of still-uncovered
 * combinations during a local search: insert a key per uncovered
 * (column-set j, tuple c) pair, call set64_random() to choose which gap to
 * attack next, and set64_delete() as gaps close. Without it, "pick a random
 * uncovered combination" means scanning ca->P.
 *
 * Packing note: set64_make_key() gives `a` only SET64_A_BITS (15) bits, so the
 * TUPLE index c must be `a` (it is < v^t, typically small) and the COLUMN-SET
 * index j must be `b`. The reverse does not fit -- C(k,t) may reach 65535
 * while `a` saturates at 32767.
 *
 * Thread safety: not reentrant. set64_random() uses a file-scope PRNG state.
 */

/*
 * Nominal load-factor ceiling for the table (70%). In practice growth is
 * triggered by dense[] filling first -- see max_fill in Set64 below -- so the
 * table settles at a 50% load factor and this bound is not reached.
 */
#define SET64_LOAD_FACTOR_NUM 7
#define SET64_LOAD_FACTOR_DEN 10

/*
 * 0 and 1 are reserved so they can never collide with a real key; keys are
 * offset by 2 in set64_make_key(). SET64_DELETED_KEY is retained only as a
 * reserved value -- deletion shifts entries back rather than leaving
 * tombstones, which is what keeps the Robin Hood probe in set64_find() sound.
 */
#define SET64_EMPTY_KEY 0ULL
#define SET64_DELETED_KEY 1ULL

/* Bits of a packed key given to the `a` component; the rest hold `b`. */
#define SET64_A_BITS 15
#define SET64_A_MASK ((1ULL << SET64_A_BITS) - 1)

/* One table slot: a key plus where that key lives in `dense`. */
typedef struct {
  uint64_t key;
  uint32_t index;
} Set64Entry;

/* Treat the fields as read-only; use the functions below to mutate the set. */
typedef struct {
  uint64_t *dense;    /* the live keys, packed into [0, size) */
  Set64Entry *table;  /* open-addressed index over dense */
  uint32_t size;      /* live keys */
  uint32_t cap;       /* slots allocated in dense; always >= size */
  uint32_t tcap;      /* slots in table; a power of two, always 2 * cap */
  uint32_t mask;      /* tcap - 1, for wrapping probes */
  uint32_t max_fill;  /* nominal load-factor bound, 0.7 * tcap. Since
                         tcap == 2 * cap this works out to 1.4 * cap, so the
                         `size >= cap` condition in set64_insert() always
                         trips first and the real growth point is `cap`. */
} Set64;

/*
 * Packs (a, b) into one key. `a` occupies the low SET64_A_BITS bits, so it must
 * be < 2^SET64_A_BITS; it is masked here so an out-of-range value cannot silently
 * corrupt `b`'s bits, as it previously could.
 */
static inline uint64_t set64_make_key(uint32_t a, uint32_t b) {
  uint64_t k = ((uint64_t)b << SET64_A_BITS) | ((uint64_t)a & SET64_A_MASK);
  return k + 2;
}

/* Recovers the `a` component of a key built by set64_make_key(). */
static inline uint32_t set64_key_get_a(uint64_t k) {
  k -= 2;
  return (uint32_t)(k & SET64_A_MASK);
}

/* Recovers the `b` component of a key built by set64_make_key(). */
static inline uint32_t set64_key_get_b(uint64_t k) {
  k -= 2;
  return (uint32_t)(k >> SET64_A_BITS);
}

/*
 * Allocates an empty set sized for roughly initial_capacity keys.
 *
 * The set grows as needed, so this is a hint, not a limit -- but sizing it for
 * the expected peak avoids rehashing.
 *
 * Ownership: caller must release with set64_free().
 * Returns:   the set, or NULL if allocation fails or initial_capacity cannot
 *            be safely rounded to a power of two and doubled.
 */
Set64 *set64_create(uint32_t initial_capacity);

/*
 * Releases the set. Accepts NULL.
 */
void set64_free(Set64 *s);

/*
 * Adds a key. Inserting a key already present does nothing, so `size` counts
 * distinct keys.
 *
 * Keys 0 and 1 are reserved sentinels and are rejected; build keys with
 * set64_make_key(), which offsets past them.
 *
 * Silently does nothing if the set needs to grow and allocation fails. Growth
 * is atomic, so the existing set and all its capacity fields remain intact.
 *
 * Cost: O(1) expected, amortised over the occasional rehash.
 */
void set64_insert(Set64 *s, uint64_t key);

/*
 * Removes a key.
 *
 * Returns: true if the key was present and removed, false if it was absent.
 * Cost:    O(1) expected. Removal shifts a short run of following entries back
 *          rather than leaving a tombstone, which is what keeps lookups exact.
 */
bool set64_delete(Set64 *s, uint64_t key);

/*
 * Returns a uniformly random member, without removing it.
 *
 * Uniform for any size, not just powers of two.
 *
 * Returns:       a key, or SET64_EMPTY_KEY if the set is empty -- which is why
 *                0 is reserved and can never be a real key.
 * Thread safety: NOT reentrant; advances a file-scope PRNG.
 * Cost:          O(1) expected.
 */
uint64_t set64_random(Set64 *s);

#endif
