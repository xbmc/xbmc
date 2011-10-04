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

#include "config.h"

#include <inttypes.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYNTHESIS_H
#include FT_GLYPH_H
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H
#ifndef _WIN32
#include <strings.h>
#endif

#include "ass.h"
#include "ass_library.h"
#include "ass_font.h"
#include "ass_fontconfig.h"
#include "ass_utils.h"
#include "ass_shaper.h"

/**
 * Select a good charmap, prefer Microsoft Unicode charmaps.
 * Otherwise, let FreeType decide.
 */
static void charmap_magic(ASS_Library *library, FT_Face face)
{
    int i;
    int ms_cmap = -1;

    // Search for a Microsoft Unicode cmap
    for (i = 0; i < face->num_charmaps; ++i) {
        FT_CharMap cmap = face->charmaps[i];
        unsigned pid = cmap->platform_id;
        unsigned eid = cmap->encoding_id;
        if (pid == 3 /*microsoft */
            && (eid == 1 /*unicode bmp */
                || eid == 10 /*full unicode */ )) {
            FT_Set_Charmap(face, cmap);
            return;
        } else if (pid == 3 && ms_cmap < 0)
            ms_cmap = i;
    }

    // Try the first Microsoft cmap if no Microsoft Unicode cmap was found
    if (ms_cmap >= 0) {
        FT_CharMap cmap = face->charmaps[ms_cmap];
        FT_Set_Charmap(face, cmap);
        return;
    }

    if (!face->charmap) {
        if (face->num_charmaps == 0) {
            ass_msg(library, MSGL_WARN, "Font face with no charmaps");
            return;
        }
        ass_msg(library, MSGL_WARN,
                "No charmap autodetected, trying the first one");
        FT_Set_Charmap(face, face->charmaps[0]);
        return;
    }
}

/**
 * \brief find a memory font by name
 */
static int find_font(ASS_Library *library, char *name)
{
    int i;
    for (i = 0; i < library->num_fontdata; ++i)
        if (strcasecmp(name, library->fontdata[i].name) == 0)
            return i;
    return -1;
}

static void buggy_font_workaround(FT_Face face)
{
    // Some fonts have zero Ascender/Descender fields in 'hhea' table.
    // In this case, get the information from 'os2' table or, as
    // a last resort, from face.bbox.
    if (face->ascender + face->descender == 0 || face->height == 0) {
        TT_OS2 *os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
        if (os2) {
            face->ascender = os2->sTypoAscender;
            face->descender = os2->sTypoDescender;
            face->height = face->ascender - face->descender;
        } else {
            face->ascender = face->bbox.yMax;
            face->descender = face->bbox.yMin;
            face->height = face->ascender - face->descender;
        }
    }
}

/**
 * \brief Select a face with the given charcode and add it to ASS_Font
 * \return index of the new face in font->faces, -1 if failed
 */
static int add_face(void *fc_priv, ASS_Font *font, uint32_t ch)
{
    char *path;
    int index;
    FT_Face face;
    int error;
    int mem_idx;

    if (font->n_faces == ASS_FONT_MAX_FACES)
        return -1;

    path =
        fontconfig_select(font->library, fc_priv, font->desc.family,
                          font->desc.treat_family_as_pattern,
                          font->desc.bold, font->desc.italic, &index, ch);
    if (!path)
        return -1;

    mem_idx = find_font(font->library, path);
    if (mem_idx >= 0) {
        error =
            FT_New_Memory_Face(font->ftlibrary,
                               (unsigned char *) font->library->
                               fontdata[mem_idx].data,
                               font->library->fontdata[mem_idx].size, index,
                               &face);
        if (error) {
            ass_msg(font->library, MSGL_WARN,
                    "Error opening memory font: '%s'", path);
            free(path);
            return -1;
        }
    } else {
        error = FT_New_Face(font->ftlibrary, path, index, &face);
        if (error) {
            ass_msg(font->library, MSGL_WARN,
                    "Error opening font: '%s', %d", path, index);
            free(path);
            return -1;
        }
    }
    charmap_magic(font->library, face);
    buggy_font_workaround(face);

    font->faces[font->n_faces++] = face;
    ass_face_set_size(face, font->size);
    free(path);
    return font->n_faces - 1;
}

