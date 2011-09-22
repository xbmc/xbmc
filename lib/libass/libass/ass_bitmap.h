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

#ifndef LIBASS_BITMAP_H
#define LIBASS_BITMAP_H

#include <ft2build.h>
#include FT_GLYPH_H

#include "ass.h"

typedef struct ass_synth_priv ASS_SynthPriv;

ASS_SynthPriv *ass_synth_init(double);
void ass_synth_done(ASS_SynthPriv *priv);

typedef struct {
    int left, top;
    int w, h;                   // width, height
    unsigned char *buffer;      // w x h buffer
} Bitmap;

/**
 * \brief perform glyph rendering
 * \param glyph original glyph
 * \param outline_glyph "border" glyph, produced from original by FreeType's glyph stroker
 * \param bm_g out: pointer to the bitmap of original glyph is returned here
 * \param bm_o out: pointer to the bitmap of outline (border) glyph is returned here
 * \param bm_g out: pointer to the bitmap of glyph shadow is returned here
 * \param be 1 = produces blurred bitmaps, 0 = normal bitmaps
 */
int glyph_to_bitmap(ASS_Library *library, ASS_SynthPriv *priv_blur,
                    FT_Glyph glyph, FT_Glyph outline_glyph,
                    Bitmap **bm_g, Bitmap **bm_o, Bitmap **bm_s,
                    int be, double blur_radius, FT_Vector shadow_offset,
                    int border_style);

void ass_free_bitmap(Bitmap *bm);
int check_glyph_area(ASS_Library *library, FT_Glyph glyph);

#endif                          /* LIBASS_BITMAP_H */
