/* memory.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

struct cache_upcall {
	struct cache_upcall *next;
	struct cache_upcall *prev;
	int (*upcall)(int);
	unsigned char name[1];
};

struct list_head cache_upcalls = { &cache_upcalls, &cache_upcalls }; /* cache_upcall */

int shrink_memory(int type)
{
	struct cache_upcall *c;
	int a = 0;
	foreach(c, cache_upcalls) a |= c->upcall(type);
	return a;
}

void register_cache_upcall(int (*upcall)(int), unsigned char *name)
{
	struct cache_upcall *c;
	if ((c = mem_alloc(sizeof(struct cache_upcall) + strlen(name) + 1))) {
		c->upcall = upcall;
		strcpy(c->name, name);
		add_to_list(cache_upcalls, c);
	}
}

void free_all_caches()
{
	struct cache_upcall *c;
	int a;
	do {
		a = 0;
		foreach(c, cache_upcalls) a |= c->upcall(SH_FREE_ALL);
	} while (a & ST_SOMETHING_FREED);
	if (!(a & ST_CACHE_EMPTY)) {
		unsigned char *m = init_str();
		int l = 0;
		foreach(c, cache_upcalls) if (!(c->upcall(SH_FREE_ALL) & ST_CACHE_EMPTY)) {
			if (l) add_to_str(&m, &l, ", ");
			add_to_str(&m, &l, c->name);
		}
		internal("could not release entries from caches: %s", m);
		mem_free(m);
	}
	free_list(cache_upcalls);
}
