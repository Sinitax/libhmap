#include "map.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int
main(int argc, const char **argv)
{
	struct map map;
	struct map_iter iter;
	void *key, *value;
	int i;

	map_init(&map, 10, map_str_hash);

	for (i = 1; i < argc; i++) {
		key = strdup(argv[i]);
		value = malloc(sizeof(int));
		memcpy(value, &i, sizeof(int));
		map_set(&map, key, strlen(key) + 1, value, sizeof(int));
	}

	for (MAP_ITER(&map, &iter)) {
		printf("%s: %i\n", iter.link->key, *(int*)iter.link->value);
	}

	map_deinit(&map);
}
