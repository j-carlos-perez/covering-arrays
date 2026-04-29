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

typedef struct {
  uint64_t key;
  uint32_t index;
} Entry;

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
  Set64 *s = (Set64 *)malloc(sizeof(Set64));
  if (!s)
    return NULL;

  s->cap = next_pow2_u32(initial_capacity);
  s->tcap = next_pow2_u32(s->cap * 2);
  s->mask = s->tcap - 1;
  s->size = 0;

  s->dense = (uint64_t *)malloc(sizeof(uint64_t) * s->cap);
  s->table = (Set64Entry *)malloc(sizeof(Set64Entry) * s->tcap);

  if (!s->dense || !s->table) {
    free(s->dense);
    free(s->table);
    free(s);
    return NULL;
  }

  for (uint32_t i = 0; i < s->tcap; i++) {
    s->table[i].key = SET64_EMPTY_KEY;
  }

  s->max_fill = (s->tcap * SET64_LOAD_FACTOR_NUM) / SET64_LOAD_FACTOR_DEN;
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
static void set64_rehash(Set64 *s) {
  uint32_t old_tcap = s->tcap;
  Set64Entry *old_tab = s->table;

  s->tcap <<= 1;
  s->mask = s->tcap - 1;
  s->table = (Set64Entry *)malloc(sizeof(Set64Entry) * s->tcap);

  for (uint32_t i = 0; i < s->tcap; i++)
    s->table[i].key = SET64_EMPTY_KEY;

  s->max_fill = (s->tcap * SET64_LOAD_FACTOR_NUM) / SET64_LOAD_FACTOR_DEN;

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

  free(old_tab);
}

/*
 * Inserts a key into the set using Robin Hood hashing.
 * If the set is at capacity, automatically rehashing occurs.
 * Duplicate keys are ignored.
 */
void set64_insert(Set64 *s, uint64_t key) {
  if (s->size >= s->max_fill) {
    set64_rehash(s);
  }

  uint32_t pos = mix64(key) & s->mask;
  uint32_t dist = 0;

  uint32_t idx = s->size;
  s->dense[idx] = key;
  s->size++;

  while (true) {
    if (s->table[pos].key <= SET64_DELETED_KEY) {
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
 * Searches for a key in the hash table.
 * Returns the position if found, or -1 if not found.
 * Uses probe distance to terminate early when past home position.
 */
static int32_t set64_find(Set64 *s, uint64_t key) {
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
 * Removes a key from the set.
 * Swaps the deleted entry with the last element in dense array to maintain consistency.
 * Returns true if deleted, false if key was not found.
 */
bool set64_delete(Set64 *s, uint64_t key) {
  int32_t pos = set64_find(s, key);
  if (pos < 0)
    return false;

  uint32_t idx = s->table[pos].index;
  uint64_t last = s->dense[s->size - 1];

  s->dense[idx] = last;

  int32_t last_pos = set64_find(s, last);
  s->table[last_pos].index = idx;

  s->size--;

  s->table[pos].key = SET64_DELETED_KEY;
  return true;
}

/*
 * Returns a uniformly random element from the set.
 * Uses fast_rand_u32() for sampling the dense array index.
 * Returns SET64_EMPTY_KEY if the set is empty.
 */
uint64_t set64_random(Set64 *s) {
  if (s->size == 0)
    return SET64_EMPTY_KEY;
  return s->dense[fast_rand_u32() & (s->size - 1)];
}