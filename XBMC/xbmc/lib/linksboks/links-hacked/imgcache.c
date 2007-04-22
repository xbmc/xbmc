/* imgcache.c
 * Image cache
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "cfg.h"

#ifdef G

#include "links.h"

struct list_head image_cache = { &image_cache, &image_cache };

struct cached_image *find_cached_image(int bg, unsigned char *url, int xw, int
		yw, int scale, int aspect)
{
	struct cached_image *i;
	if (xw>=0&&yw>=0){
		/* The xw and yw is already scaled so that scale and
		 * aspect don't matter.
		 */
		foreach (i, image_cache) {
			if (i->background_color == bg
				&& !strcmp(i->url, url)
				&& i->wanted_xw==xw
				&& i->wanted_yw==yw) goto hit;
		}
	}else{
		foreach (i, image_cache) {
			if (i->background_color == bg
				&& !strcmp(i->url, url)
				&& i->wanted_xw==xw
				&& i->wanted_yw==yw
				&& i->scale==scale
				&& i->aspect==aspect) goto hit;
	}
	}
	return NULL;

hit:		
	i->refcount++;
	del_from_list(i);
	add_to_list(image_cache, i);
	return i;
}

void add_image_to_cache(struct cached_image *ci)
{
	add_to_list(image_cache, ci);
}

int image_size(struct cached_image *cimg)
{
	int siz = 100;
	switch(cimg->state){
		case 0:
		case 1:
		case 2:
		case 3:
		case 8:
		case 9:
		case 10:
		case 11:			
		break;

		case 12:
		case 14:
		siz+=cimg->width*cimg->height*cimg->buffer_bytes_per_pixel;
		if (cimg->bmp.user){
			case 13:
			case 15:	
			siz+=cimg->bmp.x*cimg->bmp.y*(drv->depth&7);
		}
		break;

#ifdef DEBUG
		default:
		fprintf(stderr,"cimg->state=%d\n",cimg->state);
		internal("Invalid cimg->state in image_size\n");
		break;
#endif /* #ifdef DEBUG */
	}
	return siz;
}

int shrink_image_cache(int u)
{
	struct cached_image *i;
	int si = 0;
	int r = 0;
	foreach(i, image_cache) if (!i->refcount) si += image_size(i);
	if (u == SH_FREE_SOMETHING) si = options_get_int("cache_images_size") + si * 1 / 4;
	while ((si >= options_get_int("cache_images_size") || u == SH_FREE_ALL) && !list_empty(image_cache)) {
		i = image_cache.prev;
		while (i->refcount) {
			i = i->prev;
			if ((void *)i == &image_cache) goto no;
		}
		r |= ST_SOMETHING_FREED;
		si -= image_size(i);
		del_from_list(i);
		img_destruct_cached_image(i);
	}
	no:
	return r | (list_empty(image_cache) ? ST_CACHE_EMPTY : 0);
}

int imgcache_info(int type)
{
	struct cached_image *i;
	int n = 0;
	foreach(i, image_cache) {
		switch (type) {
			case CI_BYTES:
				n += image_size(i);
				break;
			case CI_LOCKED:
				if (!i->refcount) break;
				/* fall through */
			case CI_FILES:
				n++;
				break;
			default:
				internal("imgcache_info: query %d", type);
		}
	}
	return n;
}

void init_imgcache()
{
	register_cache_upcall(shrink_image_cache, "imgcache");
}

/*
del_from_list ...
void img_destruct_cached_image(sturct cached_image *img);
*/

#endif
