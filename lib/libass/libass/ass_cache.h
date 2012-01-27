/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
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

#ifndef LIBASS_CACHE_H
#define LIBASS_CACHE_H

#include "ass.h"
#include "ass_font.h"
#include "ass_bitmap.h"

typedef void (*HashmapItemDtor) (void *key, size_t key_size,
                                 void *value, size_t value_size);
typedef int (*HashmapKeyCompare) (void *key1, void *key2,
                                  size_t key_size);
typedef unsigned (*HashmapHash) (void *key, size_t key_size);

typedef struct hashmap_item {
    void *key;
    void *value;
    struct hashmap_item *next;
} HashmapItem;
typedef HashmapItem *hashmap_item_p;

typedef struct {
    int nbuckets;
    size_t key_size, value_size;
    hashmap_item_p *root;
    HashmapItemDtor item_dtor;      // a destructor for hashmap key/value pairs
    HashmapKeyCompare key_compare;
    HashmapHash hash;
    size_t cache_size;
    // stats
    int hit_count;
    int miss_count;
    int count;
    ASS_Library *library;
} Hashmap;

Hashmap *hashmap_init(ASS_Library *library, size_t key_size,
                      size_t value_size, int nbuckets,
                      HashmapItemDtor item_dtor,
                      HashmapKeyCompare key_compare,
                      HashmapHash hash);
void hashmap_done(Hashmap *map);
void *hashmap_insert(Hashmap *map, void *key, void *value);
void *hashmap_find(Hashmap *map, void *key);

Hashmap *ass_font_cache_init(ASS_Library *library);
ASS_Font *ass_font_cache_find(Hashmap *, ASS_FontDesc *desc);
void *ass_font_cache_add(Hashmap *, ASS_Font *font);
void ass_font_cache_done(Hashmap *);

// Create definitions for bitmap_hash_key and glyph_hash_key
#define CREATE_STRUCT_DEFINITIONS
#include "ass_cache_template.h"

typedef struct {
    Bitmap *bm;               // the actual bitmaps
    Bitmap *bm_o;
    Bitmap *bm_s;
} BitmapHashValue;

Hashmap *ass_bitmap_cache_init(ASS_Library *library);
void *cache_add_bitmap(Hashmap *, BitmapHashKey *key,
                       BitmapHashValue *val);
BitmapHashValue *cache_find_bitmap(Hashmap *bitmap_cache,
                                   BitmapHashKey *key);
Hashmap *ass_bitmap_cache_reset(Hashmap *bitmap_cache);
void ass_bitmap_cache_done(Hashmap *bitmap_cache);


typedef struct {
    unsigned char *a;
    unsigned char *b;
} CompositeHashValue;

Hashmap *ass_composite_cache_init(ASS_Library *library);
void *cache_add_composite(Hashmap *, CompositeHashKey *key,
                          CompositeHashValue *val);
CompositeHashValue *cache_find_composite(Hashmap *composite_cache,
                                         CompositeHashKey *key);
Hashmap *ass_composite_cache_reset(Hashmap *composite_cache);
void ass_composite_cache_done(Hashmap *composite_cache);


typedef struct {
    FT_Glyph glyph;
    FT_Glyph outline_glyph;
    FT_BBox bbox_scaled;        // bbox after scaling, but before rotation
    FT_Vector advance;          // 26.6, advance distance to the next bitmap in line
    int asc, desc;              // ascender/descender of a drawing
} GlyphHashValue;

Hashmap *ass_glyph_cache_init(ASS_Library *library);
void *cache_add_glyph(Hashmap *, GlyphHashKey *key,
                      GlyphHashValue *val);
GlyphHashValue *cache_find_glyph(Hashmap *glyph_cache,
                                 GlyphHashKey *key);
Hashmap *ass_glyph_cache_reset(Hashmap *glyph_cache);
void ass_glyph_cache_done(Hashmap *glyph_cache);

#endif                          /* LIBASS_CACHE_H */
