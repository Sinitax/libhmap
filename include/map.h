#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define MAP_ITER(map, iter) map_iter_init(iter); map_iter_next(map, iter);

typedef uint32_t (*map_hash_func)(const char *data, size_t size);

struct map_link {
	char *key;
	size_t key_size;
	char *value;
	size_t value_size;
	struct map_link *next;
};

struct map_iter {
	struct map_link *link;
	size_t bucket;
};

struct map {
	map_hash_func hash;
	size_t size;
	struct map_link **buckets;
};

void map_init(struct map *map, size_t size, map_hash_func hasher);
void map_deinit(struct map *map);

void map_clear(struct map *map);

struct map_link *map_get(struct map *map, const void *key, size_t size);
struct map_link *map_pop(struct map *map, const void *key, size_t size);

void map_link_set(struct map_link *link, void *key, size_t key_size,
	void *value, size_t value_size);
void map_set(struct map *map, void *key, size_t key_size,
	void *value, size_t value_size);

void map_iter_init(struct map_iter *iter);
bool map_iter_next(struct map *map, struct map_iter *iter);

uint32_t map_str_hash(const char *data, size_t size);

