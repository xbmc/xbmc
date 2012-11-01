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

#ifndef LIBASS_FONT_H
#define LIBASS_FONT_H

#include <stdint.h>
#include <ft2build.h>
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include "ass.h"
#include "ass_types.h"

#define VERTICAL_LOWER_BOUND 0x02f1

#define ASS_FONT_MAX_FACES 10
#define DECO_UNDERLINE 1
#define DECO_STRIKETHROUGH 2

typedef struct ass_shaper_font_data ASS_ShaperFontData;

typedef struct {
    char *family;
    unsigned bold;
    unsigned italic;
    int treat_family_as_pattern;
    int vertical;               // @font vertical layout
} ASS_FontDesc;

typedef struct {
    ASS_FontDesc desc;
    ASS_Library *library;
    FT_Library ftlibrary;
    FT_Face faces[ASS_FONT_MAX_FACES];
    ASS_ShaperFontData *shaper_priv;
    int n_faces;
    double scale_x, scale_y;    // current transform
    FT_Vector v;                // current shift
    double size;
} ASS_Font;

#include "ass_cache.h"

ASS_Font *ass_font_new(Cache *font_cache, ASS_Library *library,
                       FT_Library ftlibrary, void *fc_priv,
                       ASS_FontDesc *desc);
void ass_font_set_transform(ASS_Font *font, double scale_x,
                            double scale_y, FT_Vector *v);
void ass_face_set_size(FT_Face face, double size);
void ass_font_set_size(ASS_Font *font, double size);
void ass_font_get_asc_desc(ASS_Font *font, uint32_t ch, int *asc,
                           int *desc);
int ass_font_get_index(void *fcpriv, ASS_Font *font, uint32_t symbol,
                       int *face_index, int *glyph_index);
FT_Glyph ass_font_get_glyph(void *fontconfig_priv, ASS_Font *font,
                            uint32_t ch, int face_index, int index,
                            ASS_Hinting hinting, int deco);
FT_Vector ass_font_get_kerning(ASS_Font *font, uint32_t c1, uint32_t c2);
void ass_font_free(ASS_Font *font);
void fix_freetype_stroker(FT_Outline *outline, int border_x, int border_y);
void outline_copy(FT_Library lib, FT_Outline *source, FT_Outline **dest);
void outline_free(FT_Library lib, FT_Outline *outline);

#endif                          /* LIBASS_FONT_H */
