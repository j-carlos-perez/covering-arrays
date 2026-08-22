#include "set64.h"
#include <string.h>

/*
 * Hash function using SplitMix64 - scrambles the key bits to improve distribution.
 * Used for Robin Hood hashing probe sequence.
 */
static inline uint64_t mix64(uint64_t x) {
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ULL;
  x ^= x >> 33;
  return x;
}

/*
 * Fast 32-bit pseudo-random number generator using Xorshift.
 * Uses a static internal state; NOT thread-safe.
 * Returns values in range [0, UINT32_MAX].
 */
static inline uint32_t fast_rand_u32(void) {
  static uint64_t s = 88172645463325252ull;
  s ^= s << 13;
  s ^= s >> 7;
  s ^= s << 17;
  return (uint32_t)s;
}

/*
 * Rounds up x to the next power of 2.
 * Returns 1 if x <= 1, otherwise returns smallest pow(2, n) >= x.
 */
static inline uint32_t next_pow2_u32(uint32_t x) {
  if (x <= 1)
    return 1;
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return x + 1;
}

/*
 * Creates a new Set64 hash set with Robin Hood probing.
 * 
 * Capacity is rounded up to powers of 2 for efficient modulo via mask.
 * Table capacity is 2x the initial capacity (0.7 target load factor).
 * Keys 0 and 1 are reserved as sentinels (EMPTY_KEY, DELETED_KEY).
 * 
 * Initial capacity is the expected number of elements in dense array.
 * Caller must free with set64_free().
 */
Set64 *set64_create(uint32_t initial_capacity) {
  if (initial_capacity > UINT32_MAX / 2) {
    return NULL;
  }
  Set64 *s = (Set64 *)malloc(sizeof(Set64));
  if (!s)
    return NULL;

  s->cap = next_pow2_u32(initial_capacity);
  if (s->cap > (UINT32_MAX / 2)) {
    free(s);
    return NULL;
  }
  s->tcap = s->cap * 2; /* both are powers of two already */
  s->mask = s->tcap - 1;
  s->size = 0;

  if ((size_t)s->cap > SIZE_MAX / sizeof(uint64_t) ||
      (size_t)s->tcap > SIZE_MAX / sizeof(Set64Entry)) {
    free(s);
    return NULL;
  }
  s->dense = (uint64_t *)malloc(sizeof(uint64_t) * (size_t)s->cap);
  s->table =
      (Set64Entry *)calloc((size_t)s->tcap, sizeof(Set64Entry));

  if (!s->dense || !s->table) {
    free(s->dense);
    free(s->table);
    free(s);
    return NULL;
  }

  s->max_fill = (uint32_t)(((uint64_t)s->tcap * SET64_LOAD_FACTOR_NUM) /
                           SET64_LOAD_FACTOR_DEN);
  return s;
}

/*
 * Frees all memory associated with the set.
 * Handles NULL gracefully.
 */
void set64_free(Set64 *s) {
  if (!s)
    return;
  free(s->dense);
  free(s->table);
  free(s);
}

/*
 * Doubles the hash table capacity and rehashes all entries.
 * Used when load factor exceeds threshold.
 * Robin Hood swapping preserves probe distance invariants.
 */
static int set64_rehash(Set64 *s) {
  uint32_t old_tcap = s->tcap;
  Set64Entry *old_tab = s->table;
  uint64_t *old_dense = s->dense;

  if (s->tcap > UINT32_MAX / 2) {
    return 0; /* capacity overflow */
  }
  uint32_t new_tcap = s->tcap * 2;

  /* Allocate both replacements before committing either one. This keeps the
     public capacity fields and all existing keys unchanged on failure. */
  uint32_t new_cap = new_tcap / 2;
  if ((size_t)new_cap > SIZE_MAX / sizeof(uint64_t) ||
      (size_t)new_tcap > SIZE_MAX / sizeof(Set64Entry)) {
    return 0;
  }
  uint64_t *new_dense =
      (uint64_t *)malloc(sizeof(uint64_t) * (size_t)new_cap);
  Set64Entry *new_tab =
      (Set64Entry *)calloc((size_t)new_tcap, sizeof(Set64Entry));
  if (new_dense == NULL || new_tab == NULL) {
    free(new_dense);
    free(new_tab);
    return 0;
  }
  memcpy(new_dense, old_dense, sizeof(uint64_t) * (size_t)s->size);

  s->dense = new_dense;
  s->cap = new_cap;
  s->tcap = new_tcap;
  s->mask = s->tcap - 1;
  s->table = new_tab;

  s->max_fill = (uint32_t)(((uint64_t)s->tcap * SET64_LOAD_FACTOR_NUM) /
                           SET64_LOAD_FACTOR_DEN);

  for (uint32_t i = 0; i < old_tcap; i++) {
    uint64_t key = old_tab[i].key;
    if (key > SET64_DELETED_KEY) {
      uint32_t idx = old_tab[i].index;

      uint32_t pos = mix64(key) & s->mask;
      uint32_t dist = 0;

      while (true) {
        if (s->table[pos].key == SET64_EMPTY_KEY) {
          s->table[pos].key = key;
          s->table[pos].index = idx;
          break;
        }

        uint32_t home = mix64(s->table[pos].key) & s->mask;
        uint32_t other_dist = (pos - home) & s->mask;

        if (other_dist < dist) {
          uint64_t tmpk = s->table[pos].key;
          uint32_t tmpi = s->table[pos].index;

          s->table[pos].key = key;
          s->table[pos].index = idx;

          key = tmpk;
          idx = tmpi;
          dist = other_dist;
        }

        pos = (pos + 1) & s->mask;
        dist++;
      }
    }
  }

  free(old_dense);
  free(old_tab);
  return 1;
}