/**
 * \brief Create a new ASS_Font according to "desc" argument
 */
ASS_Font *ass_font_new(Cache *font_cache, ASS_Library *library,
                       FT_Library ftlibrary, void *fc_priv,
                       ASS_FontDesc *desc)
{
    int error;
    ASS_Font *fontp;
    ASS_Font font;

    fontp = ass_cache_get(font_cache, desc);
    if (fontp)
        return fontp;

    font.library = library;
    font.ftlibrary = ftlibrary;
    font.shaper_priv = NULL;
    font.n_faces = 0;
    font.desc.family = strdup(desc->family);
    font.desc.treat_family_as_pattern = desc->treat_family_as_pattern;
    font.desc.bold = desc->bold;
    font.desc.italic = desc->italic;
    font.desc.vertical = desc->vertical;

    font.scale_x = font.scale_y = 1.;
    font.v.x = font.v.y = 0;
    font.size = 0.;

    error = add_face(fc_priv, &font, 0);
    if (error == -1) {
        free(font.desc.family);
        return 0;
    } else
        return ass_cache_put(font_cache, &font.desc, &font);
}

/**
 * \brief Set font transformation matrix and shift vector
 **/
void ass_font_set_transform(ASS_Font *font, double scale_x,
                            double scale_y, FT_Vector *v)
{
    font->scale_x = scale_x;
    font->scale_y = scale_y;
    if (v) {
        font->v.x = v->x;
        font->v.y = v->y;
    }
}

void ass_face_set_size(FT_Face face, double size)
{
    TT_HoriHeader *hori = FT_Get_Sfnt_Table(face, ft_sfnt_hhea);
    TT_OS2 *os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    double mscale = 1.;
    FT_Size_RequestRec rq;
    FT_Size_Metrics *m = &face->size->metrics;
    // VSFilter uses metrics from TrueType OS/2 table
    // The idea was borrowed from asa (http://asa.diac24.net)
    if (hori && os2) {
        int hori_height = hori->Ascender - hori->Descender;
        int os2_height = os2->usWinAscent + os2->usWinDescent;
        if (hori_height && os2_height)
            mscale = (double) hori_height / os2_height;
    }
    memset(&rq, 0, sizeof(rq));
    rq.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
    rq.width = 0;
    rq.height = double_to_d6(size * mscale);
    rq.horiResolution = rq.vertResolution = 0;
    FT_Request_Size(face, &rq);
    m->ascender /= mscale;
    m->descender /= mscale;
    m->height /= mscale;
}

/**
 * \brief Set font size
 **/
void ass_font_set_size(ASS_Font *font, double size)
{
    int i;
    if (font->size != size) {
        font->size = size;
        for (i = 0; i < font->n_faces; ++i)
            ass_face_set_size(font->faces[i], size);
    }
}

/**
 * \brief Get maximal font ascender and descender.
 * \param ch character code
 * The values are extracted from the font face that provides glyphs for the given character
 **/
void ass_font_get_asc_desc(ASS_Font *font, uint32_t ch, int *asc,
                           int *desc)
{
    int i;
    for (i = 0; i < font->n_faces; ++i) {
        FT_Face face = font->faces[i];
        TT_OS2 *os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
        if (FT_Get_Char_Index(face, ch)) {
            int y_scale = face->size->metrics.y_scale;
            if (os2) {
                *asc = FT_MulFix(os2->usWinAscent, y_scale);
                *desc = FT_MulFix(os2->usWinDescent, y_scale);
            } else {
                *asc = FT_MulFix(face->ascender, y_scale);
                *desc = FT_MulFix(-face->descender, y_scale);
            }
            return;
        }
    }

    *asc = *desc = 0;
}

/*
 * Strike a glyph with a horizontal line; it's possible to underline it
 * and/or strike through it.  For the line's position and size, truetype
 * tables are consulted.  Obviously this relies on the data in the tables
 * being accurate.
 *
 */
