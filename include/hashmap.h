#pragma once

#include "allocator.h"

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define HASHMAP_ITER(map, iter) \
	hashmap_iter_init(iter); hashmap_iter_next(map, iter);

typedef uint32_t (*hashmap_hash_func)(const void *data, size_t size);
typedef bool (*hashmap_keycmp_func)(const void *key1, size_t size1,
	const void *key2, size_t size2);

struct hashmap_link {
	void *key;
	size_t keysize;
	void *value;
	struct hashmap_link *next;
};

struct hashmap_iter {
	struct hashmap_link *link;
	size_t bucket;
};

struct hashmap {
	hashmap_hash_func hash;
	hashmap_keycmp_func keycmp;
	struct hashmap_link **buckets;
	size_t size;
	const struct allocator *allocator;
};

int hashmap_init(struct hashmap *map, size_t size, hashmap_hash_func hasher,
	hashmap_keycmp_func keycmp, const struct allocator *allocator);
void hashmap_deinit(struct hashmap *map);

int hashmap_alloc(struct hashmap **map, size_t size, hashmap_hash_func hasher,
	hashmap_keycmp_func keycmp, const struct allocator *allocator);
void hashmap_free(struct hashmap *map);

int hashmap_copy(struct hashmap *dst, const struct hashmap *src);
void hashmap_swap(struct hashmap *m1, struct hashmap *m2);
void hashmap_clear(struct hashmap *map);

struct hashmap_link **hashmap_link_get(struct hashmap *map,
	const void *key, size_t size);
struct hashmap_link **hashmap_link_pos(struct hashmap *map,
	const void *key, size_t size);
struct hashmap_link *hashmap_link_pop(struct hashmap *map,
	const void *key, size_t size);
void hashmap_link_set(struct hashmap *map, struct hashmap_link *link,
	void *key, size_t keysize, void *value);
int hashmap_link_alloc(struct hashmap *map, struct hashmap_link **out,
	void *key, size_t keysize, void *value);

struct hashmap_link *hashmap_get(struct hashmap *map,
	const void *key, size_t size);
void hashmap_rm(struct hashmap *map, const void *key, size_t size);
int hashmap_set(struct hashmap *map, void *key, size_t keysize, void *value);

void hashmap_iter_init(struct hashmap_iter *iter);
bool hashmap_iter_next(const struct hashmap *map, struct hashmap_iter *iter);

uint32_t hashmap_str_hasher(const void *data, size_t size);
bool hashmap_default_keycmp(const void *key1, size_t size1,
	const void *key2, size_t size2);
