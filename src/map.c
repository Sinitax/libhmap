#include "map.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ASSERT(x) assert((x), __FILE__, __LINE__, #x)
#define OOM_CHECK(x) assert((x) != NULL, __FILE__, __LINE__, "Out of Memory")

static void assert(int cond, const char *file, int line, const char *condstr);
static void *memdup(const void *src, size_t size);

static size_t map_key_bucket(struct map *map,
	const void *key, size_t size);
static bool map_key_cmp(const void *a, size_t asize,
	const void *b, size_t bsize);

static struct map_link **map_get_linkp(struct map *map,
	const void *key, size_t size);
static struct map_link *map_link_alloc(void *key, size_t key_size,
	void *value, size_t value_size);

void
assert(int cond, const char *file, int line, const char *condstr)
{
	if (cond) return;

	fprintf(stderr, "CMAP: Assertion failed at %s:%i (%s)\n",
		file, line, condstr);
	exit(1);
}

void *
memdup(const void *src, size_t size)
{
	void *alloc;

	alloc = malloc(size);
	if (!alloc) return NULL;
	memcpy(alloc, src, size);

	return alloc;
}

size_t
map_key_bucket(struct map *map, const void *key, size_t size)
{
	ASSERT(map != NULL && key != NULL);

	return map->hash(key, size) % map->size;
}

bool
map_key_cmp(const void *a, size_t asize, const void *b, size_t bsize)
{
	ASSERT(a != NULL && asize != 0 && b != NULL && bsize != 0);

	return asize == bsize && !memcmp(a, b, asize);
}

struct map_link **
map_get_linkp(struct map *map, const void *key, size_t size)
{
	struct map_link **iter, *link;

	ASSERT(map != NULL);

	iter = &map->buckets[map_key_bucket(map, key, size)];
	while (*iter != NULL) {
		link = *iter;
		if (map_key_cmp(link->key, link->key_size, key, size))
			return iter;
		iter = &(*iter)->next;
	}

	return NULL;
}

struct map_link *
map_link_alloc(void *key, size_t key_size, void *value, size_t value_size)
{
	struct map_link *link;

	ASSERT(key != NULL && value != NULL);

	link = malloc(sizeof(struct map_link));
	OOM_CHECK(link);
	link->key = key;
	link->key_size = key_size;
	link->value = value;
	link->value_size = value_size;
	link->next = NULL;

	return link;
}

void
map_init(struct map *map, size_t size, map_hash_func hasher)
{
	ASSERT(map != NULL && size != 0 && hasher != NULL);

	map->buckets = calloc(size, sizeof(struct map_link *));
	OOM_CHECK(map->buckets);
	map->size = size;
	map->hash = hasher;
}

void
map_deinit(struct map *map)
{
	ASSERT(map != NULL);

	map_clear(map);
	free(map->buckets);
}

void
map_clear(struct map *map)
{
	struct map_iter iter;
	struct map_link *prev;
	int i;

	ASSERT(map != NULL);

	prev = NULL;
	for (MAP_ITER(map, &iter)) {
		free(iter.link->key);
		free(iter.link->value);
		free(prev);
		prev = iter.link;
	}
	free(prev);

	for (i = 0; i < map->size; i++)
		map->buckets[i] = NULL;
}

struct map_link *
map_get(struct map *map, const void *key, size_t size)
{
	struct map_link **iter;

	ASSERT(map != NULL);

	iter = map_get_linkp(map, key, size);

	return iter ? *iter : NULL;
}

struct map_link *
map_pop(struct map *map, const void *key, size_t size)
{
	struct map_link **iter;

	ASSERT(map != NULL);

	iter = map_get_linkp(map, key, size);
	if (iter) {
		*iter = (*iter)->next;
		return *iter;
	}

	return NULL;
}

void
map_link_set(struct map_link *link, void *key, size_t key_size,
	void *value, size_t value_size)
{
	ASSERT(link != NULL);

	free(link->key);
	link->key = key;
	link->key_size = key_size;

	free(link->value);
	link->value = value;
	link->value_size = value_size;
}

void
map_set(struct map *map, void *key, size_t key_size,
	void *value, size_t value_size)
{
	struct map_link **iter, *link;

	ASSERT(map != NULL);

	iter = map_get_linkp(map, key, key_size);
	if (iter == NULL) {
		iter = &map->buckets[map_key_bucket(map, key, key_size)];
		while (*iter) iter = &((*iter)->next);
	}

	if (*iter) {
		map_link_set(*iter, key, key_size, value, value_size);
	} else {
		*iter = map_link_alloc(key, key_size, value, value_size);
	}
}

void
map_iter_init(struct map_iter *iter)
{
	ASSERT(iter != NULL);

	iter->bucket = 0;
	iter->link = NULL;
}

bool
map_iter_next(struct map *map, struct map_iter *iter)
{
	int i;

	ASSERT(map != NULL && iter != NULL);

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
map_str_hash(const char *data, size_t size)
{
	uint32_t hash;
	int i;

	ASSERT(data != NULL && size > 0);

	hash = 0;
	for (i = 0; i < size; i++)
		hash = 31 * hash + data[i];

	return hash;
}

