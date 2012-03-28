/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 * Copyright (C) 2011 Grigori Goronzy <greg@chown.ath.cx>
 *
 * This file is part of libass.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"

#include <inttypes.h>
#include <ft2build.h>
#include FT_OUTLINE_H
#include <assert.h>

#include "ass_utils.h"
#include "ass_font.h"
#include "ass_cache.h"

// type-specific functions
// create hash/compare functions for bitmap, outline and composite cache
#define CREATE_HASH_FUNCTIONS
#include "ass_cache_template.h"
#define CREATE_COMPARISON_FUNCTIONS
#include "ass_cache_template.h"

// font cache
static unsigned font_hash(void *buf, size_t len)
{
    ASS_FontDesc *desc = buf;
    unsigned hval;
    hval = fnv_32a_str(desc->family, FNV1_32A_INIT);
    hval = fnv_32a_buf(&desc->bold, sizeof(desc->bold), hval);
    hval = fnv_32a_buf(&desc->italic, sizeof(desc->italic), hval);
    hval = fnv_32a_buf(&desc->treat_family_as_pattern,
            sizeof(desc->treat_family_as_pattern), hval);
    hval = fnv_32a_buf(&desc->vertical, sizeof(desc->vertical), hval);
    return hval;
}

static unsigned font_compare(void *key1, void *key2, size_t key_size)
{
    ASS_FontDesc *a = key1;
    ASS_FontDesc *b = key2;
    if (strcmp(a->family, b->family) != 0)
        return 0;
    if (a->bold != b->bold)
        return 0;
    if (a->italic != b->italic)
        return 0;
    if (a->treat_family_as_pattern != b->treat_family_as_pattern)
        return 0;
    if (a->vertical != b->vertical)
        return 0;
    return 1;
}

static void font_destruct(void *key, void *value)
{
    ass_font_free(value);
    free(key);
}

// bitmap cache
static void bitmap_destruct(void *key, void *value)
{
    BitmapHashValue *v = value;
    BitmapHashKey *k = key;
    if (v->bm)
        ass_free_bitmap(v->bm);
    if (v->bm_o)
        ass_free_bitmap(v->bm_o);
    if (v->bm_s)
        ass_free_bitmap(v->bm_s);
    if (k->type == BITMAP_CLIP)
        free(k->u.clip.text);
    free(key);
    free(value);
}

static size_t bitmap_size(void *value, size_t value_size)
{
    BitmapHashValue *val = value;
    if (val->bm_o)
        return val->bm_o->w * val->bm_o->h * 3;
    else if (val->bm)
        return val->bm->w * val->bm->h * 3;
    return 0;
}

static unsigned bitmap_hash(void *key, size_t key_size)
{
    BitmapHashKey *k = key;
    switch (k->type) {
        case BITMAP_OUTLINE: return outline_bitmap_hash(&k->u, key_size);
        case BITMAP_CLIP: return clip_bitmap_hash(&k->u, key_size);
        default: return 0;
    }
}

static unsigned bitmap_compare (void *a, void *b, size_t key_size)
{
    BitmapHashKey *ak = a;
    BitmapHashKey *bk = b;
    if (ak->type != bk->type) return 0;
    switch (ak->type) {
        case BITMAP_OUTLINE: return outline_bitmap_compare(&ak->u, &bk->u, key_size);
        case BITMAP_CLIP: return clip_bitmap_compare(&ak->u, &bk->u, key_size);
        default: return 0;
    }
}

// composite cache
static void composite_destruct(void *key, void *value)
{
    CompositeHashValue *v = value;
    free(v->a);
    free(v->b);
    free(key);
    free(value);
}

// outline cache

static unsigned outline_hash(void *key, size_t key_size)
{
    OutlineHashKey *k = key;
    switch (k->type) {
        case OUTLINE_GLYPH: return glyph_hash(&k->u, key_size);
        case OUTLINE_DRAWING: return drawing_hash(&k->u, key_size);
        default: return 0;
    }
}

static unsigned outline_compare(void *a, void *b, size_t key_size)
{
    OutlineHashKey *ak = a;
    OutlineHashKey *bk = b;
    if (ak->type != bk->type) return 0;
    switch (ak->type) {
        case OUTLINE_GLYPH: return glyph_compare(&ak->u, &bk->u, key_size);
        case OUTLINE_DRAWING: return drawing_compare(&ak->u, &bk->u, key_size);
        default: return 0;
    }
}

static void outline_destruct(void *key, void *value)
{
    OutlineHashValue *v = value;
    OutlineHashKey *k = key;
    if (v->outline)
        outline_free(v->lib, v->outline);
    if (v->border)
        outline_free(v->lib, v->border);
    if (k->type == OUTLINE_DRAWING)
        free(k->u.drawing.text);
    free(key);
    free(value);
}



// Cache data
typedef struct cache_item {
    void *key;
    void *value;
    struct cache_item *next;
} CacheItem;

