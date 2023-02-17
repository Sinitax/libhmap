#include "hashmap.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct hashmap_link **hashmap_get_linkp(struct hashmap *map,
	const void *key, size_t size);
static struct hashmap_link *hashmap_link_alloc(void *key, size_t key_size,
	void *value, size_t value_size);

static inline size_t
hashmap_key_bucket(struct hashmap *map, const void *key, size_t size)
{
	return map->hash(key, size) % map->size;
}

static inline bool
hashmap_key_cmp(const void *a, size_t asize, const void *b, size_t bsize)
{
	return asize == bsize && !memcmp(a, b, asize);
}

struct hashmap_link **
hashmap_get_linkp(struct hashmap *map, const void *key, size_t size)
{
	struct hashmap_link **iter, *link;

	LIBHASHMAP_ASSERT(map != NULL && key != NULL && size != 0);

	iter = &map->buckets[hashmap_key_bucket(map, key, size)];
	while (*iter != NULL) {
		link = *iter;
		if (hashmap_key_cmp(link->key, link->key_size, key, size))
			return iter;
		iter = &(*iter)->next;
	}

	return NULL;
}

struct hashmap_link *
hashmap_link_alloc(void *key, size_t key_size, void *value, size_t value_size)
{
	struct hashmap_link *link;

	LIBHASHMAP_ASSERT(key != NULL && value != NULL);

	link = malloc(sizeof(struct hashmap_link));
	if (!link) {
		LIBHASHMAP_HANDLE_ERR("malloc");
		return NULL;
	}
	link->key = key;
	link->key_size = key_size;
	link->value = value;
	link->value_size = value_size;
	link->next = NULL;

	return link;
}

bool
hashmap_init(struct hashmap *map, size_t size, map_hash_func hasher)
{
	LIBHASHMAP_ASSERT(map != NULL && size != 0 && hasher != NULL);

	map->buckets = calloc(size, sizeof(struct hashmap_link *));
	if (!map->buckets) {
		LIBHASHMAP_HANDLE_ERR("calloc");
		return false;
	}
	map->size = size;
	map->hash = hasher;

	return true;
}

void
hashmap_deinit(struct hashmap *map)
{
	LIBHASHMAP_ASSERT(map != NULL);

	hashmap_clear(map);
	free(map->buckets);
}

struct hashmap *
hashmap_alloc(size_t size, map_hash_func hasher)
{
	struct hashmap *map;

	map = malloc(sizeof(struct hashmap));
	if (!map) {
		LIBHASHMAP_HANDLE_ERR("malloc");
		return NULL;
	}

	if (!hashmap_init(map, size, hasher)) {
		free(map);
		return NULL;
	}

	return map;
}

void
hashmap_free(struct hashmap *map)
{
	hashmap_deinit(map);
	free(map);
}

void
hashmap_clear(struct hashmap *map)
{
	struct hashmap_iter iter;
	struct hashmap_link *prev;
	int i;

	LIBHASHMAP_ASSERT(map != NULL);

	prev = NULL;
	for (HASHMAP_ITER(map, &iter)) {
		free(iter.link->key);
		free(iter.link->value);
		free(prev);
		prev = iter.link;
	}
	free(prev);

	for (i = 0; i < map->size; i++)
		map->buckets[i] = NULL;
}

struct hashmap_link *
hashmap_get(struct hashmap *map, const void *key, size_t size)
{
	struct hashmap_link **iter;

	LIBHASHMAP_ASSERT(map != NULL);

	iter = hashmap_get_linkp(map, key, size);

	return iter ? *iter : NULL;
}

struct hashmap_link *
hashmap_pop(struct hashmap *map, const void *key, size_t size)
{
	struct hashmap_link **iter;

	LIBHASHMAP_ASSERT(map != NULL);

	iter = hashmap_get_linkp(map, key, size);
	if (iter) {
		*iter = (*iter)->next;
		return *iter;
	}

	return NULL;
}

void
hashmap_link_set(struct hashmap_link *link, void *key, size_t key_size,
	void *value, size_t value_size)
{
	LIBHASHMAP_ASSERT(link != NULL);

	free(link->key);
	link->key = key;
	link->key_size = key_size;

	free(link->value);
	link->value = value;
	link->value_size = value_size;
}

bool
hashmap_set(struct hashmap *map, void *key, size_t key_size,
	void *value, size_t value_size)
{
	struct hashmap_link **iter;

	LIBHASHMAP_ASSERT(map != NULL);

	iter = hashmap_get_linkp(map, key, key_size);
	if (iter == NULL) {
		iter = &map->buckets[hashmap_key_bucket(map, key, key_size)];
		while (*iter) iter = &((*iter)->next);
	}

	if (*iter) {
		hashmap_link_set(*iter, key, key_size, value, value_size);
	} else {
		*iter = hashmap_link_alloc(key, key_size, value, value_size);
		if (!*iter) return false;
	}

	return true;
}

void
hashmap_iter_init(struct hashmap_iter *iter)
{
	LIBHASHMAP_ASSERT(iter != NULL);

	iter->bucket = 0;
	iter->link = NULL;
}

bool
hashmap_iter_next(struct hashmap *map, struct hashmap_iter *iter)
{
	int i;

	LIBHASHMAP_ASSERT(map != NULL && iter != NULL);

	if (iter->link && iter->link->next) {
		iter->link = iter->link->next;
		return true;
	}

	i = iter->bucket + (iter->link ? 1 : 0);
	for (; i < map->size; i++) {
		if (map->buckets[i]) {
			iter->bucket = i;
			iter->link = map->buckets[i];
			return true;
		}
	}

	iter->link = NULL;

	return false;
}

uint32_t
hashmap_str_hasher(const void *data, size_t size)
{
	uint32_t hash;
	int i;

	LIBHASHMAP_ASSERT(data != NULL && size > 0);

	hash = 0;
	for (i = 0; i < size; i++)
		hash = 31 * hash + ((uint8_t *) data)[i];

	return hash;
}

