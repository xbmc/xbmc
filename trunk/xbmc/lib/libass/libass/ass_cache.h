/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 *
 * This file is part of libass.
 *
 * libass is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libass is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libass; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIBASS_CACHE_H
#define LIBASS_CACHE_H

#include "ass.h"
#include "ass_font.h"
#include "ass_bitmap.h"

typedef void (*hashmap_item_dtor_t) (void *key, size_t key_size,
                                     void *value, size_t value_size);
typedef int (*hashmap_key_compare_t) (void *key1, void *key2,
                                      size_t key_size);
typedef unsigned (*hashmap_hash_t) (void *key, size_t key_size);

typedef struct hashmap_item {
    void *key;
    void *value;
    struct hashmap_item *next;
} hashmap_item_t;
typedef hashmap_item_t *hashmap_item_p;

typedef struct {
    int nbuckets;
    size_t key_size, value_size;
    hashmap_item_p *root;
    hashmap_item_dtor_t item_dtor;      // a destructor for hashmap key/value pairs
    hashmap_key_compare_t key_compare;
    hashmap_hash_t hash;
    size_t cache_size;
    // stats
    int hit_count;
    int miss_count;
    int count;
    ass_library_t *library;
} hashmap_t;

hashmap_t *hashmap_init(ass_library_t *library, size_t key_size,
                        size_t value_size, int nbuckets,
                        hashmap_item_dtor_t item_dtor,
                        hashmap_key_compare_t key_compare,
                        hashmap_hash_t hash);
void hashmap_done(hashmap_t *map);
void *hashmap_insert(hashmap_t *map, void *key, void *value);
void *hashmap_find(hashmap_t *map, void *key);

hashmap_t *ass_font_cache_init(ass_library_t *library);
ass_font_t *ass_font_cache_find(hashmap_t *, ass_font_desc_t *desc);
void *ass_font_cache_add(hashmap_t *, ass_font_t *font);
void ass_font_cache_done(hashmap_t *);

// Create definitions for bitmap_hash_key and glyph_hash_key
#define CREATE_STRUCT_DEFINITIONS
#include "ass_cache_template.h"

typedef struct {
    bitmap_t *bm;               // the actual bitmaps
    bitmap_t *bm_o;
    bitmap_t *bm_s;
} bitmap_hash_val_t;

hashmap_t *ass_bitmap_cache_init(ass_library_t *library);
void *cache_add_bitmap(hashmap_t *, bitmap_hash_key_t *key,
                       bitmap_hash_val_t *val);
bitmap_hash_val_t *cache_find_bitmap(hashmap_t *bitmap_cache,
                                     bitmap_hash_key_t *key);
hashmap_t *ass_bitmap_cache_reset(hashmap_t *bitmap_cache);
void ass_bitmap_cache_done(hashmap_t *bitmap_cache);


typedef struct {
    unsigned char *a;
    unsigned char *b;
} composite_hash_val_t;

hashmap_t *ass_composite_cache_init(ass_library_t *library);
void *cache_add_composite(hashmap_t *, composite_hash_key_t *key,
                          composite_hash_val_t *val);
composite_hash_val_t *cache_find_composite(hashmap_t *composite_cache,
                                           composite_hash_key_t *key);
hashmap_t *ass_composite_cache_reset(hashmap_t *composite_cache);
void ass_composite_cache_done(hashmap_t *composite_cache);


typedef struct {
    FT_Glyph glyph;
    FT_Glyph outline_glyph;
    FT_BBox bbox_scaled;        // bbox after scaling, but before rotation
    FT_Vector advance;          // 26.6, advance distance to the next bitmap in line
    int asc, desc;              // ascender/descender of a drawing
} glyph_hash_val_t;

hashmap_t *ass_glyph_cache_init(ass_library_t *library);
void *cache_add_glyph(hashmap_t *, glyph_hash_key_t *key,
                      glyph_hash_val_t *val);
glyph_hash_val_t *cache_find_glyph(hashmap_t *glyph_cache,
                                   glyph_hash_key_t *key);
hashmap_t *ass_glyph_cache_reset(hashmap_t *glyph_cache);
void ass_glyph_cache_done(hashmap_t *glyph_cache);

#endif                          /* LIBASS_CACHE_H */
