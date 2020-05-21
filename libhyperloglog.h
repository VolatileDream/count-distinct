#ifndef LIB_HLL
#define LIB_HLL

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef uint64_t (*hash_func)(const void *key, int len, uint32_t seed);
typedef struct hyperloglog count_t;

// Creates a new filter
count_t* hll_init(hash_func hf, uint8_t precision, uint64_t seed);
// Deallocates the filter
void hll_del(count_t *c);

// Adds the item to the count.
void hll_add(count_t *c, const void *key, int len);
// Gets the count of items in the hyperloglog struct.
uint64_t hll_count(count_t *c);
double hll_error(count_t *c);

// Initializes a new filter, merged from the previous two.
// Does not free the existing filters.
count_t* hll_merge(count_t *c1, count_t *c2);

bool hll_write_to_file(count_t *c, FILE *file);
count_t* hll_read_from_file(FILE *file, hash_func hf);

#endif /* LIB_HLL */
