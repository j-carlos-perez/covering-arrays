#ifndef SET64_H
#define SET64_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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

#define SET64_A_BITS 15
#define SET64_A_MASK ((1ULL << SET64_A_BITS) - 1)

typedef struct {
  uint64_t key;
  uint32_t index;
} Set64Entry;

typedef struct {
  uint64_t *dense;
  Set64Entry *table;
  uint32_t size;
  uint32_t cap;
  uint32_t tcap;
  uint32_t mask;
  uint32_t max_fill;
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

static inline uint32_t set64_key_get_a(uint64_t k) {
  k -= 2;
  return (uint32_t)(k & SET64_A_MASK);
}

static inline uint32_t set64_key_get_b(uint64_t k) {
  k -= 2;
  return (uint32_t)(k >> SET64_A_BITS);
}

Set64 *set64_create(uint32_t initial_capacity);
void set64_free(Set64 *s);
void set64_insert(Set64 *s, uint64_t key);
bool set64_delete(Set64 *s, uint64_t key);
uint64_t set64_random(Set64 *s);

#endif