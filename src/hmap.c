#include "hmap.h"

#include <errno.h>
#include <string.h>

static inline size_t
hmap_key_bucket(const struct hmap *map, struct hmap_key key)
{
	return map->hash(key) % map->bucketcnt;
}

int
hmap_init(struct hmap *map, size_t size, hmap_hash_func hasher,
	hmap_keycmp_func keycmp, const struct allocator *allocator)
{
	int rc;

	LIBHMAP_ABORT_ON_ARGS(!map || !size || !hasher || !keycmp || !allocator);

	map->allocator = allocator;
	map->keycmp = keycmp;
	map->bucketcnt = size;
	map->hash = hasher;
	rc = map->allocator->alloc((void **)&map->buckets,
		sizeof(void *) * size);
	if (rc) return -rc;
	memset(map->buckets, 0, size * sizeof(void *));

	return 0;
}

void
hmap_deinit(struct hmap *map)
{
	LIBHMAP_ABORT_ON_ARGS(!map);

	hmap_clear(map);

	map->allocator->free(map->buckets);
}

int
hmap_alloc(struct hmap **map, size_t size, hmap_hash_func hasher,
	hmap_keycmp_func keycmp, const struct allocator *allocator)
{
	int rc;

	LIBHMAP_ABORT_ON_ARGS(!allocator);

	rc = allocator->alloc((void **)map, sizeof(struct hmap));
	if (rc) return -rc;

	rc = hmap_init(*map, size, hasher, keycmp, allocator);
	if (rc) return rc;

	return 0;
}

void
hmap_free(struct hmap *map)
{
	const struct allocator *allocator;

	LIBHMAP_ABORT_ON_ARGS(!map);

	allocator = map->allocator;
	hmap_deinit(map);
	allocator->free(map);
}

int
hmap_copy(const struct hmap *dst, const struct hmap *src)
{
	struct hmap_iter iter;
	int rc;

	LIBHMAP_ABORT_ON_ARGS(!dst || !src);

	for (HMAP_ITER(src, &iter)) {
		rc = hmap_set(dst, iter.link->key, iter.link->value);
		if (rc) return rc;
	}

	return 0;
}

void
hmap_swap(struct hmap *m1, struct hmap *m2)
{
	struct hmap tmp;

	LIBHMAP_ABORT_ON_ARGS(!m1 || !m2);

	memcpy(&tmp, m1, sizeof(struct hmap));
	memcpy(m1, m2, sizeof(struct hmap));
	memcpy(m2, &tmp, sizeof(struct hmap));
}

void
hmap_clear(const struct hmap *map)
{
	struct hmap_iter iter;
	struct hmap_link *prev;
	size_t i;

	LIBHMAP_ABORT_ON_ARGS(!map);

	prev = NULL;
	for (HMAP_ITER(map, &iter)) {
		map->allocator->free(prev);
		prev = iter.link;
	}
	map->allocator->free(prev);

	for (i = 0; i < map->bucketcnt; i++)
		map->buckets[i] = NULL;
}

struct hmap_link **
hmap_link_get(const struct hmap *map, struct hmap_key key)
{
	struct hmap_link **iter, *link;

	LIBHMAP_ABORT_ON_ARGS(!map);

	iter = &map->buckets[hmap_key_bucket(map, key)];
	while (*iter != NULL) {
		link = *iter;
		if (map->keycmp(link->key, key))
			return iter;
		iter = &(*iter)->next;
	}

	return NULL;
}

struct hmap_link **
hmap_link_pos(const struct hmap *map, struct hmap_key key)
{
	struct hmap_link **iter, *link;

	LIBHMAP_ABORT_ON_ARGS(!map);

	iter = &map->buckets[hmap_key_bucket(map, key)];
	while (*iter != NULL) {
		link = *iter;
		if (map->keycmp(link->key, key))
			return iter;
		iter = &(*iter)->next;
	}

	return iter;
}

struct hmap_link *
hmap_link_pop(const struct hmap *map, struct hmap_key key)
{
	struct hmap_link **iter;

	LIBHMAP_ABORT_ON_ARGS(!map);

	iter = hmap_link_get(map, key);
	if (iter) {
		*iter = (*iter)->next;
		return *iter;
	}

	return NULL;
}

int
hmap_link_alloc(const struct hmap *map, struct hmap_link **out,
	struct hmap_key key, struct hmap_val value)
{
	struct hmap_link *link;
	int rc;

	LIBHMAP_ABORT_ON_ARGS(!map || !out);

	rc = map->allocator->alloc((void **)&link, sizeof(struct hmap_link));
	if (rc) return -rc;
	link->key = key;
	link->value = value;
	link->next = NULL;
	*out = link;

	return 0;
}

struct hmap_link *
hmap_get(const struct hmap *map, struct hmap_key key)
{
	struct hmap_link **iter;

	LIBHMAP_ABORT_ON_ARGS(!map);

	iter = hmap_link_get(map, key);

	return iter ? *iter : NULL;
}

int
hmap_set(const struct hmap *map, struct hmap_key key, struct hmap_val value)
{
	struct hmap_link **iter;

	LIBHMAP_ABORT_ON_ARGS(!map);

	iter = hmap_link_pos(map, key);
	if (!*iter) return HMAP_KEY_MISSING;

	(*iter)->value = value;

	return 0;
}

int
hmap_rm(const struct hmap *map, struct hmap_key key)
{
	struct hmap_link *link;
	int rc;

	LIBHMAP_ABORT_ON_ARGS(!map);

	link = hmap_link_pop(map, key);
	if (!link) return HMAP_KEY_MISSING;

	rc = map->allocator->free(link);
	if (rc) return -rc;

	return 0;
}

int
hmap_add(const struct hmap *map, struct hmap_key key, struct hmap_val value)
{
	struct hmap_link **iter;
	int rc;

	LIBHMAP_ABORT_ON_ARGS(!map);

	iter = hmap_link_pos(map, key);
	if (*iter) return HMAP_KEY_EXISTS;

	rc = hmap_link_alloc(map, iter, key, value);
	if (rc) return rc;

	return 0;
}

void
hmap_iter_init(struct hmap_iter *iter)
{
	LIBHMAP_ABORT_ON_ARGS(!iter);

	iter->bucket = 0;
	iter->link = NULL;
}

bool
hmap_iter_next(const struct hmap *map, struct hmap_iter *iter)
{
	size_t i;

	LIBHMAP_ABORT_ON_ARGS(!map || !iter);

	if (iter->link && iter->link->next) {
		iter->link = iter->link->next;
		return true;
	}

	i = iter->bucket + (iter->link ? 1 : 0);
	for (; i < map->bucketcnt; i++) {
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
hmap_str_hash(struct hmap_key key)
{
	const char *c;
	uint32_t hash;

	LIBHMAP_ABORT_ON_ARGS(!key.p);

	hash = 5381;
	for (c = key.p; *c; c++)
		hash = 33 * hash + (uint8_t) *c;

	return hash;
}

bool
hmap_str_keycmp(struct hmap_key k1, struct hmap_key k2)
{
	LIBHMAP_ABORT_ON_ARGS(!k1.p || !k2.p);

	return !strcmp(k1.p, k2.p);
}