static int ass_strike_outline_glyph(FT_Face face, ASS_Font *font,
                                    FT_Glyph glyph, int under, int through)
{
    TT_OS2 *os2 = FT_Get_Sfnt_Table(face, ft_sfnt_os2);
    TT_Postscript *ps = FT_Get_Sfnt_Table(face, ft_sfnt_post);
    FT_Outline *ol = &((FT_OutlineGlyph) glyph)->outline;
    FT_Vector points[4];
    int bear, advance, y_scale, i, dir;

    if (!under && !through)
        return 0;

    // Grow outline
    i = (under ? 4 : 0) + (through ? 4 : 0);
    ol->points = (FT_Vector *) realloc(ol->points, sizeof(FT_Vector) *
                         (ol->n_points + i));
    ol->tags = (char *) realloc(ol->tags, ol->n_points + i);
    i = !!under + !!through;
    ol->contours = (short *) realloc(ol->contours, sizeof(short) *
                           (ol->n_contours + i));

    // If the bearing is negative, the glyph starts left of the current
    // pen position
    bear = FFMIN(face->glyph->metrics.horiBearingX, 0);
    // We're adding half a pixel to avoid small gaps
    advance = d16_to_d6(glyph->advance.x) + 32;
    y_scale = face->size->metrics.y_scale;

    // Reverse drawing direction for non-truetype fonts
    dir = FT_Outline_Get_Orientation(ol);

    // Add points to the outline
    if (under && ps) {
        int pos, size;
        pos = FT_MulFix(ps->underlinePosition, y_scale * font->scale_y);
        size = FT_MulFix(ps->underlineThickness,
                         y_scale * font->scale_y / 2);

        if (pos > 0 || size <= 0)
            return 1;

        points[0].x = bear;
        points[0].y = pos + size;
        points[1].x = advance;
        points[1].y = pos + size;
        points[2].x = advance;
        points[2].y = pos - size;
        points[3].x = bear;
        points[3].y = pos - size;

        if (dir == FT_ORIENTATION_TRUETYPE) {
            for (i = 0; i < 4; i++) {
                ol->points[ol->n_points] = points[i];
                ol->tags[ol->n_points++] = 1;
            }
        } else {
            for (i = 3; i >= 0; i--) {
                ol->points[ol->n_points] = points[i];
                ol->tags[ol->n_points++] = 1;
            }
        }

        ol->contours[ol->n_contours++] = ol->n_points - 1;
    }

    if (through && os2) {
        int pos, size;
        pos = FT_MulFix(os2->yStrikeoutPosition, y_scale * font->scale_y);
        size = FT_MulFix(os2->yStrikeoutSize, y_scale * font->scale_y / 2);

        if (pos < 0 || size <= 0)
            return 1;

        points[0].x = bear;
        points[0].y = pos + size;
        points[1].x = advance;
        points[1].y = pos + size;
        points[2].x = advance;
        points[2].y = pos - size;
        points[3].x = bear;
        points[3].y = pos - size;

        if (dir == FT_ORIENTATION_TRUETYPE) {
            for (i = 0; i < 4; i++) {
                ol->points[ol->n_points] = points[i];
                ol->tags[ol->n_points++] = 1;
            }
        } else {
            for (i = 3; i >= 0; i--) {
                ol->points[ol->n_points] = points[i];
                ol->tags[ol->n_points++] = 1;
            }
        }

        ol->contours[ol->n_contours++] = ol->n_points - 1;
    }

    return 0;
}

void outline_copy(FT_Library lib, FT_Outline *source, FT_Outline **dest)
{
    if (source == NULL) {
        *dest = NULL;
        return;
    }
    *dest = calloc(1, sizeof(**dest));

    FT_Outline_New(lib, source->n_points, source->n_contours, *dest);
    FT_Outline_Copy(source, *dest);
}

void outline_free(FT_Library lib, FT_Outline *outline)
{
    if (outline)
        FT_Outline_Done(lib, outline);
    free(outline);
}

/**
 * Slightly embold a glyph without touching its metrics
 */
