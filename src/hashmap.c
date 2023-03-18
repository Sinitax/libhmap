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

struct hashmap_link **
hashmap_get_linkp(struct hashmap *map, const void *key, size_t size)
{
	struct hashmap_link **iter, *link;

	iter = &map->buckets[hashmap_key_bucket(map, key, size)];
	while (*iter != NULL) {
		link = *iter;
		if (map->keycmp(link->key, link->key_size, key, size))
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
	void *key, *value;
	size_t key_size;
	size_t value_size;
	int rc;

	for (HASHMAP_ITER(src, &iter)) {
		key_size = iter.link->key_size;
		rc = dst->allocator->alloc(&key, key_size);
		if (rc) return -rc;
		memcpy(key, iter.link->key, key_size);

		value_size = iter.link->value_size;
		rc = dst->allocator->alloc(&value, value_size);
		if (rc) return -rc;
		memcpy(value, iter.link->value, value_size);

		rc = hashmap_set(dst, NULL, key, key_size, value, value_size);
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

int
hashmap_link_set(struct hashmap *map, struct hashmap_link *link,
	void *key, size_t key_size, void *value, size_t value_size)
{
	int rc;

	rc = map->allocator->free(link->key);
	if (rc) return -rc;
	link->key = key;
	link->key_size = key_size;

	rc = map->allocator->free(link->value);
	if (rc) return -rc;
	link->value = value;
	link->value_size = value_size;

	return 0;
}

int
hashmap_set(struct hashmap *map, struct hashmap_link **link,
	void *key, size_t key_size, void *value, size_t value_size)
{
	struct hashmap_link **iter;
	int rc;

	iter = hashmap_get_linkp(map, key, key_size);
	if (iter == NULL) {
		iter = &map->buckets[hashmap_key_bucket(map, key, key_size)];
		while (*iter) iter = &((*iter)->next);
	}

	if (*iter) {
		rc = hashmap_link_set(map, *iter,
			key, key_size, value, value_size);
		if (rc) return rc;
	} else {
		rc = hashmap_link_alloc(map, iter,
			key, key_size, value, value_size);
		if (rc) return rc;
	}

	if (link) *link = *iter;

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
