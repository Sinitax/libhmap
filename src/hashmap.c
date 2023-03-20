#include "hashmap.h"

#include <errno.h>
#include <string.h>

static inline size_t
hashmap_key_bucket(struct hashmap *map, const void *key, size_t size)
{
	return map->hash(key, size) % map->size;
}

int
hashmap_init(struct hashmap *map, size_t size, hashmap_hash_func hasher,
	hashmap_keycmp_func keycmp, const struct allocator *allocator)
{
	int rc;

	map->allocator = allocator;
	map->keycmp = keycmp;
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
hashmap_alloc(struct hashmap **map, size_t size, hashmap_hash_func hasher,
	hashmap_keycmp_func keycmp, const struct allocator *allocator)
{
	int rc;

	rc = allocator->alloc((void **)map, sizeof(struct hashmap));
	if (rc) return -rc;

	rc = hashmap_init(*map, size, hasher, keycmp, allocator);
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

int
hashmap_copy(struct hashmap *dst, const struct hashmap *src)
{
	struct hashmap_iter iter;
	int rc;

	for (HASHMAP_ITER(src, &iter)) {
		rc = hashmap_set(dst, iter.link->key,
			iter.link->keysize, iter.link->value);
		if (rc) return rc;
	}

	return 0;
}

void
hashmap_swap(struct hashmap *m1, struct hashmap *m2)
{
	struct hashmap tmp;

	memcpy(&tmp, m1, sizeof(struct hashmap));
	memcpy(m1, m2, sizeof(struct hashmap));
	memcpy(m2, &tmp, sizeof(struct hashmap));
}

void
hashmap_clear(struct hashmap *map)
{
	struct hashmap_iter iter;
	struct hashmap_link *prev;
	size_t i;

	prev = NULL;
	for (HASHMAP_ITER(map, &iter)) {
		map->allocator->free(prev);
		prev = iter.link;
	}
	map->allocator->free(prev);

	for (i = 0; i < map->size; i++)
		map->buckets[i] = NULL;
}

struct hashmap_link **
hashmap_link_get(struct hashmap *map, const void *key, size_t size)
{
	struct hashmap_link **iter, *link;

	iter = &map->buckets[hashmap_key_bucket(map, key, size)];
	while (*iter != NULL) {
		link = *iter;
		if (map->keycmp(link->key, link->keysize, key, size))
			return iter;
		iter = &(*iter)->next;
	}

	return NULL;
}

struct hashmap_link **
hashmap_link_pos(struct hashmap *map, const void *key, size_t keysize)
{
	struct hashmap_link **iter;

	iter = hashmap_link_get(map, key, keysize);
	if (iter == NULL) {
		iter = &map->buckets[hashmap_key_bucket(map, key, keysize)];
		while (*iter) iter = &((*iter)->next);
	}

	return iter;
}

struct hashmap_link *
hashmap_link_pop(struct hashmap *map, const void *key, size_t keysize)
{
	struct hashmap_link **iter;

	iter = hashmap_link_get(map, key, keysize);
	if (iter) {
		*iter = (*iter)->next;
		return *iter;
	}

	return NULL;
}

void
hashmap_link_set(struct hashmap *map, struct hashmap_link *link,
	void *key, size_t keysize, void *value)
{
	link->key = key;
	link->keysize = keysize;
	link->value = value;
}

int
hashmap_link_alloc(struct hashmap *map, struct hashmap_link **out,
	void *key, size_t keysize, void *value)
{
	struct hashmap_link *link;
	int rc;

	rc = map->allocator->alloc((void **)&link, sizeof(struct hashmap_link));
	if (rc) return -rc;
	link->key = key;
	link->keysize = keysize;
	link->value = value;
	link->next = NULL;
	*out = link;

	return 0;
}

struct hashmap_link *
hashmap_get(struct hashmap *map, const void *key, size_t keysize)
{
	struct hashmap_link **iter;

	iter = hashmap_link_get(map, key, keysize);

	return iter ? *iter : NULL;
}

void
hashmap_rm(struct hashmap *map, const void *key, size_t keysize)
{
	struct hashmap_link *link;

	link = hashmap_link_pop(map, key, keysize);
	if (!link) return;
	map->allocator->free(link);	
}

int
hashmap_set(struct hashmap *map, void *key, size_t keysize, void *value)
{
	struct hashmap_link **iter;

	iter = hashmap_link_pos(map, key, keysize);
	if (!*iter) return HMAP_KEY_MISSING;

	hashmap_link_set(map, *iter, key, keysize, value);

	return 0;
}

int
hashmap_add(struct hashmap *map, void *key, size_t keysize, void *value)
{
	struct hashmap_link **iter;
	int rc;

	iter = hashmap_link_pos(map, key, keysize);
	if (*iter) return HMAP_KEY_EXISTS;

	rc = hashmap_link_alloc(map, iter, key, keysize, value);
	if (rc) return rc;

	return 0;
}

void
hashmap_iter_init(struct hashmap_iter *iter)
{
	iter->bucket = 0;
	iter->link = NULL;
}

bool
hashmap_iter_next(const struct hashmap *map, struct hashmap_iter *iter)
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

bool
hashmap_default_keycmp(const void *key1, size_t size1,
	const void *key2, size_t size2)
{
	return size1 == size2 && !memcmp(key1, key2, size1);
}
