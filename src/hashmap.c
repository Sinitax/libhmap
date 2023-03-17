#include "hashmap.h"

#include <errno.h>
#include <string.h>

static struct hashmap_link **hashmap_get_linkp(struct hashmap *map,
	const void *key, size_t size);
static int hashmap_link_alloc(struct hashmap *map, struct hashmap_link **link,
	void *key, size_t key_size, void *value, size_t value_size);

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

	iter = &map->buckets[hashmap_key_bucket(map, key, size)];
	while (*iter != NULL) {
		link = *iter;
		if (hashmap_key_cmp(link->key, link->key_size, key, size))
			return iter;
		iter = &(*iter)->next;
	}

	return NULL;
}

int
hashmap_link_alloc(struct hashmap *map, struct hashmap_link **out,
	void *key, size_t key_size, void *value, size_t value_size)
{
	struct hashmap_link *link;
	int rc;

	rc = map->allocator->alloc((void **)&link, sizeof(struct hashmap_link));
	if (rc) return -rc;
	link->key = key;
	link->key_size = key_size;
	link->value = value;
	link->value_size = value_size;
	link->next = NULL;
	*out = link;

	return 0;
}

int
hashmap_init(struct hashmap *map, size_t size, map_hash_func hasher,
	const struct allocator *allocator)
{
	int rc;

	map->allocator = allocator;
	map->size = size;
	map->hash = hasher;

	rc = map->allocator->alloc((void **)&map->buckets,
		sizeof(void *) * size);
	if (rc) return -rc;
	memset(map->buckets, 0, size * sizeof(void *));

	return 0;
}

void
hashmap_deinit(struct hashmap *map)
{
	hashmap_clear(map);
	map->allocator->free(map->buckets);
}

int
hashmap_alloc(struct hashmap **map, size_t size, map_hash_func hasher,
	const struct allocator *allocator)
{
	int rc;

	rc = allocator->alloc((void **)map, sizeof(struct hashmap));
	if (rc) return -rc;

	rc = hashmap_init(*map, size, hasher, allocator);
	if (rc) {
		allocator->free(*map);
		return rc;
	}

	return 0;
}

void
hashmap_free(struct hashmap *map)
{
	const struct allocator *allocator;

	allocator = map->allocator;
	hashmap_deinit(map);
	allocator->free(map);
}

void
hashmap_clear(struct hashmap *map)
{
	struct hashmap_iter iter;
	struct hashmap_link *prev;
	size_t i;

	prev = NULL;
	for (HASHMAP_ITER(map, &iter)) {
		free(prev);
		map->allocator->free(iter.link->key);
		map->allocator->free(iter.link->value);
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

	iter = hashmap_get_linkp(map, key, size);

	return iter ? *iter : NULL;
}

struct hashmap_link *
hashmap_pop(struct hashmap *map, const void *key, size_t size)
{
	struct hashmap_link **iter;

	iter = hashmap_get_linkp(map, key, size);
	if (iter) {
		*iter = (*iter)->next;
		return *iter;
	}

	return NULL;
}

void
hashmap_link_set(struct hashmap *map, struct hashmap_link *link,
	void *key, size_t key_size, void *value, size_t value_size)
{
	map->allocator->free(link->key);
	link->key = key;
	link->key_size = key_size;

	map->allocator->free(link->value);
	link->value = value;
	link->value_size = value_size;
}

int
hashmap_set(struct hashmap *map, void *key, size_t key_size,
	void *value, size_t value_size)
{
	struct hashmap_link **iter;
	int rc;

	iter = hashmap_get_linkp(map, key, key_size);
	if (iter == NULL) {
		iter = &map->buckets[hashmap_key_bucket(map, key, key_size)];
		while (*iter) iter = &((*iter)->next);
	}

	if (*iter) {
		hashmap_link_set(map, *iter,
			key, key_size, value, value_size);
	} else {
		rc = hashmap_link_alloc(map, iter,
			key, key_size, value, value_size);
		if (rc) return rc;
	}

	return 0;
}

void
hashmap_iter_init(struct hashmap_iter *iter)
{
	iter->bucket = 0;
	iter->link = NULL;
}

bool
hashmap_iter_next(struct hashmap *map, struct hashmap_iter *iter)
{
	size_t i;

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
	size_t i;

	hash = 5381;
	for (i = 0; i < size; i++)
		hash = 33 * hash + ((uint8_t *) data)[i];

	return hash;
}

