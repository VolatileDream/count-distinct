#include "libhyperloglog.h"
#include "libs/c/serde/libserde.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const uint32_t MAGIC_HEADER = 0x8000411c;

struct hyperloglog {
  hash_func func; // hash function pointer.
  uint64_t seed;
  uint8_t precision; // log base 2 of size.
  uint8_t *zeros; // storage of counters, length size.
};

uint32_t hll_size(uint8_t precision) {
  return 1 << precision;
}

count_t* hll_init(hash_func func, uint8_t precision, uint64_t seed) {
  // The compiler will generate the struct such that .zeros is at the end and
  // aligned, we can place the allocated content of the array after it.
  count_t *c = (count_t*) malloc(sizeof(count_t) + hll_size(precision));
  c->func = func;
  c->seed = seed;
  c->precision = precision;
  c->zeros = (uint8_t*)(c + 1); // point to just after the struct.
  memset(c->zeros, 0, hll_size(c->precision));
  return c;
}

void hll_del(count_t *c) {
  free(c);
}

uint8_t max(uint8_t u1, uint8_t u2) {
  if (u1 >= u2) {
    return u1;
  }
  return u2;
}

// Adds the item to the count.
void hll_add(count_t *c, const void *key, int len) {
  uint64_t hash = c->func(key, len, c->seed);
  // Split the hash into a register and value.
  uint64_t mask = (1 << c->precision) - 1;
  uint64_t reg = hash & mask;
  // We need the count of leading zeros to have output
  // values zero and up. We bit-or with the mask to avoid
  // using those bits for the register computation and
  // leading zero count.
  uint64_t value = __builtin_clz(hash | mask) + 1;

  c->zeros[reg] = max(c->zeros[reg], value);
}

double alpha(uint8_t precision) {
  // From https://en.wikipedia.org/wiki/HyperLogLog#Practical_Considerations
  switch (precision) {
    case 4:
      return 0.673;
    case 5:
      return 0.697;
    case 6:
      return 0.709;
    default:
      return 0.7213 / ( 1.0 + 1.079 / hll_size(precision));
  }
}

// It'd be nicer to use a tuple as a return value instead...
double raw_estimate(count_t *c) {
  const uint32_t size = hll_size(c->precision);
  const double correction = alpha(c->precision);

  double sum = 0;
  for (uint32_t i = 0; i < size; i++) {
    uint8_t reg = c->zeros[i];
    sum += 1.0 / (1 << reg);
  }

  return size * size * correction / sum;
}

int zero_count(count_t *c) {
  const uint32_t size = hll_size(c->precision);
  int zeros = 0;
  for (uint32_t i = 0; i < size; i++) {
    if (c->zeros[i] == 0) {
      zeros++;
    }
  }
  return zeros;
}

uint64_t hll_count(count_t *c) {
  const uint32_t size = hll_size(c->precision);
  const double raw = raw_estimate(c);
  // fprintf(stderr, "raw estimate: %lf\n", raw);
  if (raw < 5/2 * size) {
    // Lower bound correction.
    int zeros = zero_count(c);
    if (zeros != 0) {
      double est = size * log2(1.0 * size / zeros);
      // fprintf(stderr, "low estimate with zeros: %d, E*=%lf\n", zeros, est);
      return est;
    }
  } else if (raw > (1.0 / 30) * pow(2, 32)) {
    return - pow(2, 32) * log2(1 - raw / pow(2, 32));
  }
  // No upper bound correction because we use 64 bit registers.
  return raw;
}

double hll_error(count_t *c) {
  return 1.04 / sqrt(hll_size(c->precision));
}

// Initializes a new hll structure, merged from the previous two.
// Does not free the existing structures.
count_t* hll_merge(count_t *c1, count_t *c2) {
  if (c1->func != c2->func || c1->precision != c2->precision || c1->seed != c2->seed) {
    return false;
  }

  count_t *out = hll_init(c1->func, c1->precision, c1->seed);
  for (uint32_t i = 0; i < hll_size(c1->precision); i++) {
    out->zeros[i] = max(c1->zeros[i], c2->zeros[i]);
  }
  return out;
}

bool hll_write_to_file(count_t *c, FILE *file) {
  bool ok = true;
  ok = ok && serde_write(file, MAGIC_HEADER);
  ok = ok && serde_write(file, c->seed);
  ok = ok && serde_write(file, c->precision);

  for (uint64_t i=0; i < hll_size(c->precision) && ok; i++) {
    ok = ok && serde_write(file, c->zeros[i]);
  }

  return ok;
}

count_t* hll_read_from_file(FILE *file, hash_func hf) {
  uint32_t header = 0;
  uint64_t seed = 0;
  uint8_t precision = 0;

  bool ok = true;
  ok = ok && serde_read(file, &header);
  ok = ok && serde_read(file, &seed);
  ok = ok && serde_read(file, &precision);

  if (!ok || header != MAGIC_HEADER) {
    return 0;
  }

  count_t *c = hll_init(hf, precision, seed);
  for (uint64_t i=0; i < hll_size(precision) && ok; i++) {
    ok = ok && serde_read(file, &c->zeros[i]);
  }

  if (!ok) {
    hll_del(c);
    return 0;
  }
  return c;
}