struct cache {
    unsigned buckets;
    CacheItem **map;

    HashFunction hash_func;
    ItemSize size_func;
    HashCompare compare_func;
    CacheItemDestructor destruct_func;
    size_t key_size;
    size_t value_size;

    size_t cache_size;
    unsigned hits;
    unsigned misses;
    unsigned items;
};

// Hash for a simple (single value or array) type
static unsigned hash_simple(void *key, size_t key_size)
{
    return fnv_32a_buf(key, key_size, FNV1_32A_INIT);
}

// Comparison of a simple type
static unsigned compare_simple(void *a, void *b, size_t key_size)
{
    return memcmp(a, b, key_size) == 0;
}

// Default destructor
static void destruct_simple(void *key, void *value)
{
    free(key);
    free(value);
}


// Create a cache with type-specific hash/compare/destruct/size functions
Cache *ass_cache_create(HashFunction hash_func, HashCompare compare_func,
                        CacheItemDestructor destruct_func, ItemSize size_func,
                        size_t key_size, size_t value_size)
{
    Cache *cache = calloc(1, sizeof(*cache));
    cache->buckets = 0xFFFF;
    cache->hash_func = hash_simple;
    cache->compare_func = compare_simple;
    cache->destruct_func = destruct_simple;
    cache->size_func = size_func;
    if (hash_func)
        cache->hash_func = hash_func;
    if (compare_func)
        cache->compare_func = compare_func;
    if (destruct_func)
        cache->destruct_func = destruct_func;
    cache->key_size = key_size;
    cache->value_size = value_size;
    cache->map = calloc(cache->buckets, sizeof(CacheItem *));

    return cache;
}

void *ass_cache_put(Cache *cache, void *key, void *value)
{
    unsigned bucket = cache->hash_func(key, cache->key_size) % cache->buckets;
    CacheItem **item = &cache->map[bucket];
    while (*item)
        item = &(*item)->next;
    (*item) = calloc(1, sizeof(CacheItem));
    (*item)->key = malloc(cache->key_size);
    (*item)->value = malloc(cache->value_size);
    memcpy((*item)->key, key, cache->key_size);
    memcpy((*item)->value, value, cache->value_size);

    cache->items++;
    if (cache->size_func)
        cache->cache_size += cache->size_func(value, cache->value_size);
    else
        cache->cache_size++;

    return (*item)->value;
}

void *ass_cache_get(Cache *cache, void *key)
{
    unsigned bucket = cache->hash_func(key, cache->key_size) % cache->buckets;
    CacheItem *item = cache->map[bucket];
    while (item) {
        if (cache->compare_func(key, item->key, cache->key_size)) {
            cache->hits++;
            return item->value;
        }
        item = item->next;
    }
    cache->misses++;
    return NULL;
}

int ass_cache_empty(Cache *cache, size_t max_size)
{
    int i;

    if (cache->cache_size < max_size)
        return 0;

    for (i = 0; i < cache->buckets; i++) {
        CacheItem *item = cache->map[i];
        while (item) {
            CacheItem *next = item->next;
            cache->destruct_func(item->key, item->value);
            free(item);
            item = next;
        }
        cache->map[i] = NULL;
    }

    cache->items = cache->hits = cache->misses = cache->cache_size = 0;

    return 1;
}

void ass_cache_stats(Cache *cache, size_t *size, unsigned *hits,
                     unsigned *misses, unsigned *count)
{
    if (size)
        *size = cache->cache_size;
    if (hits)
        *hits = cache->hits;
    if (misses)
        *misses = cache->misses;
    if (count)
        *count = cache->items;
}

void ass_cache_done(Cache *cache)
{
    ass_cache_empty(cache, 0);
    free(cache->map);
    free(cache);
}

// Type-specific creation function
Cache *ass_font_cache_create(void)
{
    return ass_cache_create(font_hash, font_compare, font_destruct,
            (ItemSize)NULL, sizeof(ASS_FontDesc), sizeof(ASS_Font));
}

Cache *ass_outline_cache_create(void)
{
    return ass_cache_create(outline_hash, outline_compare, outline_destruct,
            NULL, sizeof(OutlineHashKey), sizeof(OutlineHashValue));
}

Cache *ass_glyph_metrics_cache_create(void)
{
    return ass_cache_create(glyph_metrics_hash, glyph_metrics_compare, NULL,
            (ItemSize) NULL, sizeof(GlyphMetricsHashKey),
            sizeof(GlyphMetricsHashValue));
}

Cache *ass_bitmap_cache_create(void)
{
    return ass_cache_create(bitmap_hash, bitmap_compare, bitmap_destruct,
            bitmap_size, sizeof(BitmapHashKey), sizeof(BitmapHashValue));
}

Cache *ass_composite_cache_create(void)
{
    return ass_cache_create(composite_hash, composite_compare,
            composite_destruct, (ItemSize)NULL, sizeof(CompositeHashKey),
            sizeof(CompositeHashValue));
}
