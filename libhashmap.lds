LIBHASHMAP_1.3 {
	hashmap_init;
	hashmap_deinit;
	hashmap_alloc;
	hashmap_free;

	hashmap_copy;
	hashmap_swap;
	hashmap_clear;

	hashmap_link_get;
	hashmap_link_pop;
	hashmap_link_set;
	hashmap_link_alloc;

	hashmap_get;
	hashmap_rm;
	hashmap_set;

	hashmap_iter_init;
	hashmap_iter_next;

	hashmap_str_hasher;
	hashmap_default_keycmp;
};