static void ass_glyph_embolden(FT_GlyphSlot slot)
{
    int str;

    if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
        return;

    str = FT_MulFix(slot->face->units_per_EM,
                    slot->face->size->metrics.y_scale) / 64;

    FT_Outline_Embolden(&slot->outline, str);
}

/**
 * \brief Get glyph and face index
 * Finds a face that has the requested codepoint and returns both face
 * and glyph index.
 */
int ass_font_get_index(void *fcpriv, ASS_Font *font, uint32_t symbol,
                       int *face_index, int *glyph_index)
{
    int index = 0;
    int i;
    FT_Face face = 0;

    *glyph_index = 0;

    if (symbol < 0x20) {
        *face_index = 0;
        return 0;
    }
    // Handle NBSP like a regular space when rendering the glyph
    if (symbol == 0xa0)
        symbol = ' ';
    if (font->n_faces == 0) {
        *face_index = 0;
        return 0;
    }

    // try with the requested face
    if (*face_index < font->n_faces) {
        face = font->faces[*face_index];
        index = FT_Get_Char_Index(face, symbol);
    }

    // not found in requested face, try all others
    for (i = 0; i < font->n_faces && index == 0; ++i) {
        face = font->faces[i];
        index = FT_Get_Char_Index(face, symbol);
        if (index)
            *face_index = i;
    }

#ifdef CONFIG_FONTCONFIG
    if (index == 0) {
        int face_idx;
        ass_msg(font->library, MSGL_INFO,
                "Glyph 0x%X not found, selecting one more "
                "font for (%s, %d, %d)", symbol, font->desc.family,
                font->desc.bold, font->desc.italic);
        face_idx = *face_index = add_face(fcpriv, font, symbol);
        if (face_idx >= 0) {
            face = font->faces[face_idx];
            index = FT_Get_Char_Index(face, symbol);
            if (index == 0 && face->num_charmaps > 0) {
                int i;
                ass_msg(font->library, MSGL_WARN,
                    "Glyph 0x%X not found, broken font? Trying all charmaps", symbol);
                for (i = 0; i < face->num_charmaps; i++) {
                    FT_Set_Charmap(face, face->charmaps[i]);
                    if ((index = FT_Get_Char_Index(face, symbol)) != 0) break;
                }
            }
            if (index == 0) {
                ass_msg(font->library, MSGL_ERR,
                        "Glyph 0x%X not found in font for (%s, %d, %d)",
                        symbol, font->desc.family, font->desc.bold,
                        font->desc.italic);
            }
        }
    }
#endif
    // FIXME: make sure we have a valid face_index. this is a HACK.
    *face_index  = FFMAX(*face_index, 0);
    *glyph_index = index;

    return 1;
}

/**
 * \brief Get a glyph
 * \param ch character code
 **/
