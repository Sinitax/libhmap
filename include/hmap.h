#pragma once

#include "allocator.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define HMAP_ITER(map, iter) \
	hmap_iter_init(iter); hmap_iter_next(map, iter);

#define HMAP_STRERR_INIT \
	[HMAP_OK] = "Success", \
	[HMAP_KEY_EXISTS] = "Key exists", \
	[HMAP_KEY_MISSING] = "Key missing"

#ifdef LIBHMAP_ASSERT_ARGS
#define LIBHMAP_ABORT_ON_ARGS(cond) do { if (cond) abort(); } while (0)
#else
#define LIBHMAP_ABORT_ON_ARGS(cond)
#endif

#ifdef LIBHMAP_ASSERT_ALLOC
#define LIBHMAP_ABORT_ON_ALLOC(cond) do { if (cond) abort(); } while (0)
#else
#define LIBHMAP_ABORT_ON_ALLOC(cond)
#endif

struct hmap_key;

typedef uint32_t (*hmap_hash_func)(struct hmap_key key);
typedef bool (*hmap_keycmp_func)(struct hmap_key k1, struct hmap_key k2);

enum {
	HMAP_OK,
	HMAP_KEY_EXISTS,
	HMAP_KEY_MISSING,
};

struct hmap_key {
	union {
		bool b;
		char c;
		float f;
		double d;
		int i;
		unsigned int u;
		size_t s;
		ssize_t ss;
		const void *p;
		void *_p;
	};
};

struct hmap_val {
	union {
		bool b;
		char c;
		float f;
		double d;
		int i;
		unsigned int u;
		size_t s;
		ssize_t ss;
		const void *p;
		void *_p;
	};
};

struct hmap_link {
	struct hmap_key key;
	struct hmap_val value;
	struct hmap_link *next;
};

struct hmap_iter {
	struct hmap_link *link;
	size_t bucket;
};

struct hmap {
	hmap_hash_func hash;
	hmap_keycmp_func keycmp;
	struct hmap_link **buckets;
	size_t bucketcnt;
	const struct allocator *allocator;
};

int hmap_init(struct hmap *map, size_t size, hmap_hash_func hasher,
	hmap_keycmp_func keycmp, const struct allocator *allocator);
void hmap_deinit(struct hmap *map);

int hmap_alloc(struct hmap **map, size_t size, hmap_hash_func hasher,
	hmap_keycmp_func keycmp, const struct allocator *allocator);
void hmap_free(struct hmap *map);

int hmap_copy(struct hmap *dst, const struct hmap *src);
void hmap_swap(struct hmap *m1, struct hmap *m2);
void hmap_clear(struct hmap *map);

struct hmap_link **hmap_link_get(struct hmap *map, struct hmap_key key);
struct hmap_link **hmap_link_pos(struct hmap *map, struct hmap_key key);
struct hmap_link *hmap_link_pop(struct hmap *map, struct hmap_key key);
int hmap_link_alloc(struct hmap *map, struct hmap_link **out,
	struct hmap_key key, struct hmap_val value);

struct hmap_link *hmap_get(struct hmap *map, struct hmap_key key);
void hmap_rm(struct hmap *map, struct hmap_key key);
int hmap_set(struct hmap *map, struct hmap_key key, struct hmap_val value);
int hmap_add(struct hmap *map, struct hmap_key key, struct hmap_val value);

void hmap_iter_init(struct hmap_iter *iter);
bool hmap_iter_next(const struct hmap *map, struct hmap_iter *iter);

uint32_t hmap_str_hash(struct hmap_key key);
bool hmap_str_keycmp(struct hmap_key k1, struct hmap_key k2);
