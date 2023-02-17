#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef LIBHASHMAP_ASSERT_ENABLE

#include <stdio.h>

#define LIBHASHMAP_ASSERT(x) libhashmap_assert((x), __FILE__, __LINE__, #x)

static inline void libhashmap_assert(int cond,
	const char *file, int line, const char *condstr)
{
	if (cond) return;

	fprintf(stderr, "libhashmap: Assertion failed at %s:%i (%s)\n",
		file, line, condstr);
	abort();
}

#else
#define LIBHASHMAP_ASSERT(x)
#endif

#ifdef LIBHASHMAP_HANDLE_ERRS

#include <errno.h>
#include <stdio.h>

#define LIBHASHMAP_HANDLE_ERR(x) libhashmap_err(__FILE__, __LINE__, x)

static inline void libhashmap_err(const char *file, int line, const char *info)
{
	fprintf(stderr, "libhashmap: %s at %s:%i: %s\n",
		info, file, line, strerror(errno));
	exit(1);
}

#else
#define LIBHASHMAP_HANDLE_ERR(x)
#endif


#define HASHMAP_ITER(map, iter) \
	hashmap_iter_init(iter); hashmap_iter_next(map, iter);

typedef uint32_t (*map_hash_func)(const void *data, size_t size);

struct hashmap_link {
	uint8_t *key;
	size_t key_size;
	uint8_t *value;
	size_t value_size;
	struct hashmap_link *next;
};

struct hashmap_iter {
	struct hashmap_link *link;
	size_t bucket;
};

struct hashmap {
	map_hash_func hash;
	struct hashmap_link **buckets;
	size_t size;
};

bool hashmap_init(struct hashmap *map, size_t size, map_hash_func hasher);
void hashmap_deinit(struct hashmap *map);

struct hashmap *hashmap_alloc(size_t size, map_hash_func hasher);
void hashmap_free(struct hashmap *map);

void hashmap_clear(struct hashmap *map);

struct hashmap_link *hashmap_get(struct hashmap *map, const void *key, size_t size);
struct hashmap_link *hashmap_pop(struct hashmap *map, const void *key, size_t size);

void hashmap_link_set(struct hashmap_link *link, void *key, size_t key_size,
	void *value, size_t value_size);
void hashmap_set(struct hashmap *map, void *key, size_t key_size,
	void *value, size_t value_size);

void hashmap_iter_init(struct hashmap_iter *iter);
bool hashmap_iter_next(struct hashmap *map, struct hashmap_iter *iter);

uint32_t hashmap_str_hasher(const void *data, size_t size);

