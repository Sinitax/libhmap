#include "hashmap.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int
main(int argc, const char **argv)
{
	struct hashmap hashmap;
	struct hashmap_iter iter;
	void *key, *value;
	int i;

	hashmap_init(&hashmap, 10, hashmap_str_hasher);

	for (i = 1; i < argc; i++) {
		key = strdup(argv[i]);
		value = malloc(sizeof(int));
		memcpy(value, &i, sizeof(int));
		hashmap_set(&hashmap, key, strlen(key) + 1,
			value, sizeof(int));
	}

	for (HASHMAP_ITER(&hashmap, &iter)) {
		printf("%s: %i\n", iter.link->key, *(int*)iter.link->value);
	}

	hashmap_deinit(&hashmap);
}