/*
 * Locates a key in the hash table.
 * Returns the table position, or -1 if the key is absent.
 *
 * The early exit is the Robin Hood invariant: every entry sits at least as far
 * from its home slot as anything inserted before it, so once we are further
 * from home than the entry we are looking at, our key cannot be further along.
 * That invariant only holds while the table has no tombstones, which is why
 * deletion below shifts entries back instead of marking them.
 */
static int32_t set64_find(const Set64 *s, uint64_t key) {
  uint32_t pos = mix64(key) & s->mask;
  uint32_t dist = 0;

  while (true) {
    uint64_t k = s->table[pos].key;

    if (k == SET64_EMPTY_KEY)
      return -1;
    if (k == key)
      return (int32_t)pos;

    uint32_t home = mix64(k) & s->mask;
    uint32_t other_dist = (pos - home) & s->mask;

    if (other_dist < dist)
      return -1;

    pos = (pos + 1) & s->mask;
    dist++;
  }
}

/*
 * Inserts a key into the set using Robin Hood hashing.
 * Grows the table (and the dense array with it) when the load factor is hit.
 * Duplicate keys are ignored, as documented -- the previous version appended
 * them to dense[] and inflated size.
 */
void set64_insert(Set64 *s, uint64_t key) {
  if (s == NULL || key <= SET64_DELETED_KEY) {
    return; /* 0 and 1 are reserved sentinels */
  }

  if (set64_find(s, key) >= 0) {
    return;
  }

  if (s->size >= s->max_fill || s->size >= s->cap) {
    if (!set64_rehash(s)) {
      return; /* out of memory: leave the set untouched rather than overrun it */
    }
  }

  uint32_t pos = mix64(key) & s->mask;
  uint32_t dist = 0;

  uint32_t idx = s->size;
  s->dense[idx] = key;
  s->size++;

  while (true) {
    if (s->table[pos].key == SET64_EMPTY_KEY) {
      s->table[pos].key = key;
      s->table[pos].index = idx;
      return;
    }

    uint32_t home = mix64(s->table[pos].key) & s->mask;
    uint32_t other_dist = (pos - home) & s->mask;

    if (other_dist < dist) {
      uint64_t tmpk = s->table[pos].key;
      uint32_t tmpi = s->table[pos].index;

      s->table[pos].key = key;
      s->table[pos].index = idx;

      key = tmpk;
      idx = tmpi;
      dist = other_dist;
    }

    pos = (pos + 1) & s->mask;
    dist++;
  }
}

/*
 * Removes a key from the set.
 *
 * dense[] is kept compact by moving the last element into the freed slot. The
 * table entry is removed by backward-shift deletion: following entries that
 * are not already at their home slot move back one position, which keeps the
 * Robin Hood invariant that set64_find() relies on. Tombstones would break it.
 *
 * Returns true if deleted, false if the key was not present.
 */
bool set64_delete(Set64 *s, uint64_t key) {
  if (s == NULL || s->size == 0) {
    return false;
  }

  int32_t found = set64_find(s, key);
  if (found < 0) {
    return false;
  }
  uint32_t pos = (uint32_t)found;
  uint32_t idx = s->table[pos].index;

  /* Compact dense[] first: locating `last` must happen while the table is
     still intact. */
  uint64_t last = s->dense[s->size - 1];
  if (last != key) {
    int32_t last_pos = set64_find(s, last);
    if (last_pos < 0) {
      return false; /* table corrupt; refuse rather than write to table[-1] */
    }
    s->dense[idx] = last;
    s->table[last_pos].index = idx;
  }
  s->size--;

  /* Backward-shift deletion. Terminates because max_fill < tcap guarantees at
     least one empty slot. */
  uint32_t i = pos;
  uint32_t j = (pos + 1) & s->mask;
  while (s->table[j].key != SET64_EMPTY_KEY) {
    uint32_t home = mix64(s->table[j].key) & s->mask;
    if (((j - home) & s->mask) == 0) {
      break; /* already at its home slot: cannot move back */
    }
    s->table[i] = s->table[j];
    i = j;
    j = (j + 1) & s->mask;
  }
  s->table[i].key = SET64_EMPTY_KEY;

  return true;
}

/*
 * Returns a uniformly random element from the set.
 * Returns SET64_EMPTY_KEY if the set is empty.
 *
 * Rejection sampling, not `rand & (size - 1)`: masking is only uniform when
 * size is a power of two, and for e.g. size 6 it can only ever produce four of
 * the six indices.
 */
uint64_t set64_random(Set64 *s) {
  if (s == NULL || s->size == 0)
    return SET64_EMPTY_KEY;

  uint32_t n = s->size;
  /* Largest multiple of n that fits in uint32_t; draws above it are rejected
     so every index is equally likely. */
  uint32_t limit = UINT32_MAX - (UINT32_MAX % n);
  uint32_t r;
  do {
    r = fast_rand_u32();
  } while (r >= limit);

  return s->dense[r % n];
}
