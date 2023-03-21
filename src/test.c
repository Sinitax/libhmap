#include "hmap.h"

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LIBHMAP_ERR(rc) errx(1, "libhmap: %s", \
	rc < 0 ? strerror(-rc) : hmap_err[rc])

static const char *hmap_err[] = {
	HMAP_STRERR_INIT
};

int
main(int argc, const char **argv)
{
	struct hmap hmap;
	struct hmap_iter iter;
	void *key;
	int i, rc;

	rc = hmap_init(&hmap, 10, hmap_str_hash,
		hmap_str_keycmp, &stdlib_heap_allocator);
	if (rc) LIBHMAP_ERR(rc);

	for (i = 1; i < argc; i++) {
		key = strdup(argv[i]);
		rc = hmap_add(&hmap, (struct hmap_key) {.p = key},
			(struct hmap_val) {.i = i});
		if (rc) LIBHMAP_ERR(rc);
	}

	for (HMAP_ITER(&hmap, &iter)) {
		printf("%s: %i\n", (char *)iter.link->key.p,
			iter.link->value.i);
	}

	for (HMAP_ITER(&hmap, &iter))
		free(iter.link->key.p);
	hmap_deinit(&hmap);
}
