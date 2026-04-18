#ifndef SET64_H
#define SET64_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define SET64_LOAD_FACTOR_NUM 7
#define SET64_LOAD_FACTOR_DEN 10

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

static inline uint64_t set64_mix64(uint64_t x) {
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ULL;
  x ^= x >> 33;
  return x;
}

static inline uint32_t set64_fast_rand_u32(void) {
  static uint64_t s = 88172645463325252ull;
  s ^= s << 13;
  s ^= s >> 7;
  s ^= s << 17;
  return (uint32_t)s;
}

static inline uint32_t set64_next_pow2_u32(uint32_t x) {
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

static inline uint64_t set64_make_key(uint32_t a, uint32_t b) {
  uint64_t k = ((uint64_t)b << SET64_A_BITS) | (uint64_t)a;
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