FT_Glyph ass_font_get_glyph(void *fontconfig_priv, ASS_Font *font,
                            uint32_t ch, int face_index, int index,
                            ASS_Hinting hinting, int deco)
{
    int error;
    FT_Glyph glyph;
    FT_Matrix scale;
    FT_Outline *outl = NULL;
    FT_Face face = font->faces[face_index];
    int flags = 0;
    int vertical = font->desc.vertical;

    flags = FT_LOAD_NO_BITMAP | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH
            | FT_LOAD_IGNORE_TRANSFORM;
    switch (hinting) {
    case ASS_HINTING_NONE:
        flags |= FT_LOAD_NO_HINTING;
        break;
    case ASS_HINTING_LIGHT:
        flags |= FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LIGHT;
        break;
    case ASS_HINTING_NORMAL:
        flags |= FT_LOAD_FORCE_AUTOHINT;
        break;
    case ASS_HINTING_NATIVE:
        break;
    }

    error = FT_Load_Glyph(face, index, flags);
    if (error) {
        ass_msg(font->library, MSGL_WARN, "Error loading glyph, index %d",
                index);
        return 0;
    }
    if (!(face->style_flags & FT_STYLE_FLAG_ITALIC) &&
        (font->desc.italic > 55)) {
        FT_GlyphSlot_Oblique(face->glyph);
    }

    if (!(face->style_flags & FT_STYLE_FLAG_BOLD) &&
        (font->desc.bold > 80)) {
        ass_glyph_embolden(face->glyph);
    }
    error = FT_Get_Glyph(face->glyph, &glyph);
    if (error) {
        ass_msg(font->library, MSGL_WARN, "Error loading glyph, index %d",
                index);
        return 0;
    }

    // Rotate glyph, if needed
    if (vertical && ch >= VERTICAL_LOWER_BOUND) {
        FT_Matrix m = { 0, double_to_d16(-1.0), double_to_d16(1.0), 0 };
        TT_OS2 *os2 = (TT_OS2*) FT_Get_Sfnt_Table(face, ft_sfnt_os2);
        int desc = 0;

        if (os2)
            desc = FT_MulFix(os2->sTypoDescender, face->size->metrics.y_scale);

        FT_Outline_Translate(&((FT_OutlineGlyph) glyph)->outline, 0, -desc);
        FT_Outline_Transform(&((FT_OutlineGlyph) glyph)->outline, &m);
        FT_Outline_Translate(&((FT_OutlineGlyph) glyph)->outline,
                             face->glyph->metrics.vertAdvance, desc);
        glyph->advance.x = face->glyph->linearVertAdvance;
    }

    // Apply scaling and shift
    scale.xx = double_to_d16(font->scale_x);
    scale.xy = 0;
    scale.yx = 0;
    scale.yy  = double_to_d16(font->scale_y);

    outl = &((FT_OutlineGlyph) glyph)->outline;
    FT_Outline_Transform(outl, &scale);
    FT_Outline_Translate(outl, font->v.x, font->v.y);
    glyph->advance.x *= font->scale_x;

    ass_strike_outline_glyph(face, font, glyph, deco & DECO_UNDERLINE,
                             deco & DECO_STRIKETHROUGH);

    return glyph;
}

/**
 * \brief Get kerning for the pair of glyphs.
 **/
FT_Vector ass_font_get_kerning(ASS_Font *font, uint32_t c1, uint32_t c2)
{
    FT_Vector v = { 0, 0 };
    int i;

    if (font->desc.vertical)
        return v;

    for (i = 0; i < font->n_faces; ++i) {
        FT_Face face = font->faces[i];
        int i1 = FT_Get_Char_Index(face, c1);
        int i2 = FT_Get_Char_Index(face, c2);
        if (i1 && i2) {
            if (FT_HAS_KERNING(face))
                FT_Get_Kerning(face, i1, i2, FT_KERNING_DEFAULT, &v);
            return v;
        }
        if (i1 || i2)           // these glyphs are from different font faces, no kerning information
            return v;
    }
    return v;
}

/**
 * \brief Deallocate ASS_Font
 **/
void ass_font_free(ASS_Font *font)
{
    int i;
    for (i = 0; i < font->n_faces; ++i)
        if (font->faces[i])
            FT_Done_Face(font->faces[i]);
    if (font->shaper_priv)
        ass_shaper_font_data_free(font->shaper_priv);
    free(font->desc.family);
    free(font);
}

/**
 * \brief Calculate the cbox of a series of points
 */
static void
get_contour_cbox(FT_BBox *box, FT_Vector *points, int start, int end)
{
    int i;
    box->xMin = box->yMin = INT_MAX;
    box->xMax = box->yMax = INT_MIN;

    for (i = start; i <= end; i++) {
        box->xMin = (points[i].x < box->xMin) ? points[i].x : box->xMin;
        box->xMax = (points[i].x > box->xMax) ? points[i].x : box->xMax;
        box->yMin = (points[i].y < box->yMin) ? points[i].y : box->yMin;
        box->yMax = (points[i].y > box->yMax) ? points[i].y : box->yMax;
    }
}

/**
 * \brief Determine winding direction of a contour
 * \return direction; 0 = clockwise
 */
