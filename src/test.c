#include "hashmap.h"

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LIBHASHMAP_ERR(rc) errx(1, "libhashmap: %s", rc < 0 ? strerror(-rc) : "???")

int
main(int argc, const char **argv)
{
	struct hashmap hashmap;
	struct hashmap_iter iter;
	void *key, *value;
	int i, rc;

	rc = hashmap_init(&hashmap, 10, hashmap_str_hasher,
		hashmap_default_keycmp, &stdlib_heap_allocator);
	if (rc) LIBHASHMAP_ERR(rc);

	for (i = 1; i < argc; i++) {
		key = strdup(argv[i]);
		value = malloc(sizeof(int));
		memcpy(value, &i, sizeof(int));
		rc = hashmap_set(&hashmap, key, strlen(key) + 1,
			value, sizeof(int));
		if (rc) LIBHASHMAP_ERR(rc);
	}

	for (HASHMAP_ITER(&hashmap, &iter)) {
		printf("%s: %i\n", (char *)iter.link->key,
			*(int*)iter.link->value);
	}

	hashmap_deinit(&hashmap);
}