static int get_contour_direction(FT_Vector *points, int start, int end)
{
    int i;
    long long sum = 0;
    int x = points[start].x;
    int y = points[start].y;
    for (i = start + 1; i <= end; i++) {
        sum += x * (points[i].y - y) - y * (points[i].x - x);
        x = points[i].x;
        y = points[i].y;
    }
    sum += x * (points[start].y - y) - y * (points[start].x - x);
    return sum > 0;
}

/**
 * \brief Apply fixups to please the FreeType stroker and improve the
 * rendering result, especially in case the outline has some anomalies.
 * At the moment, the following fixes are done:
 *
 * 1. Reverse contours that have "inside" winding direction but are not
 *    contained in any other contours' cbox.
 * 2. Remove "inside" contours depending on border size, so that large
 *    borders do not reverse the winding direction, which leads to "holes"
 *    inside the border. The inside will be filled by the border of the
 *    outside contour anyway in this case.
 *
 * \param outline FreeType outline, modified in-place
 * \param border_x border size, x direction, d6 format
 * \param border_x border size, y direction, d6 format
 */
void fix_freetype_stroker(FT_Outline *outline, int border_x, int border_y)
{
    int nc = outline->n_contours;
    int begin, stop;
    char modified = 0;
    char *valid_cont = malloc(nc);
    int start = 0;
    int end = -1;
    FT_BBox *boxes = malloc(nc * sizeof(FT_BBox));
    int i, j;
    int inside_direction;

    inside_direction = FT_Outline_Get_Orientation(outline) ==
        FT_ORIENTATION_TRUETYPE;

    // create a list of cboxes of the contours
    for (i = 0; i < nc; i++) {
        start = end + 1;
        end = outline->contours[i];
        get_contour_cbox(&boxes[i], outline->points, start, end);
    }

    // for each contour, check direction and whether it's "outside"
    // or contained in another contour
    end = -1;
    for (i = 0; i < nc; i++) {
        int dir;
        start = end + 1;
        end = outline->contours[i];
        dir = get_contour_direction(outline->points, start, end);
        valid_cont[i] = 1;
        if (dir == inside_direction) {
            for (j = 0; j < nc; j++) {
                if (i == j)
                    continue;
                if (boxes[i].xMin >= boxes[j].xMin &&
                    boxes[i].xMax <= boxes[j].xMax &&
                    boxes[i].yMin >= boxes[j].yMin &&
                    boxes[i].yMax <= boxes[j].yMax)
                    goto check_inside;
            }
            /* "inside" contour but we can't find anything it could be
             * inside of - assume the font is buggy and it should be
             * an "outside" contour, and reverse it */
            for (j = 0; j < (end + 1 - start) / 2; j++) {
                FT_Vector temp = outline->points[start + j];
                char temp2 = outline->tags[start + j];
                outline->points[start + j] = outline->points[end - j];
                outline->points[end - j] = temp;
                outline->tags[start + j] = outline->tags[end - j];
                outline->tags[end - j] = temp2;
            }
            dir ^= 1;
        }
        check_inside:
        if (dir == inside_direction) {
            FT_BBox box;
            int width, height;
            get_contour_cbox(&box, outline->points, start, end);
            width = box.xMax - box.xMin;
            height = box.yMax - box.yMin;
            if (width < border_x * 2 || height < border_y * 2) {
                valid_cont[i] = 0;
                modified = 1;
            }
        }
    }

    // if we need to modify the outline, rewrite it and skip
    // the contours that we determined should be removed.
    if (modified) {
        int p = 0, c = 0;
        for (i = 0; i < nc; i++) {
            if (!valid_cont[i])
                continue;
            begin = (i == 0) ? 0 : outline->contours[i - 1] + 1;
            stop = outline->contours[i];
            for (j = begin; j <= stop; j++) {
                outline->points[p].x = outline->points[j].x;
                outline->points[p].y = outline->points[j].y;
                outline->tags[p] = outline->tags[j];
                p++;
            }
            outline->contours[c] = p - 1;
            c++;
        }
        outline->n_points = p;
        outline->n_contours = c;
    }

    free(boxes);
    free(valid_cont);
}

