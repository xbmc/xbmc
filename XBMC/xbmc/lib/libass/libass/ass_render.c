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

#include "config.h"

#include <assert.h>
#include <math.h>
#include <inttypes.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_GLYPH_H
#include FT_SYNTHESIS_H

#include "ass.h"
#include "ass_font.h"
#include "ass_bitmap.h"
#include "ass_cache.h"
#include "ass_utils.h"
#include "ass_fontconfig.h"
#include "ass_library.h"
#include "ass_drawing.h"

#define MAX_GLYPHS_INITIAL 1024
#define MAX_LINES_INITIAL 64
#define BLUR_MAX_RADIUS 100.0
#define MAX_BE 127
#define SUBPIXEL_MASK 63
#define SUBPIXEL_ACCURACY 7    // d6 mask for subpixel accuracy adjustment
#define GLYPH_CACHE_MAX 1000
#define BITMAP_CACHE_MAX_SIZE 50 * 1048576;

typedef struct {
    double xMin;
    double xMax;
    double yMin;
    double yMax;
} double_bbox_t;

typedef struct {
    double x;
    double y;
} double_vector_t;

typedef struct free_list {
    void *object;
    struct free_list *next;
} free_list_t;

typedef struct {
	int frame_width;
	int frame_height;
	double font_size_coeff; // font size multiplier
	double line_spacing; // additional line spacing (in frame pixels)
	int top_margin; // height of top margin. Everything except toptitles is shifted down by top_margin.
	int bottom_margin; // height of bottom margin. (frame_height - top_margin - bottom_margin) is original video height.
	int left_margin;
	int right_margin;
	int use_margins; // 0 - place all subtitles inside original frame
	                 // 1 - use margins for placing toptitles and subtitles
	double aspect; // frame aspect ratio, d_width / d_height.
    double pixel_ratio;         // pixel ratio of the source image
	ass_hinting_t hinting;

	char* default_font;
	char* default_family;
} ass_settings_t;

// a rendered event
typedef struct {
	ass_image_t* imgs;
	int top, height;
	int detect_collisions;
	int shift_direction;
	ass_event_t* event;
} event_images_t;

typedef enum { EF_NONE = 0, EF_KARAOKE, EF_KARAOKE_KF, EF_KARAOKE_KO
} effect_t;

// describes a glyph
// glyph_info_t and text_info_t are used for text centering and word-wrapping operations
typedef struct {
	unsigned symbol;
	FT_Glyph glyph;
	FT_Glyph outline_glyph;
	bitmap_t* bm; // glyph bitmap
	bitmap_t* bm_o; // outline bitmap
	bitmap_t* bm_s; // shadow bitmap
	FT_BBox bbox;
	FT_Vector pos;
	char linebreak; // the first (leading) glyph of some line ?
	uint32_t c[4]; // colors
	FT_Vector advance; // 26.6
	effect_t effect_type;
	int effect_timing; // time duration of current karaoke word
	                   // after process_karaoke_effects: distance in pixels from the glyph origin.
	                   // part of the glyph to the left of it is displayed in a different color.
	int effect_skip_timing; // delay after the end of last karaoke word
	int asc, desc; // font max ascender and descender
//	int height;
	int be; // blur edges
    double blur;                // gaussian blur
    double shadow_x;
    double shadow_y;
	double frx, fry, frz; // rotation
    double fax, fay;            // text shearing
	
	bitmap_hash_key_t hash_key;
} glyph_info_t;

typedef struct {
    double asc, desc;
} line_info_t;

typedef struct {
	glyph_info_t* glyphs;
	int length;
    line_info_t *lines;
	int n_lines;
    double height;
    int max_glyphs;
    int max_lines;
} text_info_t;


// Renderer state.
// Values like current font face, color, screen position, clipping and so on are stored here.
typedef struct {
	ass_event_t* event;
	ass_style_t* style;
	
	ass_font_t* font;
	char* font_path;
	double font_size;
    int flags;                  // decoration flags (underline/strike-through)
	
	FT_Stroker stroker;
	int alignment; // alignment overrides go here; if zero, style value will be used
	double frx, fry, frz;
    double fax, fay;            // text shearing
	enum {	EVENT_NORMAL, // "normal" top-, sub- or mid- title
		EVENT_POSITIONED, // happens after pos(,), margins are ignored
		EVENT_HSCROLL, // "Banner" transition effect, text_width is unlimited
		EVENT_VSCROLL // "Scroll up", "Scroll down" transition effects
		} evt_type;
	double pos_x, pos_y; // position
	double org_x, org_y; // origin
	char have_origin; // origin is explicitly defined; if 0, get_base_point() is used
	double scale_x, scale_y;
	double hspacing; // distance between letters, in pixels
    double border_x;              // outline width
    double border_y;
	uint32_t c[4]; // colors(Primary, Secondary, so on) in RGBA
	int clip_x0, clip_y0, clip_x1, clip_y1;
    char clip_mode;             // 1 = iclip
	char detect_collisions;
	uint32_t fade; // alpha from \fad
	char be; // blur edges
    double blur;                // gaussian blur
    double shadow_x;
    double shadow_y;
	int drawing_mode; // not implemented; when != 0 text is discarded, except for style override tags
    ass_drawing_t *drawing;     // current drawing
    ass_drawing_t *clip_drawing;// clip vector
    int clip_drawing_mode;      // 0 = regular clip, 1 = inverse clip

	effect_t effect_type;
	int effect_timing;
	int effect_skip_timing;

	enum { SCROLL_LR, // left-to-right
	       SCROLL_RL,
	       SCROLL_TB, // top-to-bottom
	       SCROLL_BT
	       } scroll_direction; // for EVENT_HSCROLL, EVENT_VSCROLL
	int scroll_shift;

	// face properties
	char* family;
	unsigned bold;
	unsigned italic;
    int treat_family_as_pattern;
	
} render_context_t;

typedef struct {
    hashmap_t *font_cache;
    hashmap_t *glyph_cache;
    hashmap_t *bitmap_cache;
    hashmap_t *composite_cache;
    size_t glyph_max;
    size_t bitmap_max_size;
} cache_store_t;

struct ass_renderer {
    ass_library_t *library;
    FT_Library ftlibrary;
    fc_instance_t *fontconfig_priv;
    ass_settings_t settings;
    int render_id;
    ass_synth_priv_t *synth_priv;

    ass_image_t *images_root;   // rendering result is stored here
    ass_image_t *prev_images_root;

    event_images_t *eimg;       // temporary buffer for sorting rendered events
    int eimg_size;              // allocated buffer size

// frame-global data
	int width, height; // screen dimensions
	int orig_height; // frame height ( = screen height - margins )
	int orig_width; // frame width ( = screen width - margins )
	int orig_height_nocrop; // frame height ( = screen height - margins + cropheight)
	int orig_width_nocrop; // frame width ( = screen width - margins + cropwidth)
	ass_track_t* track;
	long long time; // frame's timestamp, ms
	double font_scale;
	double font_scale_x; // x scale applied to all glyphs to preserve text aspect ratio
	double border_scale;

    render_context_t state;
	text_info_t text_info; 
    cache_store_t cache;

    free_list_t *free_head;
    free_list_t *free_tail;
};

struct render_priv {
	int top, height;
	int render_id;
};

static void ass_lazy_track_init(ass_renderer_t *render_priv)
{
    ass_track_t *track = render_priv->track;

	if (track->PlayResX && track->PlayResY)
		return;
	if (!track->PlayResX && !track->PlayResY) {
        ass_msg(render_priv->library, MSGL_WARN,
               "Neither PlayResX nor PlayResY defined. Assuming 384x288");
		track->PlayResX = 384;
		track->PlayResY = 288;
	} else {
        if (!track->PlayResY && track->PlayResX == 1280) {
            track->PlayResY = 1024;
            ass_msg(render_priv->library, MSGL_WARN,
                   "PlayResY undefined, setting to %d", track->PlayResY);
        } else if (!track->PlayResY) {
            track->PlayResY = track->PlayResX * 3 / 4;
            ass_msg(render_priv->library, MSGL_WARN,
                   "PlayResY undefined, setting to %d", track->PlayResY);
        } else if (!track->PlayResX && track->PlayResY == 1024) {
            track->PlayResX = 1280;
            ass_msg(render_priv->library, MSGL_WARN,
                   "PlayResX undefined, setting to %d", track->PlayResX);
		} else if (!track->PlayResX) {
            track->PlayResX = track->PlayResY * 4 / 3;
            ass_msg(render_priv->library, MSGL_WARN,
                   "PlayResX undefined, setting to %d", track->PlayResX);
		}
	}
}

ass_renderer_t* ass_renderer_init(ass_library_t* library)
{
	int error;
	FT_Library ft;
	ass_renderer_t* priv = 0;
	int vmajor, vminor, vpatch;
	
	error = FT_Init_FreeType( &ft );
	if ( error ) { 
        ass_msg(library, MSGL_FATAL, "%s failed", "FT_Init_FreeType");
		goto ass_init_exit;
	}

	FT_Library_Version(ft, &vmajor, &vminor, &vpatch);
    ass_msg(library, MSGL_V, "FreeType library version: %d.%d.%d",
	       vmajor, vminor, vpatch);
    ass_msg(library, MSGL_V, "FreeType headers version: %d.%d.%d",
	       FREETYPE_MAJOR, FREETYPE_MINOR, FREETYPE_PATCH);

	priv = calloc(1, sizeof(ass_renderer_t));
	if (!priv) {
		FT_Done_FreeType(ft);
		goto ass_init_exit;
	}

    priv->synth_priv = ass_synth_init(BLUR_MAX_RADIUS);

	priv->library = library;
	priv->ftlibrary = ft;
	// images_root and related stuff is zero-filled in calloc
	
    priv->cache.font_cache = ass_font_cache_init(library);
    priv->cache.bitmap_cache = ass_bitmap_cache_init(library);
    priv->cache.composite_cache = ass_composite_cache_init(library);
    priv->cache.glyph_cache = ass_glyph_cache_init(library);
    priv->cache.glyph_max = GLYPH_CACHE_MAX;
    priv->cache.bitmap_max_size = BITMAP_CACHE_MAX_SIZE;

    priv->text_info.max_glyphs = MAX_GLYPHS_INITIAL;
    priv->text_info.max_lines = MAX_LINES_INITIAL;
    priv->text_info.glyphs =
        calloc(MAX_GLYPHS_INITIAL, sizeof(glyph_info_t));
    priv->text_info.lines = calloc(MAX_LINES_INITIAL, sizeof(line_info_t));

ass_init_exit:
    if (priv)
        ass_msg(library, MSGL_INFO, "Init");
    else
        ass_msg(library, MSGL_ERR, "Init failed");

	return priv;
}

void ass_set_cache_limits(ass_renderer_t *render_priv, int glyph_max,
                          int bitmap_max)
{
    render_priv->cache.glyph_max = glyph_max ? glyph_max : GLYPH_CACHE_MAX;
    render_priv->cache.bitmap_max_size = bitmap_max ? 1048576 * bitmap_max :
                                         BITMAP_CACHE_MAX_SIZE;
	}

static void free_list_clear(ass_renderer_t *render_priv)
{
    if (render_priv->free_head) {
        free_list_t *item = render_priv->free_head;
        while(item) {
            free_list_t *oi = item;
            free(item->object);
            item = item->next;
            free(oi);
}
        render_priv->free_head = NULL;
    }
}

static void ass_free_images(ass_image_t *img);

void ass_renderer_done(ass_renderer_t *render_priv)
{
    ass_font_cache_done(render_priv->cache.font_cache);
    ass_bitmap_cache_done(render_priv->cache.bitmap_cache);
    ass_composite_cache_done(render_priv->cache.composite_cache);
    ass_glyph_cache_done(render_priv->cache.glyph_cache);

    ass_free_images(render_priv->images_root);
    ass_free_images(render_priv->prev_images_root);

    if (render_priv->state.stroker) {
        FT_Stroker_Done(render_priv->state.stroker);
        render_priv->state.stroker = 0;
    }
    if (render_priv && render_priv->ftlibrary)
        FT_Done_FreeType(render_priv->ftlibrary);
    if (render_priv && render_priv->fontconfig_priv)
        fontconfig_done(render_priv->fontconfig_priv);
    if (render_priv && render_priv->synth_priv)
        ass_synth_done(render_priv->synth_priv);
    if (render_priv && render_priv->eimg)
        free(render_priv->eimg);
    free(render_priv->text_info.glyphs);
    free(render_priv->text_info.lines);

    free(render_priv->settings.default_font);
    free(render_priv->settings.default_family);

    free_list_clear(render_priv);
    free(render_priv);
}

/**
 * \brief Create a new ass_image_t
 * Parameters are the same as ass_image_t fields.
 */
static ass_image_t *my_draw_bitmap(unsigned char *bitmap, int bitmap_w,
                                   int bitmap_h, int stride, int dst_x,
                                   int dst_y, uint32_t color)
{
	ass_image_t* img = calloc(1, sizeof(ass_image_t));
	
	img->w = bitmap_w;
	img->h = bitmap_h;
	img->stride = stride;
	img->bitmap = bitmap;
	img->color = color;
	img->dst_x = dst_x;
	img->dst_y = dst_y;

	return img;
}

static double x2scr_pos(ass_renderer_t *render_priv, double x);
static double y2scr_pos(ass_renderer_t *render_priv, double y);

typedef struct {
    int x0;
    int y0;
    int x1;
    int y1;
} rect_t;

/*
 * \brief Convert bitmap glyphs into ass_image_t list with inverse clipping
 *
 * Inverse clipping with the following strategy:
 * - find rectangle from (x0, y0) to (cx0, y1)
 * - find rectangle from (cx0, y0) to (cx1, cy0)
 * - find rectangle from (cx0, cy1) to (cx1, y1)
 * - find rectangle from (cx1, y0) to (x1, y1)
 * These rectangles can be invalid and in this case are discarded.
 * Afterwards, they are clipped against the screen coordinates.
 * In an additional pass, the rectangles need to be split up left/right for
 * karaoke effects.  This can result in a lot of bitmaps (6 to be exact).
 */
static ass_image_t **render_glyph_i(ass_renderer_t *render_priv,
                                    bitmap_t *bm, int dst_x, int dst_y,
                                    uint32_t color, uint32_t color2, int brk,
                                    ass_image_t **tail)
{
    int i, j, x0, y0, x1, y1, cx0, cy0, cx1, cy1, sx, sy, zx, zy;
    rect_t r[4];
    ass_image_t *img;

    dst_x += bm->left;
    dst_y += bm->top;

    // we still need to clip against screen boundaries
    zx = x2scr_pos(render_priv, 0);
    zy = y2scr_pos(render_priv, 0);
    sx = x2scr_pos(render_priv, render_priv->track->PlayResX);
    sy = y2scr_pos(render_priv, render_priv->track->PlayResY);

    x0 = 0;
    y0 = 0;
    x1 = bm->w;
    y1 = bm->h;
    cx0 = render_priv->state.clip_x0 - dst_x;
    cy0 = render_priv->state.clip_y0 - dst_y;
    cx1 = render_priv->state.clip_x1 - dst_x;
    cy1 = render_priv->state.clip_y1 - dst_y;

    // calculate rectangles and discard invalid ones while we're at it.
    i = 0;
    r[i].x0 = x0;
    r[i].y0 = y0;
    r[i].x1 = (cx0 > x1) ? x1 : cx0;
    r[i].y1 = y1;
    if (r[i].x1 > r[i].x0 && r[i].y1 > r[i].y0) i++;
    r[i].x0 = (cx0 < 0) ? x0 : cx0;
    r[i].y0 = y0;
    r[i].x1 = (cx1 > x1) ? x1 : cx1;
    r[i].y1 = (cy0 > y1) ? y1 : cy0;
    if (r[i].x1 > r[i].x0 && r[i].y1 > r[i].y0) i++;
    r[i].x0 = (cx0 < 0) ? x0 : cx0;
    r[i].y0 = (cy1 < 0) ? y0 : cy1;
    r[i].x1 = (cx1 > x1) ? x1 : cx1;
    r[i].y1 = y1;
    if (r[i].x1 > r[i].x0 && r[i].y1 > r[i].y0) i++;
    r[i].x0 = (cx1 < 0) ? x0 : cx1;
    r[i].y0 = y0;
    r[i].x1 = x1;
    r[i].y1 = y1;
    if (r[i].x1 > r[i].x0 && r[i].y1 > r[i].y0) i++;

    // clip each rectangle to screen coordinates
    for (j = 0; j < i; j++) {
        r[j].x0 = (r[j].x0 + dst_x < zx) ? zx - dst_x : r[j].x0;
        r[j].y0 = (r[j].y0 + dst_y < zy) ? zy - dst_y : r[j].y0;
        r[j].x1 = (r[j].x1 + dst_x > sx) ? sx - dst_x : r[j].x1;
        r[j].y1 = (r[j].y1 + dst_y > sy) ? sy - dst_y : r[j].y1;
    }

    // draw the rectangles
    for (j = 0; j < i; j++) {
        int lbrk = brk;
        // kick out rectangles that are invalid now
        if (r[j].x1 <= r[j].x0 || r[j].y1 <= r[j].y0)
            continue;
        // split up into left and right for karaoke, if needed
        if (lbrk > r[j].x0) {
            if (lbrk > r[j].x1) lbrk = r[j].x1;
            img = my_draw_bitmap(bm->buffer + r[j].y0 * bm->w + r[j].x0,
                lbrk - r[j].x0, r[j].y1 - r[j].y0,
                bm->w, dst_x + r[j].x0, dst_y + r[j].y0, color);
            *tail = img;
            tail = &img->next;
        }
        if (lbrk < r[j].x1) {
            if (lbrk < r[j].x0) lbrk = r[j].x0;
            img = my_draw_bitmap(bm->buffer + r[j].y0 * bm->w + lbrk,
                r[j].x1 - lbrk, r[j].y1 - r[j].y0,
                bm->w, dst_x + lbrk, dst_y + r[j].y0, color2);
            *tail = img;
            tail = &img->next;
        }
    }

    return tail;
}

/**
 * \brief convert bitmap glyph into ass_image_t struct(s)
 * \param bit freetype bitmap glyph, FT_PIXEL_MODE_GRAY
 * \param dst_x bitmap x coordinate in video frame
 * \param dst_y bitmap y coordinate in video frame
 * \param color first color, RGBA
 * \param color2 second color, RGBA
 * \param brk x coordinate relative to glyph origin, color is used to the left of brk, color2 - to the right
 * \param tail pointer to the last image's next field, head of the generated list should be stored here
 * \return pointer to the new list tail
 * Performs clipping. Uses my_draw_bitmap for actual bitmap convertion.
 */
static ass_image_t **render_glyph(ass_renderer_t *render_priv,
                                  bitmap_t *bm, int dst_x, int dst_y,
                                  uint32_t color, uint32_t color2, int brk,
                                  ass_image_t **tail)
{
	// brk is relative to dst_x
	// color = color left of brk
	// color2 = color right of brk
	int b_x0, b_y0, b_x1, b_y1; // visible part of the bitmap
	int clip_x0, clip_y0, clip_x1, clip_y1;
	int tmp;
	ass_image_t* img;

    // Inverse clipping in use?
    if (render_priv->state.clip_mode)
        return render_glyph_i(render_priv, bm, dst_x, dst_y, color, color2,
                              brk, tail);
	dst_x += bm->left;
	dst_y += bm->top;
	brk -= bm->left;
	
	// clipping
    clip_x0 = FFMINMAX(render_priv->state.clip_x0, 0, render_priv->width);
    clip_y0 = FFMINMAX(render_priv->state.clip_y0, 0, render_priv->height);
    clip_x1 = FFMINMAX(render_priv->state.clip_x1, 0, render_priv->width);
    clip_y1 = FFMINMAX(render_priv->state.clip_y1, 0, render_priv->height);
	b_x0 = 0;
	b_y0 = 0;
	b_x1 = bm->w;
	b_y1 = bm->h;
	
	tmp = dst_x - clip_x0;
	if (tmp < 0) {
        ass_msg(render_priv->library, MSGL_DBG2, "clip left");
		b_x0 = - tmp;
	}
	tmp = dst_y - clip_y0;
	if (tmp < 0) {
        ass_msg(render_priv->library, MSGL_DBG2, "clip top");
		b_y0 = - tmp;
	}
	tmp = clip_x1 - dst_x - bm->w;
	if (tmp < 0) {
        ass_msg(render_priv->library, MSGL_DBG2, "clip right");
		b_x1 = bm->w + tmp;
	}
	tmp = clip_y1 - dst_y - bm->h;
	if (tmp < 0) {
        ass_msg(render_priv->library, MSGL_DBG2, "clip bottom");
		b_y1 = bm->h + tmp;
	}
	
	if ((b_y0 >= b_y1) || (b_x0 >= b_x1))
		return tail;

	if (brk > b_x0) { // draw left part
        if (brk > b_x1)
            brk = b_x1;
		img = my_draw_bitmap(bm->buffer + bm->w * b_y0 + b_x0, 
			brk - b_x0, b_y1 - b_y0, bm->w,
			dst_x + b_x0, dst_y + b_y0, color);
		*tail = img;
		tail = &img->next;
	}
	if (brk < b_x1) { // draw right part
        if (brk < b_x0)
            brk = b_x0;
		img = my_draw_bitmap(bm->buffer + bm->w * b_y0 + brk, 
			b_x1 - brk, b_y1 - b_y0, bm->w,
			dst_x + brk, dst_y + b_y0, color2);
		*tail = img;
		tail = &img->next;
	}
	return tail;
}

/**
 * \brief Replace the bitmap buffer in ass_image_t with a copy
 * \param img ass_image_t to operate on
 * \return pointer to old bitmap buffer
 */
static unsigned char *clone_bitmap_buffer(ass_image_t *img)
{
    unsigned char *old_bitmap = img->bitmap;
    int size = img->stride * (img->h - 1) + img->w;
    img->bitmap = malloc(size);
    memcpy(img->bitmap, old_bitmap, size);
    return old_bitmap;
}

/**
 * \brief Calculate overlapping area of two consecutive bitmaps and in case they
 * overlap, composite them together
 * Mainly useful for translucent glyphs and especially borders, to avoid the
 * luminance adding up where they overlap (which looks ugly)
 */
static void
render_overlap(ass_renderer_t *render_priv, ass_image_t **last_tail,
               ass_image_t **tail, bitmap_hash_key_t *last_hash,
               bitmap_hash_key_t *hash)
{
    int left, top, bottom, right;
    int old_left, old_top, w, h, cur_left, cur_top;
    int x, y, opos, cpos;
    char m;
    composite_hash_key_t hk;
    composite_hash_val_t *hv;
    composite_hash_val_t chv;
    int ax = (*last_tail)->dst_x;
    int ay = (*last_tail)->dst_y;
    int aw = (*last_tail)->w;
    int as = (*last_tail)->stride;
    int ah = (*last_tail)->h;
    int bx = (*tail)->dst_x;
    int by = (*tail)->dst_y;
    int bw = (*tail)->w;
    int bs = (*tail)->stride;
    int bh = (*tail)->h;
    unsigned char *a;
    unsigned char *b;

    if ((*last_tail)->bitmap == (*tail)->bitmap)
        return;

    if ((*last_tail)->color != (*tail)->color)
        return;

    // Calculate overlap coordinates
    left = (ax > bx) ? ax : bx;
    top = (ay > by) ? ay : by;
    right = ((ax + aw) < (bx + bw)) ? (ax + aw) : (bx + bw);
    bottom = ((ay + ah) < (by + bh)) ? (ay + ah) : (by + bh);
    if ((right <= left) || (bottom <= top))
        return;
    old_left = left - ax;
    old_top = top - ay;
    w = right - left;
    h = bottom - top;
    cur_left = left - bx;
    cur_top = top - by;

    // Query cache
    memset(&hk, 0, sizeof(hk));
    memcpy(&hk.a, last_hash, sizeof(*last_hash));
    memcpy(&hk.b, hash, sizeof(*hash));
    hk.aw = aw;
    hk.ah = ah;
    hk.bw = bw;
    hk.bh = bh;
    hk.ax = ax;
    hk.ay = ay;
    hk.bx = bx;
    hk.by = by;
    hv = cache_find_composite(render_priv->cache.composite_cache, &hk);
    if (hv) {
        (*last_tail)->bitmap = hv->a;
        (*tail)->bitmap = hv->b;
        return;
    }
    // Allocate new bitmaps and copy over data
    a = clone_bitmap_buffer(*last_tail);
    b = clone_bitmap_buffer(*tail);

    // Composite overlapping area
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            opos = (old_top + y) * (as) + (old_left + x);
            cpos = (cur_top + y) * (bs) + (cur_left + x);
            m = (a[opos] > b[cpos]) ? a[opos] : b[cpos];
            (*last_tail)->bitmap[opos] = 0;
            (*tail)->bitmap[cpos] = m;
        }

    // Insert bitmaps into the cache
    chv.a = (*last_tail)->bitmap;
    chv.b = (*tail)->bitmap;
    cache_add_composite(render_priv->cache.composite_cache, &hk, &chv);
}

static void free_list_add(ass_renderer_t *render_priv, void *object)
{
    if (!render_priv->free_head) {
        render_priv->free_head = calloc(1, sizeof(free_list_t));
        render_priv->free_head->object = object;
        render_priv->free_tail = render_priv->free_head;
    } else {
        free_list_t *l = calloc(1, sizeof(free_list_t));
        l->object = object;
        render_priv->free_tail->next = l;
        render_priv->free_tail = render_priv->free_tail->next;
    }
}

/**
 * Iterate through a list of bitmaps and blend with clip vector, if
 * applicable. The blended bitmaps are added to a free list which is freed
 * at the start of a new frame.
 */
static void blend_vector_clip(ass_renderer_t *render_priv,
                              ass_image_t *head)
{
    FT_Glyph glyph;
    FT_BitmapGlyph clip_bm;
    ass_image_t *cur;
    ass_drawing_t *drawing = render_priv->state.clip_drawing;
    int error;

    if (!drawing)
        return;

    // Rasterize it
    FT_Glyph_Copy((FT_Glyph) drawing->glyph, &glyph);
    error = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
    if (error) {
        ass_msg(render_priv->library, MSGL_V,
            "Clip vector rasterization failed: %d. Skipping.", error);
        goto blend_vector_exit;
    }
    clip_bm = (FT_BitmapGlyph) glyph;
    clip_bm->top = -clip_bm->top;

    assert(clip_bm->bitmap.pitch >= 0);

    // Iterate through bitmaps and blend/clip them
    for (cur = head; cur; cur = cur->next) {
        int left, top, right, bottom, apos, bpos, y, x, w, h;
        int ax, ay, aw, ah, as;
        int bx, by, bw, bh, bs;
        int aleft, atop, bleft, btop;
        unsigned char *abuffer, *bbuffer, *nbuffer;

        abuffer = cur->bitmap;
        bbuffer = clip_bm->bitmap.buffer;
        ax = cur->dst_x;
        ay = cur->dst_y;
        aw = cur->w;
        ah = cur->h;
        as = cur->stride;
        bx = clip_bm->left;
        by = clip_bm->top;
        bw = clip_bm->bitmap.width;
        bh = clip_bm->bitmap.rows;
        bs = clip_bm->bitmap.pitch;

        // Calculate overlap coordinates
        left = (ax > bx) ? ax : bx;
        top = (ay > by) ? ay : by;
        right = ((ax + aw) < (bx + bw)) ? (ax + aw) : (bx + bw);
        bottom = ((ay + ah) < (by + bh)) ? (ay + ah) : (by + bh);
        aleft = left - ax;
        atop = top - ay;
        w = right - left;
        h = bottom - top;
        bleft = left - bx;
        btop = top - by;

        if (render_priv->state.clip_drawing_mode) {
            // Inverse clip
            if (ax + aw < bx || ay + ah < by || ax > bx + bw ||
                ay > by + bh) {
                continue;
            }

            // Allocate new buffer and add to free list
            nbuffer = malloc(as * ah);
            free_list_add(render_priv, nbuffer);

            // Blend together
            memcpy(nbuffer, abuffer, as * ah);
            for (y = 0; y < h; y++)
                for (x = 0; x < w; x++) {
                    apos = (atop + y) * as + aleft + x;
                    bpos = (btop + y) * bs + bleft + x;
                    nbuffer[apos] = FFMAX(0, abuffer[apos] - bbuffer[bpos]);
                }
        } else {
            // Regular clip
            if (ax + aw < bx || ay + ah < by || ax > bx + bw ||
                ay > by + bh) {
                cur->w = cur->h = 0;
                continue;
            }

            // Allocate new buffer and add to free list
            nbuffer = calloc(as, ah);
            free_list_add(render_priv, nbuffer);

            // Blend together
            for (y = 0; y < h; y++)
                for (x = 0; x < w; x++) {
                    apos = (atop + y) * as + aleft + x;
                    bpos = (btop + y) * bs + bleft + x;
                    nbuffer[apos] = (abuffer[apos] * bbuffer[bpos] + 255) >> 8;
                }
        }
        cur->bitmap = nbuffer;
    }

    // Free clip vector and its bitmap, we don't need it anymore
    FT_Done_Glyph(glyph);
blend_vector_exit:
    ass_drawing_free(render_priv->state.clip_drawing);
    render_priv->state.clip_drawing = 0;
}

/**
 * \brief Convert text_info_t struct to ass_image_t list
 * Splits glyphs in halves when needed (for \kf karaoke).
 */
static ass_image_t *render_text(ass_renderer_t *render_priv, int dst_x,
                                int dst_y)
{
	int pen_x, pen_y;
	int i;
	bitmap_t* bm;
	ass_image_t* head;
	ass_image_t** tail = &head;
    ass_image_t **last_tail = 0;
    ass_image_t **here_tail = 0;
    bitmap_hash_key_t *last_hash = 0;
    text_info_t *text_info = &render_priv->text_info;

	for (i = 0; i < text_info->length; ++i) {
		glyph_info_t* info = text_info->glyphs + i;
        if ((info->symbol == 0) || (info->symbol == '\n') || !info->bm_s
            || (info->shadow_x == 0 && info->shadow_y == 0))
			continue;

        pen_x =
            dst_x + (info->pos.x >> 6) +
            (int) (info->shadow_x * render_priv->border_scale);
        pen_y =
            dst_y + (info->pos.y >> 6) +
            (int) (info->shadow_y * render_priv->border_scale);
		bm = info->bm_s;

        here_tail = tail;
        tail =
            render_glyph(render_priv, bm, pen_x, pen_y, info->c[3], 0,
                         1000000, tail);
        if (last_tail && tail != here_tail && ((info->c[3] & 0xff) > 0))
            render_overlap(render_priv, last_tail, here_tail, last_hash,
                           &info->hash_key);
        last_tail = here_tail;
        last_hash = &info->hash_key;
	}

    last_tail = 0;
	for (i = 0; i < text_info->length; ++i) {
		glyph_info_t* info = text_info->glyphs + i;
		if ((info->symbol == 0) || (info->symbol == '\n') || !info->bm_o)
			continue;

        pen_x = dst_x + (info->pos.x >> 6);
        pen_y = dst_y + (info->pos.y >> 6);
		bm = info->bm_o;
		
        if ((info->effect_type == EF_KARAOKE_KO)
            && (info->effect_timing <= (info->bbox.xMax >> 6))) {
			// do nothing
        } else {
            here_tail = tail;
            tail =
                render_glyph(render_priv, bm, pen_x, pen_y, info->c[2],
                             0, 1000000, tail);
            if (last_tail && tail != here_tail && ((info->c[2] & 0xff) > 0))
                render_overlap(render_priv, last_tail, here_tail,
                               last_hash, &info->hash_key);
            last_tail = here_tail;
            last_hash = &info->hash_key;
	}
    }
	for (i = 0; i < text_info->length; ++i) {
		glyph_info_t* info = text_info->glyphs + i;
		if ((info->symbol == 0) || (info->symbol == '\n') || !info->bm)
			continue;

        pen_x = dst_x + (info->pos.x >> 6);
        pen_y = dst_y + (info->pos.y >> 6);
		bm = info->bm;

        if ((info->effect_type == EF_KARAOKE)
            || (info->effect_type == EF_KARAOKE_KO)) {
            if (info->effect_timing > (info->bbox.xMax >> 6))
                tail =
                    render_glyph(render_priv, bm, pen_x, pen_y,
                                 info->c[0], 0, 1000000, tail);
			else
                tail =
                    render_glyph(render_priv, bm, pen_x, pen_y,
                                 info->c[1], 0, 1000000, tail);
		} else if (info->effect_type == EF_KARAOKE_KF) {
            tail =
                render_glyph(render_priv, bm, pen_x, pen_y, info->c[0],
                             info->c[1], info->effect_timing, tail);
		} else
            tail =
                render_glyph(render_priv, bm, pen_x, pen_y, info->c[0],
                             0, 1000000, tail);
	}

	*tail = 0;
    blend_vector_clip(render_priv, head);

	return head;
}

/**
 * \brief Mapping between script and screen coordinates
 */
static double x2scr(ass_renderer_t *render_priv, double x)
{
    return x * render_priv->orig_width_nocrop /
        render_priv->track->PlayResX +
        FFMAX(render_priv->settings.left_margin, 0);
}
static double x2scr_pos(ass_renderer_t *render_priv, double x)
{
    return x * render_priv->orig_width / render_priv->track->PlayResX +
        render_priv->settings.left_margin;
}

/**
 * \brief Mapping between script and screen coordinates
 */
static double y2scr(ass_renderer_t *render_priv, double y)
{
    return y * render_priv->orig_height_nocrop /
        render_priv->track->PlayResY +
        FFMAX(render_priv->settings.top_margin, 0);
}
static double y2scr_pos(ass_renderer_t *render_priv, double y)
{
    return y * render_priv->orig_height / render_priv->track->PlayResY +
        render_priv->settings.top_margin;
}

// the same for toptitles
static double y2scr_top(ass_renderer_t *render_priv, double y)
{
    if (render_priv->settings.use_margins)
        return y * render_priv->orig_height_nocrop /
            render_priv->track->PlayResY;
	else
        return y * render_priv->orig_height_nocrop /
            render_priv->track->PlayResY +
            FFMAX(render_priv->settings.top_margin, 0);
}

// the same for subtitles
static double y2scr_sub(ass_renderer_t *render_priv, double y)
{
    if (render_priv->settings.use_margins)
        return y * render_priv->orig_height_nocrop /
            render_priv->track->PlayResY +
            FFMAX(render_priv->settings.top_margin,
                  0) + FFMAX(render_priv->settings.bottom_margin, 0);
	else
        return y * render_priv->orig_height_nocrop /
            render_priv->track->PlayResY +
            FFMAX(render_priv->settings.top_margin, 0);
}

static void compute_string_bbox(text_info_t *info, double_bbox_t *bbox)
{
	int i;
	
    if (info->length > 0) {
        bbox->xMin = 32000;
        bbox->xMax = -32000;
        bbox->yMin = -1 * info->lines[0].asc + d6_to_double(info->glyphs[0].pos.y);
        bbox->yMax = info->height - info->lines[0].asc +
                     d6_to_double(info->glyphs[0].pos.y);

        for (i = 0; i < info->length; ++i) {
            double s = d6_to_double(info->glyphs[i].pos.x);
            double e = s + d6_to_double(info->glyphs[i].advance.x);
            bbox->xMin = FFMIN(bbox->xMin, s);
            bbox->xMax = FFMAX(bbox->xMax, e);
		}
	} else
        bbox->xMin = bbox->xMax = bbox->yMin = bbox->yMax = 0.;
}


/**
 * \brief Check if starting part of (*p) matches sample. If true, shift p to the first symbol after the matching part.
 */
static inline int mystrcmp(char **p, const char *sample)
{
	int len = strlen(sample);
	if (strncmp(*p, sample, len) == 0) {
		(*p) += len;
		return 1;
	} else
		return 0;
}

static void change_font_size(ass_renderer_t *render_priv, double sz)
{
    double size = sz * render_priv->font_scale;

	if (size < 1)
		size = 1;
    else if (size > render_priv->height * 2)
        size = render_priv->height * 2;

    ass_font_set_size(render_priv->state.font, size);

    render_priv->state.font_size = sz;
}

/**
 * \brief Change current font, using setting from render_priv->state.
 */
static void update_font(ass_renderer_t *render_priv)
{
	unsigned val;
	ass_font_desc_t desc;
    desc.family = strdup(render_priv->state.family);
    desc.treat_family_as_pattern =
        render_priv->state.treat_family_as_pattern;

    val = render_priv->state.bold;
	// 0 = normal, 1 = bold, >1 = exact weight
    if (val == 1 || val == -1)
        val = 200;              // bold
    else if (val <= 0)
        val = 80;               // normal
	desc.bold = val;

    val = render_priv->state.italic;
    if (val == 1 || val == -1)
        val = 110;              // italic
    else if (val <= 0)
        val = 0;                // normal
	desc.italic = val;

    render_priv->state.font =
        ass_font_new(render_priv->cache.font_cache, render_priv->library,
                     render_priv->ftlibrary, render_priv->fontconfig_priv,
                     &desc);
	free(desc.family);
	
    if (render_priv->state.font)
        change_font_size(render_priv, render_priv->state.font_size);
}

/**
 * \brief Change border width
 * negative value resets border to style value
 */
static void change_border(ass_renderer_t *render_priv, double border_x,
                          double border_y)
{
    int bord;
    if (!render_priv->state.font)
        return;

    if (border_x < 0 && border_y < 0) {
        if (render_priv->state.style->BorderStyle == 1)
            border_x = border_y = render_priv->state.style->Outline;
			else
            border_x = border_y = 1.;
	}

    render_priv->state.border_x = border_x;
    render_priv->state.border_y = border_y;

    bord = 64 * border_x * render_priv->border_scale;
    if (bord > 0 && border_x == border_y) {
        if (!render_priv->state.stroker) {
			int error;
#if (FREETYPE_MAJOR > 2) || ((FREETYPE_MAJOR == 2) && (FREETYPE_MINOR > 1))
            error =
                FT_Stroker_New(render_priv->ftlibrary,
                               &render_priv->state.stroker);
#else // < 2.2
            error =
                FT_Stroker_New(render_priv->state.font->faces[0]->
                               memory, &render_priv->state.stroker);
#endif
			if (error) {
                ass_msg(render_priv->library, MSGL_V,
                        "failed to get stroker");
                render_priv->state.stroker = 0;
			}
		}
        if (render_priv->state.stroker)
            FT_Stroker_Set(render_priv->state.stroker, bord,
					FT_STROKER_LINECAP_ROUND,
                           FT_STROKER_LINEJOIN_ROUND, 0);
	} else {
        FT_Stroker_Done(render_priv->state.stroker);
        render_priv->state.stroker = 0;
	}
}

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)

/**
 * \brief Calculate a weighted average of two colors
 * calculates c1*(1-a) + c2*a, but separately for each component except alpha
 */
static void change_color(uint32_t* var, uint32_t new, double pwr)
{
	(*var)= ((uint32_t)(_r(*var) * (1 - pwr) + _r(new) * pwr) << 24) +
		((uint32_t)(_g(*var) * (1 - pwr) + _g(new) * pwr) << 16) +
        ((uint32_t) (_b(*var) * (1 - pwr) + _b(new) * pwr) << 8) + _a(*var);
}

// like change_color, but for alpha component only
static void change_alpha(uint32_t* var, uint32_t new, double pwr)
{
    *var =
        (_r(*var) << 24) + (_g(*var) << 16) + (_b(*var) << 8) +
        (_a(*var) * (1 - pwr) + _a(new) * pwr);
}

/**
 * \brief Multiply two alpha values
 * \param a first value
 * \param b second value
 * \return result of multiplication
 * Parameters and result are limited by 0xFF.
 */
static uint32_t mult_alpha(uint32_t a, uint32_t b)
{
	return 0xFF - (0xFF - a) * (0xFF - b) / 0xFF;
}

/**
 * \brief Calculate alpha value by piecewise linear function
 * Used for \fad, \fade implementation.
 */
static unsigned
interpolate_alpha(long long now,
		long long t1, long long t2, long long t3, long long t4,
		unsigned a1, unsigned a2, unsigned a3)
{
	unsigned a;
	double cf;
	if (now <= t1) {
		a = a1;
	} else if (now >= t4) {
		a = a3;
	} else if (now < t2) { // and > t1
		cf = ((double)(now - t1)) / (t2 - t1);
		a = a1 * (1 - cf) + a2 * cf;
	} else if (now > t3) {
		cf = ((double)(now - t3)) / (t4 - t3);
		a = a2 * (1 - cf) + a3 * cf;
	} else { // t2 <= now <= t3
		a = a2;
	}

	return a;
}

#define skip_to(x) while ((*p != (x)) && (*p != '}') && (*p != 0)) { ++p;}
#define skip(x) if (*p == (x)) ++p; else { return p; }
#define skipopt(x) if (*p == (x)) { ++p; }

/**
 * Parse a vector clip into an outline, using the proper scaling
 * parameters.  Translate it to correct for screen borders, if needed.
 */
static char *parse_vector_clip(ass_renderer_t *render_priv, char *p)
{
    int scale = 1;
    int res = 0;
    ass_drawing_t *drawing;
    render_priv->state.clip_drawing = ass_drawing_new(
        render_priv->fontconfig_priv,
        render_priv->state.font,
        render_priv->settings.hinting,
        render_priv->ftlibrary);
    drawing = render_priv->state.clip_drawing;
    skipopt('(');
    res = mystrtoi(&p, &scale);
    skipopt(',')
    if (!res)
        scale = 1;
    drawing->scale = scale;
    drawing->scale_x = render_priv->font_scale_x * render_priv->font_scale;
    drawing->scale_y = render_priv->font_scale;
    while (*p != ')' && *p != '}' && p != 0)
        ass_drawing_add_char(drawing, *p++);
    skipopt(')');
    ass_drawing_parse(drawing, 1);
    // We need to translate the clip according to screen borders
    if (render_priv->settings.left_margin != 0 ||
        render_priv->settings.top_margin != 0) {
        FT_Vector trans = {
            int_to_d6(render_priv->settings.left_margin),
            -int_to_d6(render_priv->settings.top_margin),
        };
        FT_Outline_Translate(&drawing->glyph->outline, trans.x, trans.y);
    }
    ass_msg(render_priv->library, MSGL_DBG2,
            "Parsed vector clip: scale %d, scales (%f, %f) string [%s]\n",
            scale, drawing->scale_x, drawing->scale_y, drawing->text);

    return p;
}

static void reset_render_context(ass_renderer_t *);

/**
 * \brief Parse style override tag.
 * \param p string to parse
 * \param pwr multiplier for some tag effects (comes from \t tags)
 */
static char *parse_tag(ass_renderer_t *render_priv, char *p, double pwr)
{
	skip_to('\\');
	skip('\\');
	if ((*p == '}') || (*p == 0))
		return p;

    // New tags introduced in vsfilter 2.39
    if (mystrcmp(&p, "xbord")) {
        double val;
        if (mystrtod(&p, &val))
            val = render_priv->state.border_x * (1 - pwr) + val * pwr;
        else
            val = -1.;
        change_border(render_priv, val, render_priv->state.border_y);
    } else if (mystrcmp(&p, "ybord")) {
        double val;
        if (mystrtod(&p, &val))
            val = render_priv->state.border_y * (1 - pwr) + val * pwr;
        else
            val = -1.;
        change_border(render_priv, render_priv->state.border_x, val);
    } else if (mystrcmp(&p, "xshad")) {
        double val;
        if (mystrtod(&p, &val))
            val = render_priv->state.shadow_x * (1 - pwr) + val * pwr;
        else
            val = 0.;
        render_priv->state.shadow_x = val;
    } else if (mystrcmp(&p, "yshad")) {
        double val;
        if (mystrtod(&p, &val))
            val = render_priv->state.shadow_y * (1 - pwr) + val * pwr;
        else
            val = 0.;
        render_priv->state.shadow_y = val;
    } else if (mystrcmp(&p, "fax")) {
        double val;
        if (mystrtod(&p, &val))
            render_priv->state.fax =
                val * pwr + render_priv->state.fax * (1 - pwr);
        else
            render_priv->state.fax = 0.;
    } else if (mystrcmp(&p, "fay")) {
        double val;
        if (mystrtod(&p, &val))
            render_priv->state.fay =
                val * pwr + render_priv->state.fay * (1 - pwr);
        else
            render_priv->state.fay = 0.;
    } else if (mystrcmp(&p, "iclip")) {
        int x0, y0, x1, y1;
        int res = 1;
        char *start = p;
        skipopt('(');
        res &= mystrtoi(&p, &x0);
        skipopt(',');
        res &= mystrtoi(&p, &y0);
        skipopt(',');
        res &= mystrtoi(&p, &x1);
        skipopt(',');
        res &= mystrtoi(&p, &y1);
        skipopt(')');
        if (res) {
            render_priv->state.clip_x0 =
                render_priv->state.clip_x0 * (1 - pwr) + x0 * pwr;
            render_priv->state.clip_x1 =
                render_priv->state.clip_x1 * (1 - pwr) + x1 * pwr;
            render_priv->state.clip_y0 =
                render_priv->state.clip_y0 * (1 - pwr) + y0 * pwr;
            render_priv->state.clip_y1 =
                render_priv->state.clip_y1 * (1 - pwr) + y1 * pwr;
            render_priv->state.clip_mode = 1;
        } else if (!render_priv->state.clip_drawing) {
            p = parse_vector_clip(render_priv, start);
            render_priv->state.clip_drawing_mode = 1;
        } else
            render_priv->state.clip_mode = 0;
    } else if (mystrcmp(&p, "blur")) {
        double val;
        if (mystrtod(&p, &val)) {
            val = render_priv->state.blur * (1 - pwr) + val * pwr;
            val = (val < 0) ? 0 : val;
            val = (val > BLUR_MAX_RADIUS) ? BLUR_MAX_RADIUS : val;
            render_priv->state.blur = val;
        } else
            render_priv->state.blur = 0.0;
        // ASS standard tags
    } else if (mystrcmp(&p, "fsc")) {
		char tp = *p++;
		double val;
		if (tp == 'x') {
			if (mystrtod(&p, &val)) {
				val /= 100;
                render_priv->state.scale_x =
                    render_priv->state.scale_x * (1 - pwr) + val * pwr;
			} else
                render_priv->state.scale_x =
                    render_priv->state.style->ScaleX;
		} else if (tp == 'y') {
			if (mystrtod(&p, &val)) {
				val /= 100;
                render_priv->state.scale_y =
                    render_priv->state.scale_y * (1 - pwr) + val * pwr;
			} else
                render_priv->state.scale_y =
                    render_priv->state.style->ScaleY;
		}
	} else if (mystrcmp(&p, "fsp")) {
		double val;
		if (mystrtod(&p, &val))
            render_priv->state.hspacing =
                render_priv->state.hspacing * (1 - pwr) + val * pwr;
		else
            render_priv->state.hspacing = render_priv->state.style->Spacing;
	} else if (mystrcmp(&p, "fs")) {
		double val;
		if (mystrtod(&p, &val))
            val = render_priv->state.font_size * (1 - pwr) + val * pwr;
		else
            val = render_priv->state.style->FontSize;
        if (render_priv->state.font)
            change_font_size(render_priv, val);
	} else if (mystrcmp(&p, "bord")) {
		double val;
        if (mystrtod(&p, &val)) {
            if (render_priv->state.border_x == render_priv->state.border_y)
                val = render_priv->state.border_x * (1 - pwr) + val * pwr;
        } else
			val = -1.; // reset to default
        change_border(render_priv, val, val);
	} else if (mystrcmp(&p, "move")) {
        double x1, x2, y1, y2;
		long long t1, t2, delta_t, t;
		double x, y;
		double k;
		skip('(');
        mystrtod(&p, &x1);
		skip(',');
        mystrtod(&p, &y1);
		skip(',');
        mystrtod(&p, &x2);
		skip(',');
        mystrtod(&p, &y2);
		if (*p == ',') {
			skip(',');
            mystrtoll(&p, &t1);
			skip(',');
            mystrtoll(&p, &t2);
            ass_msg(render_priv->library, MSGL_DBG2,
                   "movement6: (%f, %f) -> (%f, %f), (%" PRId64 " .. %"
                   PRId64 ")\n", x1, y1, x2, y2, (int64_t) t1,
                   (int64_t) t2);
		} else {
			t1 = 0;
            t2 = render_priv->state.event->Duration;
            ass_msg(render_priv->library, MSGL_DBG2,
                   "movement: (%f, %f) -> (%f, %f)", x1, y1, x2, y2);
		}
		skip(')');
		delta_t = t2 - t1;
        t = render_priv->time - render_priv->state.event->Start;
		if (t < t1)
			k = 0.;
		else if (t > t2)
			k = 1.;
        else
            k = ((double) (t - t1)) / delta_t;
		x = k * (x2 - x1) + x1;
		y = k * (y2 - y1) + y1;
        if (render_priv->state.evt_type != EVENT_POSITIONED) {
            render_priv->state.pos_x = x;
            render_priv->state.pos_y = y;
            render_priv->state.detect_collisions = 0;
            render_priv->state.evt_type = EVENT_POSITIONED;
        }
	} else if (mystrcmp(&p, "frx")) {
		double val;
		if (mystrtod(&p, &val)) {
			val *= M_PI / 180;
            render_priv->state.frx =
                val * pwr + render_priv->state.frx * (1 - pwr);
		} else
            render_priv->state.frx = 0.;
	} else if (mystrcmp(&p, "fry")) {
		double val;
		if (mystrtod(&p, &val)) {
			val *= M_PI / 180;
            render_priv->state.fry =
                val * pwr + render_priv->state.fry * (1 - pwr);
		} else
            render_priv->state.fry = 0.;
	} else if (mystrcmp(&p, "frz") || mystrcmp(&p, "fr")) {
		double val;
		if (mystrtod(&p, &val)) {
			val *= M_PI / 180;
            render_priv->state.frz =
                val * pwr + render_priv->state.frz * (1 - pwr);
		} else
            render_priv->state.frz =
                M_PI * render_priv->state.style->Angle / 180.;
	} else if (mystrcmp(&p, "fn")) {
		char* start = p;
		char* family;
		skip_to('\\');
		if (p > start) {
			family = malloc(p - start + 1);
			strncpy(family, start, p - start);
			family[p - start] = '\0';
		} else
            family = strdup(render_priv->state.style->FontName);
        if (render_priv->state.family)
            free(render_priv->state.family);
        render_priv->state.family = family;
        update_font(render_priv);
	} else if (mystrcmp(&p, "alpha")) {
		uint32_t val;
		int i;
        if (strtocolor(render_priv->library, &p, &val)) {
			unsigned char a = val >> 24;
			for (i = 0; i < 4; ++i)
                change_alpha(&render_priv->state.c[i], a, pwr);
		} else {
            change_alpha(&render_priv->state.c[0],
                         render_priv->state.style->PrimaryColour, pwr);
            change_alpha(&render_priv->state.c[1],
                         render_priv->state.style->SecondaryColour, pwr);
            change_alpha(&render_priv->state.c[2],
                         render_priv->state.style->OutlineColour, pwr);
            change_alpha(&render_priv->state.c[3],
                         render_priv->state.style->BackColour, pwr);
		}
		// FIXME: simplify
	} else if (mystrcmp(&p, "an")) {
		int val;
        if (mystrtoi(&p, &val) && val) {
			int v = (val - 1) / 3; // 0, 1 or 2 for vertical alignment
            ass_msg(render_priv->library, MSGL_DBG2, "an %d", val);
            if (v != 0)
                v = 3 - v;
			val = ((val - 1) % 3) + 1; // horizontal alignment
			val += v*4;
            ass_msg(render_priv->library, MSGL_DBG2, "align %d", val);
            render_priv->state.alignment = val;
		} else
            render_priv->state.alignment =
                render_priv->state.style->Alignment;
	} else if (mystrcmp(&p, "a")) {
		int val;
        if (mystrtoi(&p, &val) && val)
            render_priv->state.alignment = val;
		else
            render_priv->state.alignment =
                render_priv->state.style->Alignment;
	} else if (mystrcmp(&p, "pos")) {
        double v1, v2;
		skip('(');
        mystrtod(&p, &v1);
		skip(',');
        mystrtod(&p, &v2);
		skip(')');
        ass_msg(render_priv->library, MSGL_DBG2, "pos(%f, %f)", v1, v2);
        if (render_priv->state.evt_type == EVENT_POSITIONED) {
            ass_msg(render_priv->library, MSGL_V, "Subtitle has a new \\pos "
                   "after \\move or \\pos, ignoring");
        } else {
            render_priv->state.evt_type = EVENT_POSITIONED;
            render_priv->state.detect_collisions = 0;
            render_priv->state.pos_x = v1;
            render_priv->state.pos_y = v2;
        }
	} else if (mystrcmp(&p, "fad")) {
		int a1, a2, a3;
		long long t1, t2, t3, t4;
        if (*p == 'e')
            ++p;                // either \fad or \fade
		skip('(');
        mystrtoi(&p, &a1);
		skip(',');
        mystrtoi(&p, &a2);
		if (*p == ')') {
			// 2-argument version (\fad, according to specs)
			// a1 and a2 are fade-in and fade-out durations
			t1 = 0;
            t4 = render_priv->state.event->Duration;
			t2 = a1;
			t3 = t4 - a2;
			a1 = 0xFF;
			a2 = 0;
			a3 = 0xFF;
		} else {
			// 6-argument version (\fade)
			// a1 and a2 (and a3) are opacity values
			skip(',');
            mystrtoi(&p, &a3);
			skip(',');
            mystrtoll(&p, &t1);
			skip(',');
            mystrtoll(&p, &t2);
			skip(',');
            mystrtoll(&p, &t3);
			skip(',');
            mystrtoll(&p, &t4);
		}
		skip(')');
        render_priv->state.fade =
            interpolate_alpha(render_priv->time -
                              render_priv->state.event->Start, t1, t2,
                              t3, t4, a1, a2, a3);
	} else if (mystrcmp(&p, "org")) {
		int v1, v2;
		skip('(');
        mystrtoi(&p, &v1);
		skip(',');
        mystrtoi(&p, &v2);
		skip(')');
        ass_msg(render_priv->library, MSGL_DBG2, "org(%d, %d)", v1, v2);
        if (!render_priv->state.have_origin) {
            render_priv->state.org_x = v1;
            render_priv->state.org_y = v2;
            render_priv->state.have_origin = 1;
            render_priv->state.detect_collisions = 0;
        }
	} else if (mystrcmp(&p, "t")) {
		double v[3];
		int v1, v2;
		double v3;
		int cnt;
		long long t1, t2, t, delta_t;
		double k;
		skip('(');
		for (cnt = 0; cnt < 3; ++cnt) {
			if (*p == '\\')
				break;
			v[cnt] = strtod(p, &p);
			skip(',');
		}
		if (cnt == 3) {
            v1 = v[0];
            v2 = (v[1] < v1) ? render_priv->state.event->Duration : v[1];
            v3 = v[2];
		} else if (cnt == 2) {
            v1 = v[0];
            v2 = (v[1] < v1) ? render_priv->state.event->Duration : v[1];
            v3 = 1.;
		} else if (cnt == 1) {
            v1 = 0;
            v2 = render_priv->state.event->Duration;
            v3 = v[0];
		} else { // cnt == 0
            v1 = 0;
            v2 = render_priv->state.event->Duration;
            v3 = 1.;
		}
        render_priv->state.detect_collisions = 0;
		t1 = v1;
		t2 = v2;
		delta_t = v2 - v1;
		if (v3 < 0.)
			v3 = 0.;
        t = render_priv->time - render_priv->state.event->Start;        // FIXME: move to render_context
		if (t <= t1)
			k = 0.;
		else if (t >= t2)
			k = 1.;
		else {
			assert(delta_t != 0.);
			k = pow(((double)(t - t1)) / delta_t, v3);
		}
		while (*p == '\\')
            p = parse_tag(render_priv, p, k);   // maybe k*pwr ? no, specs forbid nested \t's
		skip_to(')'); // in case there is some unknown tag or a comment
		skip(')');
	} else if (mystrcmp(&p, "clip")) {
        char *start = p;
		int x0, y0, x1, y1;
		int res = 1;
        skipopt('(');
        res &= mystrtoi(&p, &x0);
        skipopt(',');
        res &= mystrtoi(&p, &y0);
        skipopt(',');
        res &= mystrtoi(&p, &x1);
        skipopt(',');
        res &= mystrtoi(&p, &y1);
        skipopt(')');
		if (res) {
            render_priv->state.clip_x0 =
                render_priv->state.clip_x0 * (1 - pwr) + x0 * pwr;
            render_priv->state.clip_x1 =
                render_priv->state.clip_x1 * (1 - pwr) + x1 * pwr;
            render_priv->state.clip_y0 =
                render_priv->state.clip_y0 * (1 - pwr) + y0 * pwr;
            render_priv->state.clip_y1 =
                render_priv->state.clip_y1 * (1 - pwr) + y1 * pwr;
        // Might be a vector clip
        } else if (!render_priv->state.clip_drawing) {
            p = parse_vector_clip(render_priv, start);
            render_priv->state.clip_drawing_mode = 0;
		} else {
            render_priv->state.clip_x0 = 0;
            render_priv->state.clip_y0 = 0;
            render_priv->state.clip_x1 = render_priv->track->PlayResX;
            render_priv->state.clip_y1 = render_priv->track->PlayResY;
		}
	} else if (mystrcmp(&p, "c")) {
		uint32_t val;
        if (!strtocolor(render_priv->library, &p, &val))
            val = render_priv->state.style->PrimaryColour;
        ass_msg(render_priv->library, MSGL_DBG2, "color: %X", val);
        change_color(&render_priv->state.c[0], val, pwr);
    } else if ((*p >= '1') && (*p <= '4') && (++p)
               && (mystrcmp(&p, "c") || mystrcmp(&p, "a"))) {
		char n = *(p-2);
		int cidx = n - '1';
		char cmd = *(p-1);
		uint32_t val;
		assert((n >= '1') && (n <= '4'));
        if (!strtocolor(render_priv->library, &p, &val))
			switch(n) {
            case '1':
                val = render_priv->state.style->PrimaryColour;
                break;
            case '2':
                val = render_priv->state.style->SecondaryColour;
                break;
            case '3':
                val = render_priv->state.style->OutlineColour;
                break;
            case '4':
                val = render_priv->state.style->BackColour;
                break;
            default:
                val = 0;
                break;          // impossible due to assert; avoid compilation warning
			}
		switch (cmd) {
        case 'c':
            change_color(render_priv->state.c + cidx, val, pwr);
            break;
        case 'a':
            change_alpha(render_priv->state.c + cidx, val >> 24, pwr);
            break;
        default:
            ass_msg(render_priv->library, MSGL_WARN, "Bad command: %c%c",
                    n, cmd);
            break;
		}
        ass_msg(render_priv->library, MSGL_DBG2, "single c/a at %f: %c%c = %X",
               pwr, n, cmd, render_priv->state.c[cidx]);
	} else if (mystrcmp(&p, "r")) {
        reset_render_context(render_priv);
	} else if (mystrcmp(&p, "be")) {
		int val;
        if (mystrtoi(&p, &val)) {
            // Clamp to a safe upper limit, since high values need excessive CPU
            val = (val < 0) ? 0 : val;
            val = (val > MAX_BE) ? MAX_BE : val;
            render_priv->state.be = val;
        } else
            render_priv->state.be = 0;
	} else if (mystrcmp(&p, "b")) {
		int b;
        if (mystrtoi(&p, &b)) {
			if (pwr >= .5)
                render_priv->state.bold = b;
		} else
            render_priv->state.bold = render_priv->state.style->Bold;
        update_font(render_priv);
	} else if (mystrcmp(&p, "i")) {
		int i;
        if (mystrtoi(&p, &i)) {
			if (pwr >= .5)
                render_priv->state.italic = i;
		} else
            render_priv->state.italic = render_priv->state.style->Italic;
        update_font(render_priv);
	} else if (mystrcmp(&p, "kf") || mystrcmp(&p, "K")) {
        int val = 0;
        mystrtoi(&p, &val);
        render_priv->state.effect_type = EF_KARAOKE_KF;
        if (render_priv->state.effect_timing)
            render_priv->state.effect_skip_timing +=
                render_priv->state.effect_timing;
        render_priv->state.effect_timing = val * 10;
	} else if (mystrcmp(&p, "ko")) {
        int val = 0;
        mystrtoi(&p, &val);
        render_priv->state.effect_type = EF_KARAOKE_KO;
        if (render_priv->state.effect_timing)
            render_priv->state.effect_skip_timing +=
                render_priv->state.effect_timing;
        render_priv->state.effect_timing = val * 10;
	} else if (mystrcmp(&p, "k")) {
        int val = 0;
        mystrtoi(&p, &val);
        render_priv->state.effect_type = EF_KARAOKE;
        if (render_priv->state.effect_timing)
            render_priv->state.effect_skip_timing +=
                render_priv->state.effect_timing;
        render_priv->state.effect_timing = val * 10;
	} else if (mystrcmp(&p, "shad")) {
        double val;
        if (mystrtod(&p, &val)) {
            if (render_priv->state.shadow_x == render_priv->state.shadow_y)
                val = render_priv->state.shadow_x * (1 - pwr) + val * pwr;
        } else
            val = 0.;
        render_priv->state.shadow_x = render_priv->state.shadow_y = val;
    } else if (mystrcmp(&p, "s")) {
		int val;
        if (mystrtoi(&p, &val) && val)
            render_priv->state.flags |= DECO_STRIKETHROUGH;
		else
            render_priv->state.flags &= ~DECO_STRIKETHROUGH;
    } else if (mystrcmp(&p, "u")) {
        int val;
        if (mystrtoi(&p, &val) && val)
            render_priv->state.flags |= DECO_UNDERLINE;
        else
            render_priv->state.flags &= ~DECO_UNDERLINE;
	} else if (mystrcmp(&p, "pbo")) {
        double val = 0;
        if (mystrtod(&p, &val))
            render_priv->state.drawing->pbo = val;
	} else if (mystrcmp(&p, "p")) {
		int val;
        if (!mystrtoi(&p, &val))
			val = 0;
        if (val)
            render_priv->state.drawing->scale = val;
        render_priv->state.drawing_mode = !!val;
	}

	return p;

#undef skip
#undef skipopt
#undef skip_to
}

/**
 * \brief Get next ucs4 char from string, parsing and executing style overrides
 * \param str string pointer
 * \return ucs4 code of the next char
 * On return str points to the unparsed part of the string
 */
static unsigned get_next_char(ass_renderer_t *render_priv, char **str)
{
	char* p = *str;
	unsigned chr;
	if (*p == '{') { // '\0' goes here
		p++;
		while (1) {
            p = parse_tag(render_priv, p, 1.);
			if (*p == '}') { // end of tag
				p++;
				if (*p == '{') {
					p++;
					continue;
				} else
					break;
			} else if (*p != '\\')
                ass_msg(render_priv->library, MSGL_V,
                        "Unable to parse: '%s'", p);
			if (*p == 0)
				break;
		}
	}
	if (*p == '\t') {
		++p;
		*str = p;
		return ' ';
	}
	if (*p == '\\') {
        if ((*(p + 1) == 'N')
            || ((*(p + 1) == 'n')
                && (render_priv->track->WrapStyle == 2))) {
			p += 2;
			*str = p;
			return '\n';
		} else if ((*(p+1) == 'n') || (*(p+1) == 'h')) {
			p += 2;
			*str = p;
			return ' ';
		}
	}
    chr = ass_utf8_get_char((char **) &p);
	*str = p;
	return chr;
}

static void
apply_transition_effects(ass_renderer_t *render_priv, ass_event_t *event)
{
	int v[4];
	int cnt;
	char* p = event->Effect;

    if (!p || !*p)
        return;

	cnt = 0;
	while (cnt < 4 && (p = strchr(p, ';'))) {
		v[cnt++] = atoi(++p);
	}
	
	if (strncmp(event->Effect, "Banner;", 7) == 0) {
		int delay;
		if (cnt < 1) {
            ass_msg(render_priv->library, MSGL_V,
                    "Error parsing effect: '%s'", event->Effect);
			return;
		}
		if (cnt >= 2 && v[1] == 0) // right-to-left
            render_priv->state.scroll_direction = SCROLL_RL;
		else // left-to-right
            render_priv->state.scroll_direction = SCROLL_LR;

		delay = v[0];
        if (delay == 0)
            delay = 1;          // ?
        render_priv->state.scroll_shift =
            (render_priv->time - render_priv->state.event->Start) / delay;
        render_priv->state.evt_type = EVENT_HSCROLL;
		return;
	}

	if (strncmp(event->Effect, "Scroll up;", 10) == 0) {
        render_priv->state.scroll_direction = SCROLL_BT;
	} else if (strncmp(event->Effect, "Scroll down;", 12) == 0) {
        render_priv->state.scroll_direction = SCROLL_TB;
	} else {
        ass_msg(render_priv->library, MSGL_V,
                "Unknown transition effect: '%s'", event->Effect);
		return;
	}
	// parse scroll up/down parameters
	{
		int delay;
		int y0, y1;
		if (cnt < 3) {
            ass_msg(render_priv->library, MSGL_V,
                    "Error parsing effect: '%s'", event->Effect);
			return;
		}
		delay = v[2];
        if (delay == 0)
            delay = 1;          // ?
        render_priv->state.scroll_shift =
            (render_priv->time - render_priv->state.event->Start) / delay;
		if (v[0] < v[1]) {
            y0 = v[0];
            y1 = v[1];
		} else {
            y0 = v[1];
            y1 = v[0];
		}
		if (y1 == 0)
            y1 = render_priv->track->PlayResY;  // y0=y1=0 means fullscreen scrolling
        render_priv->state.clip_y0 = y0;
        render_priv->state.clip_y1 = y1;
        render_priv->state.evt_type = EVENT_VSCROLL;
        render_priv->state.detect_collisions = 0;
	}

}

/**
 * \brief partially reset render_context to style values
 * Works like {\r}: resets some style overrides
 */
static void reset_render_context(ass_renderer_t *render_priv)
{
    render_priv->state.c[0] = render_priv->state.style->PrimaryColour;
    render_priv->state.c[1] = render_priv->state.style->SecondaryColour;
    render_priv->state.c[2] = render_priv->state.style->OutlineColour;
    render_priv->state.c[3] = render_priv->state.style->BackColour;
    render_priv->state.flags =
        (render_priv->state.style->Underline ? DECO_UNDERLINE : 0) |
        (render_priv->state.style->StrikeOut ? DECO_STRIKETHROUGH : 0);
    render_priv->state.font_size = render_priv->state.style->FontSize;

    free(render_priv->state.family);
    render_priv->state.family = NULL;
    render_priv->state.family = strdup(render_priv->state.style->FontName);
    render_priv->state.treat_family_as_pattern =
        render_priv->state.style->treat_fontname_as_pattern;
    render_priv->state.bold = render_priv->state.style->Bold;
    render_priv->state.italic = render_priv->state.style->Italic;
    update_font(render_priv);

    change_border(render_priv, -1., -1.);
    render_priv->state.scale_x = render_priv->state.style->ScaleX;
    render_priv->state.scale_y = render_priv->state.style->ScaleY;
    render_priv->state.hspacing = render_priv->state.style->Spacing;
    render_priv->state.be = 0;
    render_priv->state.blur = 0.0;
    render_priv->state.shadow_x = render_priv->state.style->Shadow;
    render_priv->state.shadow_y = render_priv->state.style->Shadow;
    render_priv->state.frx = render_priv->state.fry = 0.;
    render_priv->state.frz = M_PI * render_priv->state.style->Angle / 180.;
    render_priv->state.fax = render_priv->state.fay = 0.;

	// FIXME: does not reset unsupported attributes.
}

/**
 * \brief Start new event. Reset render_priv->state.
 */
static void
init_render_context(ass_renderer_t *render_priv, ass_event_t *event)
{
    render_priv->state.event = event;
    render_priv->state.style = render_priv->track->styles + event->Style;

    reset_render_context(render_priv);

    render_priv->state.evt_type = EVENT_NORMAL;
    render_priv->state.alignment = render_priv->state.style->Alignment;
    render_priv->state.pos_x = 0;
    render_priv->state.pos_y = 0;
    render_priv->state.org_x = 0;
    render_priv->state.org_y = 0;
    render_priv->state.have_origin = 0;
    render_priv->state.clip_x0 = 0;
    render_priv->state.clip_y0 = 0;
    render_priv->state.clip_x1 = render_priv->track->PlayResX;
    render_priv->state.clip_y1 = render_priv->track->PlayResY;
    render_priv->state.detect_collisions = 1;
    render_priv->state.fade = 0;
    render_priv->state.drawing_mode = 0;
    render_priv->state.effect_type = EF_NONE;
    render_priv->state.effect_timing = 0;
    render_priv->state.effect_skip_timing = 0;
    render_priv->state.drawing =
        ass_drawing_new(render_priv->fontconfig_priv,
                        render_priv->state.font,
                        render_priv->settings.hinting,
                        render_priv->ftlibrary);
	
    apply_transition_effects(render_priv, event);
}

static void free_render_context(ass_renderer_t *render_priv)
{
    free(render_priv->state.family);
    ass_drawing_free(render_priv->state.drawing);

    render_priv->state.family = NULL;
    render_priv->state.drawing = NULL;
}

// Calculate the cbox of a series of points
static void
get_contour_cbox(FT_BBox *box, FT_Vector *points, int start, int end)
{
    int i;
    box->xMin = box->yMin = INT_MAX;
    box->xMax = box->yMax = INT_MIN;

    for (i = start; i < end; i++) {
        box->xMin = (points[i].x < box->xMin) ? points[i].x : box->xMin;
        box->xMax = (points[i].x > box->xMax) ? points[i].x : box->xMax;
        box->yMin = (points[i].y < box->yMin) ? points[i].y : box->yMin;
        box->yMax = (points[i].y > box->yMax) ? points[i].y : box->yMax;
    }
}

/**
 * \brief Fix-up stroker result for huge borders by removing the contours from
 * the outline that are harmful.
*/
static void fix_freetype_stroker(FT_OutlineGlyph glyph, int border_x,
                                 int border_y)
{
    int nc = glyph->outline.n_contours;
    int begin, stop;
    char modified = 0;
    char *valid_cont;
    int start = 0;
    int end = -1;
    FT_BBox *boxes = calloc(nc, sizeof(FT_BBox));
    int i, j;

    // Create a list of cboxes of the contours
    for (i = 0; i < nc; i++) {
        start = end + 1;
        end = glyph->outline.contours[i];
        get_contour_cbox(&boxes[i], glyph->outline.points, start, end);
    }

    // if a) contour's cbox is contained in another contours cbox
    //    b) contour's height or width is smaller than the border*2
    // the contour can be safely removed.
    valid_cont = calloc(1, nc);
    for (i = 0; i < nc; i++) {
        valid_cont[i] = 1;
        for (j = 0; j < nc; j++) {
            if (i == j)
                continue;
            if (boxes[i].xMin >= boxes[j].xMin &&
                boxes[i].xMax <= boxes[j].xMax &&
                boxes[i].yMin >= boxes[j].yMin &&
                boxes[i].yMax <= boxes[j].yMax) {
                int width = boxes[i].xMax - boxes[i].xMin;
                int height = boxes[i].yMax - boxes[i].yMin;
                if (width < border_x * 2 || height < border_y * 2) {
                    valid_cont[i] = 0;
                    modified = 1;
                    break;
                }
            }
        }
    }

    // Zero-out contours that can be removed; much simpler than copying
    if (modified) {
        for (i = 0; i < nc; i++) {
            if (valid_cont[i])
                continue;
            begin = (i == 0) ? 0 : glyph->outline.contours[i - 1] + 1;
            stop = glyph->outline.contours[i];
            for (j = begin; j <= stop; j++) {
                glyph->outline.points[j].x = 0;
                glyph->outline.points[j].y = 0;
                glyph->outline.tags[j] = 0;
            }
        }
    }

    free(boxes);
    free(valid_cont);
}

/*
 * Stroke an outline glyph in x/y direction.  Applies various fixups to get
 * around limitations of the FreeType stroker.
 */
static void stroke_outline_glyph(ass_renderer_t *render_priv,
                                 FT_OutlineGlyph *glyph, int sx, int sy)
{
    if (sx <= 0 && sy <= 0)
        return;

    fix_freetype_stroker(*glyph, sx, sy);

    // Borders are equal; use the regular stroker
    if (sx == sy && render_priv->state.stroker) {
        int error;
        error = FT_Glyph_StrokeBorder((FT_Glyph *) glyph,
                                      render_priv->state.stroker, 0, 1);
        if (error)
            ass_msg(render_priv->library, MSGL_WARN,
                    "FT_Glyph_Stroke error: %d", error);

    // "Stroke" with the outline emboldener in two passes.
    // The outlines look uglier, but the emboldening never adds any points
    } else {
        int i;
        FT_Outline *ol = &(*glyph)->outline;
        FT_Outline nol;
        FT_Outline_New(render_priv->ftlibrary, ol->n_points,
                       ol->n_contours, &nol);
        FT_Outline_Copy(ol, &nol);

        FT_Outline_Embolden(ol, sx * 2);
        FT_Outline_Translate(ol, -sx, -sx);
        FT_Outline_Embolden(&nol, sy * 2);
        FT_Outline_Translate(&nol, -sy, -sy);

        for (i = 0; i < ol->n_points; i++)
            ol->points[i].y = nol.points[i].y;

        FT_Outline_Done(render_priv->ftlibrary, &nol);
    }
}

/**
 * \brief Get normal and outline (border) glyphs
 * \param symbol ucs4 char
 * \param info out: struct filled with extracted data
 * Tries to get both glyphs from cache.
 * If they can't be found, gets a glyph from font face, generates outline with FT_Stroker,
 * and add them to cache.
 * The glyphs are returned in info->glyph and info->outline_glyph
 */
static void
get_outline_glyph(ass_renderer_t *render_priv, int symbol,
                  glyph_info_t *info, ass_drawing_t *drawing)
{
	glyph_hash_val_t* val;
	glyph_hash_key_t key;
    memset(&key, 0, sizeof(key));

    if (drawing->hash) {
        key.scale_x = double_to_d16(render_priv->state.scale_x);
        key.scale_y = double_to_d16(render_priv->state.scale_y);
        key.outline.x = render_priv->state.border_x * 0xFFFF;
        key.outline.y = render_priv->state.border_y * 0xFFFF;
        key.drawing_hash = drawing->hash;
    } else {
        key.font = render_priv->state.font;
        key.size = render_priv->state.font_size;
	key.ch = symbol;
        key.bold = render_priv->state.bold;
        key.italic = render_priv->state.italic;
        key.scale_x = double_to_d16(render_priv->state.scale_x);
        key.scale_y = double_to_d16(render_priv->state.scale_y);
        key.outline.x = render_priv->state.border_x * 0xFFFF;
        key.outline.y = render_priv->state.border_y * 0xFFFF;
        key.flags = render_priv->state.flags;
    }
    memset(info, 0, sizeof(glyph_info_t));

    val = cache_find_glyph(render_priv->cache.glyph_cache, &key);
	if (val) {
		FT_Glyph_Copy(val->glyph, &info->glyph);
		if (val->outline_glyph)
			FT_Glyph_Copy(val->outline_glyph, &info->outline_glyph);
		info->bbox = val->bbox_scaled;
		info->advance.x = val->advance.x;
		info->advance.y = val->advance.y;
        if (drawing->hash) {
            drawing->asc = val->asc;
            drawing->desc = val->desc;
        }
	} else {
		glyph_hash_val_t v;
        if (drawing->hash) {
            ass_drawing_parse(drawing, 0);
            FT_Glyph_Copy((FT_Glyph) drawing->glyph, &info->glyph);
        } else {
            info->glyph =
                ass_font_get_glyph(render_priv->fontconfig_priv,
                                   render_priv->state.font, symbol,
                                   render_priv->settings.hinting,
                                   render_priv->state.flags);
        }
		if (!info->glyph)
			return;
		info->advance.x = d16_to_d6(info->glyph->advance.x);
		info->advance.y = d16_to_d6(info->glyph->advance.y);
        FT_Glyph_Get_CBox(info->glyph, FT_GLYPH_BBOX_SUBPIXELS, &info->bbox);

        if (render_priv->state.border_x > 0 ||
            render_priv->state.border_y > 0) {

            FT_Glyph_Copy(info->glyph, &info->outline_glyph);
            stroke_outline_glyph(render_priv,
                                 (FT_OutlineGlyph *) &info->outline_glyph,
                                 double_to_d6(render_priv->state.border_x *
                                              render_priv->border_scale),
                                 double_to_d6(render_priv->state.border_y *
                                              render_priv->border_scale));
			}

		memset(&v, 0, sizeof(v));
		FT_Glyph_Copy(info->glyph, &v.glyph);
		if (info->outline_glyph)
			FT_Glyph_Copy(info->outline_glyph, &v.outline_glyph);
		v.advance = info->advance;
		v.bbox_scaled = info->bbox;
        if (drawing->hash) {
            v.asc = drawing->asc;
            v.desc = drawing->desc;
	}
        cache_add_glyph(render_priv->cache.glyph_cache, &key, &v);
}
}

static void transform_3d(FT_Vector shift, FT_Glyph *glyph,
                         FT_Glyph *glyph2, double frx, double fry,
                         double frz, double fax, double fay, double scale);

/**
 * \brief Get bitmaps for a glyph
 * \param info glyph info
 * Tries to get glyph bitmaps from bitmap cache.
 * If they can't be found, they are generated by rotating and rendering the glyph.
 * After that, bitmaps are added to the cache.
 * They are returned in info->bm (glyph), info->bm_o (outline) and info->bm_s (shadow).
 */
static void
get_bitmap_glyph(ass_renderer_t *render_priv, glyph_info_t *info)
{
	bitmap_hash_val_t* val;
	bitmap_hash_key_t* key = &info->hash_key;
	
    val = cache_find_bitmap(render_priv->cache.bitmap_cache, key);
	
	if (val) {
		info->bm = val->bm;
		info->bm_o = val->bm_o;
		info->bm_s = val->bm_s;
	} else {
		FT_Vector shift;
		bitmap_hash_val_t hash_val;
		int error;
		info->bm = info->bm_o = info->bm_s = 0;
		if (info->glyph && info->symbol != '\n' && info->symbol != 0) {
			// calculating rotation shift vector (from rotation origin to the glyph basepoint)
            shift.x = info->hash_key.shift_x;
            shift.y = info->hash_key.shift_y;
			// apply rotation
            transform_3d(shift, &info->glyph, &info->outline_glyph,
                         info->frx, info->fry, info->frz, info->fax,
                         info->fay, render_priv->font_scale);

            // subpixel shift
            if (info->glyph)
                FT_Outline_Translate(
                    &((FT_OutlineGlyph) info->glyph)->outline,
                    info->hash_key.advance.x,
                    -info->hash_key.advance.y);
            if (info->outline_glyph)
                FT_Outline_Translate(
                    &((FT_OutlineGlyph) info->outline_glyph)->outline,
                    info->hash_key.advance.x,
                    -info->hash_key.advance.y);

			// render glyph
            error = glyph_to_bitmap(render_priv->library,
                                    render_priv->synth_priv,
					info->glyph, info->outline_glyph,
					&info->bm, &info->bm_o,
                                    &info->bm_s, info->be,
                                    info->blur * render_priv->border_scale,
                                    info->hash_key.shadow_offset);
			if (error)
				info->symbol = 0;

			// add bitmaps to cache
			hash_val.bm_o = info->bm_o;
			hash_val.bm = info->bm;
			hash_val.bm_s = info->bm_s;
            cache_add_bitmap(render_priv->cache.bitmap_cache,
                             &(info->hash_key), &hash_val);
		}
	}
	// deallocate glyphs
	if (info->glyph)
		FT_Done_Glyph(info->glyph);
	if (info->outline_glyph)
		FT_Done_Glyph(info->outline_glyph);
}

/**
 * This function goes through text_info and calculates text parameters.
 * The following text_info fields are filled:
 *   height
 *   lines[].height
 *   lines[].asc
 *   lines[].desc
 */
static void measure_text(ass_renderer_t *render_priv)
{
    text_info_t *text_info = &render_priv->text_info;
    int cur_line = 0;
    double max_asc = 0., max_desc = 0.;
    glyph_info_t *last = NULL;
	int i;
    int empty_line = 1;
    text_info->height = 0.;
	for (i = 0; i < text_info->length + 1; ++i) {
		if ((i == text_info->length) || text_info->glyphs[i].linebreak) {
            if (empty_line && cur_line > 0 && last && i < text_info->length) {
                max_asc = d6_to_double(last->asc) / 2.0;
                max_desc = d6_to_double(last->desc) / 2.0;
            }
			text_info->lines[cur_line].asc = max_asc;
			text_info->lines[cur_line].desc = max_desc;
			text_info->height += max_asc + max_desc;
			cur_line ++;
            max_asc = max_desc = 0.;
            empty_line = 1;
        } else
            empty_line = 0;
		if (i < text_info->length) {
			glyph_info_t* cur = text_info->glyphs + i;
            if (d6_to_double(cur->asc) > max_asc)
                max_asc = d6_to_double(cur->asc);
            if (d6_to_double(cur->desc) > max_desc)
                max_desc = d6_to_double(cur->desc);
            if (cur->symbol != '\n' && cur->symbol != 0)
                last = cur;
		}
	}
    text_info->height +=
        (text_info->n_lines -
         1) * render_priv->settings.line_spacing;
}

/**
 * \brief rearrange text between lines
 * \param max_text_width maximal text line width in pixels
 * The algo is similar to the one in libvo/sub.c:
 * 1. Place text, wrapping it when current line is full
 * 2. Try moving words from the end of a line to the beginning of the next one while it reduces
 * the difference in lengths between this two lines.
 * The result may not be optimal, but usually is good enough.
 */
static void
wrap_lines_smart(ass_renderer_t *render_priv, double max_text_width)
{
    int i;
	glyph_info_t *cur, *s1, *e1, *s2, *s3, *w;
	int last_space;
	int break_type;
	int exit;
    double pen_shift_x;
    double pen_shift_y;
	int cur_line;
    text_info_t *text_info = &render_priv->text_info;

	last_space = -1;
	text_info->n_lines = 1;
	break_type = 0;
	s1 = text_info->glyphs; // current line start
	for (i = 0; i < text_info->length; ++i) {
        int break_at;
        double s_offset, len;
		cur = text_info->glyphs + i;
		break_at = -1;
        s_offset = d6_to_double(s1->bbox.xMin + s1->pos.x);
        len = d6_to_double(cur->bbox.xMax + cur->pos.x) - s_offset;

		if (cur->symbol == '\n') {
			break_type = 2;
			break_at = i;
            ass_msg(render_priv->library, MSGL_DBG2,
                    "forced line break at %d", break_at);
		}
		
        if ((len >= max_text_width)
            && (render_priv->track->WrapStyle != 2)) {
			break_type = 1;
			break_at = last_space;
			if (break_at == -1)
				break_at = i - 1;
			if (break_at == -1)
				break_at = 0;
            ass_msg(render_priv->library, MSGL_DBG2, "overfill at %d", i);
            ass_msg(render_priv->library, MSGL_DBG2, "line break at %d",
                    break_at);
		}

		if (break_at != -1) {
			// need to use one more line
			// marking break_at+1 as start of a new line
			int lead = break_at + 1; // the first symbol of the new line
            if (text_info->n_lines >= text_info->max_lines) {
                // Raise maximum number of lines
                text_info->max_lines *= 2;
                text_info->lines = realloc(text_info->lines,
                                           sizeof(line_info_t) *
                                           text_info->max_lines);
			}
			if (lead < text_info->length)
				text_info->glyphs[lead].linebreak = break_type;
			last_space = -1;
			s1 = text_info->glyphs + lead;
            s_offset = d6_to_double(s1->bbox.xMin + s1->pos.x);
			text_info->n_lines ++;
		}
		
		if (cur->symbol == ' ')
			last_space = i;

		// make sure the hard linebreak is not forgotten when
		// there was a new soft linebreak just inserted
		if (cur->symbol == '\n' && break_type == 1)
			i--;
	}
#define DIFF(x,y) (((x) < (y)) ? (y - x) : (x - y))
	exit = 0;
	while (!exit) {
		exit = 1;
		w = s3 = text_info->glyphs;
		s1 = s2 = 0;
		for (i = 0; i <= text_info->length; ++i) {
			cur = text_info->glyphs + i;
			if ((i == text_info->length) || cur->linebreak) {
				s1 = s2;
				s2 = s3;
				s3 = cur;
				if (s1 && (s2->linebreak == 1)) { // have at least 2 lines, and linebreak is 'soft'
                    double l1, l2, l1_new, l2_new;

					w = s2;
                    do {
                        --w;
                    } while ((w > s1) && (w->symbol == ' '));
                    while ((w > s1) && (w->symbol != ' ')) {
                        --w;
                    }
					e1 = w;
                    while ((e1 > s1) && (e1->symbol == ' ')) {
                        --e1;
                    }
                    if (w->symbol == ' ')
                        ++w;

                    l1 = d6_to_double(((s2 - 1)->bbox.xMax + (s2 - 1)->pos.x) -
                        (s1->bbox.xMin + s1->pos.x));
                    l2 = d6_to_double(((s3 - 1)->bbox.xMax + (s3 - 1)->pos.x) -
                        (s2->bbox.xMin + s2->pos.x));
                    l1_new = d6_to_double(
                        (e1->bbox.xMax + e1->pos.x) -
                        (s1->bbox.xMin + s1->pos.x));
                    l2_new = d6_to_double(
                        ((s3 - 1)->bbox.xMax + (s3 - 1)->pos.x) -
                        (w->bbox.xMin + w->pos.x));

					if (DIFF(l1_new, l2_new) < DIFF(l1, l2)) {
						w->linebreak = 1;
						s2->linebreak = 0;
						exit = 0;
					}
				}
			}
			if (i == text_info->length)
				break;
		}
		
	}
	assert(text_info->n_lines >= 1);
#undef DIFF
	
    measure_text(render_priv);

    pen_shift_x = 0.;
    pen_shift_y = 0.;
	cur_line = 1;
	for (i = 0; i < text_info->length; ++i) {
		cur = text_info->glyphs + i;
		if (cur->linebreak) {
            double height =
                text_info->lines[cur_line - 1].desc +
                text_info->lines[cur_line].asc;
			cur_line ++;
            pen_shift_x = d6_to_double(-cur->pos.x);
            pen_shift_y += height + render_priv->settings.line_spacing;
            ass_msg(render_priv->library, MSGL_DBG2,
                   "shifting from %d to %d by (%f, %f)", i,
                   text_info->length - 1, pen_shift_x, pen_shift_y);
		}
        cur->pos.x += double_to_d6(pen_shift_x);
        cur->pos.y += double_to_d6(pen_shift_y);
	}
}

/**
 * \brief determine karaoke effects
 * Karaoke effects cannot be calculated during parse stage (get_next_char()),
 * so they are done in a separate step.
 * Parse stage: when karaoke style override is found, its parameters are stored in the next glyph's 
 * (the first glyph of the karaoke word)'s effect_type and effect_timing.
 * This function:
 * 1. sets effect_type for all glyphs in the word (_karaoke_ word)
 * 2. sets effect_timing for all glyphs to x coordinate of the border line between the left and right karaoke parts
 * (left part is filled with PrimaryColour, right one - with SecondaryColour).
 */
static void process_karaoke_effects(ass_renderer_t *render_priv)
{
	glyph_info_t *cur, *cur2;
	glyph_info_t *s1, *e1; // start and end of the current word
	glyph_info_t *s2; // start of the next word
	int i;
	int timing; // current timing
	int tm_start, tm_end; // timings at start and end of the current word
	int tm_current;
	double dt;
	int x;
	int x_start, x_end;

    tm_current = render_priv->time - render_priv->state.event->Start;
	timing = 0;
	s1 = s2 = 0;
    for (i = 0; i <= render_priv->text_info.length; ++i) {
        cur = render_priv->text_info.glyphs + i;
        if ((i == render_priv->text_info.length)
            || (cur->effect_type != EF_NONE)) {
			s1 = s2;
			s2 = cur;
			if (s1) {
				e1 = s2 - 1;
				tm_start = timing + s1->effect_skip_timing;
				tm_end = tm_start + s1->effect_timing;
				timing = tm_end;
				x_start = 1000000;
				x_end = -1000000;
				for (cur2 = s1; cur2 <= e1; ++cur2) {
                    x_start = FFMIN(x_start, d6_to_int(cur2->bbox.xMin + cur2->pos.x));
                    x_end = FFMAX(x_end, d6_to_int(cur2->bbox.xMax + cur2->pos.x));
				}

				dt = (tm_current - tm_start);
                if ((s1->effect_type == EF_KARAOKE)
                    || (s1->effect_type == EF_KARAOKE_KO)) {
					if (dt > 0)
						x = x_end + 1;
					else
						x = x_start;
				} else if (s1->effect_type == EF_KARAOKE_KF) {
					dt /= (tm_end - tm_start);
					x = x_start + (x_end - x_start) * dt;
				} else {
                    ass_msg(render_priv->library, MSGL_ERR,
                            "Unknown effect type");
					continue;
				}

				for (cur2 = s1; cur2 <= e1; ++cur2) {
					cur2->effect_type = s1->effect_type;
                    cur2->effect_timing = x - d6_to_int(cur2->pos.x);
				}
			}
		}
	}
}

/**
 * \brief Calculate base point for positioning and rotation
 * \param bbox text bbox
 * \param alignment alignment
 * \param bx, by out: base point coordinates
 */
static void get_base_point(double_bbox_t *bbox, int alignment, double *bx, double *by)
{
	const int halign = alignment & 3;
	const int valign = alignment & 12;
	if (bx)
		switch(halign) {
		case HALIGN_LEFT:
            *bx = bbox->xMin;
			break;
		case HALIGN_CENTER:
            *bx = (bbox->xMax + bbox->xMin) / 2.0;
			break;
		case HALIGN_RIGHT:
            *bx = bbox->xMax;
			break;
		}
	if (by)
		switch(valign) {
		case VALIGN_TOP:
            *by = bbox->yMin;
			break;
		case VALIGN_CENTER:
            *by = (bbox->yMax + bbox->yMin) / 2.0;
			break;
		case VALIGN_SUB:
            *by = bbox->yMax;
			break;
		}
}

/**
 * \brief Apply transformation to outline points of a glyph
 * Applies rotations given by frx, fry and frz and projects the points back
 * onto the screen plane.
 */
static void
transform_3d_points(FT_Vector shift, FT_Glyph glyph, double frx,
                    double fry, double frz, double fax, double fay,
                    double scale)
{
    double sx = sin(frx);
    double sy = sin(fry);
    double sz = sin(frz);
    double cx = cos(frx);
    double cy = cos(fry);
    double cz = cos(frz);
	FT_Outline* outline = &((FT_OutlineGlyph)glyph)->outline;
	FT_Vector* p = outline->points;
    double x, y, z, xx, yy, zz;
    int i, dist;

    dist = 20000 * scale;
	for (i=0; i<outline->n_points; i++) {
        x = (double) p[i].x + shift.x + (-fax * p[i].y);
        y = (double) p[i].y + shift.y + (-fay * p[i].x);
        z = 0.;

        xx = x * cz + y * sz;
        yy = -(x * sz - y * cz);
        zz = z;

        x = xx;
        y = yy * cx + zz * sx;
        z = yy * sx - zz * cx;

        xx = x * cy + z * sy;
        yy = y;
        zz = x * sy - z * cy;

        zz = FFMAX(zz, 1000 - dist);

        x = (xx * dist) / (zz + dist);
        y = (yy * dist) / (zz + dist);
        p[i].x = x - shift.x + 0.5;
        p[i].y = y - shift.y + 0.5;
}
}

/**
 * \brief Apply 3d transformation to several objects
 * \param shift FreeType vector
 * \param glyph FreeType glyph
 * \param glyph2 FreeType glyph
 * \param frx x-axis rotation angle
 * \param fry y-axis rotation angle
 * \param frz z-axis rotation angle
 * Rotates both glyphs by frx, fry and frz. Shift vector is added before rotation and subtracted after it.
 */
static void
transform_3d(FT_Vector shift, FT_Glyph *glyph, FT_Glyph *glyph2,
             double frx, double fry, double frz, double fax, double fay,
             double scale)
{
    frx = -frx;
    frz = -frz;
    if (frx != 0. || fry != 0. || frz != 0. || fax != 0. || fay != 0.) {
		if (glyph && *glyph)
            transform_3d_points(shift, *glyph, frx, fry, frz,
                                fax, fay, scale);

		if (glyph2 && *glyph2)
            transform_3d_points(shift, *glyph2, frx, fry, frz,
                                fax, fay, scale);
	}
}


/**
 * \brief Main ass rendering function, glues everything together
 * \param event event to render
 * \param event_images struct containing resulting images, will also be initialized
 * Process event, appending resulting ass_image_t's to images_root.
 */
static int
ass_render_event(ass_renderer_t *render_priv, ass_event_t *event,
                 event_images_t *event_images)
{
	char* p;
	FT_UInt previous; 
	FT_UInt num_glyphs;
	FT_Vector pen;
	unsigned code;
    double_bbox_t bbox;
	int i, j;
	int MarginL, MarginR, MarginV;
	int last_break;
	int alignment, halign, valign;
    double device_x = 0;
    double device_y = 0;
    text_info_t *text_info = &render_priv->text_info;
    ass_drawing_t *drawing;

    if (event->Style >= render_priv->track->n_styles) {
        ass_msg(render_priv->library, MSGL_WARN, "No style found");
		return 1;
	}
	if (!event->Text) {
        ass_msg(render_priv->library, MSGL_WARN, "Empty event");
		return 1;
	}

    init_render_context(render_priv, event);

    drawing = render_priv->state.drawing;
	text_info->length = 0;
	pen.x = 0;
	pen.y = 0;
	previous = 0;
	num_glyphs = 0;
	p = event->Text;
	// Event parsing.
	while (1) {
		// get next char, executing style override
        // this affects render_context
		do {
            code = get_next_char(render_priv, &p);
            if (render_priv->state.drawing_mode && code)
                ass_drawing_add_char(drawing, (char) code);
        } while (code && render_priv->state.drawing_mode);      // skip everything in drawing mode

        // Parse drawing
        if (drawing->i) {
            drawing->scale_x = render_priv->state.scale_x *
                                     render_priv->font_scale_x *
                                     render_priv->font_scale;
            drawing->scale_y = render_priv->state.scale_y *
                                     render_priv->font_scale;
            ass_drawing_hash(drawing);
            p--;
            code = -1;
        }

		// face could have been changed in get_next_char
        if (!render_priv->state.font) {
            free_render_context(render_priv);
			return 1;
		}

		if (code == 0)
			break;

        if (text_info->length >= text_info->max_glyphs) {
            // Raise maximum number of glyphs
            text_info->max_glyphs *= 2;
            text_info->glyphs =
                realloc(text_info->glyphs,
                        sizeof(glyph_info_t) * text_info->max_glyphs);
		}

        // Add kerning to pen
        if (previous && code && !drawing->hash) {
			FT_Vector delta;
            delta =
                ass_font_get_kerning(render_priv->state.font, previous,
                                     code);
            pen.x += delta.x * render_priv->state.scale_x;
            pen.y += delta.y * render_priv->state.scale_y;
		}

        ass_font_set_transform(render_priv->state.font,
                               render_priv->state.scale_x *
                               render_priv->font_scale_x,
                               render_priv->state.scale_y, NULL);

        get_outline_glyph(render_priv, code,
                          text_info->glyphs + text_info->length, drawing);

        text_info->glyphs[text_info->length].pos.x = pen.x;
        text_info->glyphs[text_info->length].pos.y = pen.y;
		
		pen.x += text_info->glyphs[text_info->length].advance.x;
        pen.x += double_to_d6(render_priv->state.hspacing *
                              render_priv->font_scale);
		pen.y += text_info->glyphs[text_info->length].advance.y;
        pen.y += render_priv->state.fay *
                 text_info->glyphs[text_info->length].advance.x;
		
		previous = code;

		text_info->glyphs[text_info->length].symbol = code;
		text_info->glyphs[text_info->length].linebreak = 0;
		for (i = 0; i < 4; ++i) {
            uint32_t clr = render_priv->state.c[i];
            change_alpha(&clr,
                         mult_alpha(_a(clr), render_priv->state.fade), 1.);
			text_info->glyphs[text_info->length].c[i] = clr;
		}
        text_info->glyphs[text_info->length].effect_type =
            render_priv->state.effect_type;
        text_info->glyphs[text_info->length].effect_timing =
            render_priv->state.effect_timing;
        text_info->glyphs[text_info->length].effect_skip_timing =
            render_priv->state.effect_skip_timing;
        text_info->glyphs[text_info->length].be = render_priv->state.be;
        text_info->glyphs[text_info->length].blur = render_priv->state.blur;
        text_info->glyphs[text_info->length].shadow_x =
            render_priv->state.shadow_x;
        text_info->glyphs[text_info->length].shadow_y =
            render_priv->state.shadow_y;
        text_info->glyphs[text_info->length].frx = render_priv->state.frx;
        text_info->glyphs[text_info->length].fry = render_priv->state.fry;
        text_info->glyphs[text_info->length].frz = render_priv->state.frz;
        text_info->glyphs[text_info->length].fax = render_priv->state.fax;
        text_info->glyphs[text_info->length].fay = render_priv->state.fay;
        if (drawing->hash) {
            text_info->glyphs[text_info->length].asc = drawing->asc;
            text_info->glyphs[text_info->length].desc = drawing->desc;
        } else {
            ass_font_get_asc_desc(render_priv->state.font, code,
				      &text_info->glyphs[text_info->length].asc,
				      &text_info->glyphs[text_info->length].desc);

            text_info->glyphs[text_info->length].asc *=
                render_priv->state.scale_y;
            text_info->glyphs[text_info->length].desc *=
                render_priv->state.scale_y;
        }

		// fill bitmap_hash_key
        if (!drawing->hash) {
            text_info->glyphs[text_info->length].hash_key.font =
                render_priv->state.font;
            text_info->glyphs[text_info->length].hash_key.size =
                render_priv->state.font_size;
            text_info->glyphs[text_info->length].hash_key.bold =
                render_priv->state.bold;
            text_info->glyphs[text_info->length].hash_key.italic =
                render_priv->state.italic;
        } else
            text_info->glyphs[text_info->length].hash_key.drawing_hash =
                drawing->hash;
		text_info->glyphs[text_info->length].hash_key.ch = code;
        text_info->glyphs[text_info->length].hash_key.outline.x =
            render_priv->state.border_x * 0xFFFF;
        text_info->glyphs[text_info->length].hash_key.outline.y =
            render_priv->state.border_y * 0xFFFF;
        text_info->glyphs[text_info->length].hash_key.scale_x =
            render_priv->state.scale_x * 0xFFFF;
        text_info->glyphs[text_info->length].hash_key.scale_y =
            render_priv->state.scale_y * 0xFFFF;
        text_info->glyphs[text_info->length].hash_key.frx =
            render_priv->state.frx * 0xFFFF;
        text_info->glyphs[text_info->length].hash_key.fry =
            render_priv->state.fry * 0xFFFF;
        text_info->glyphs[text_info->length].hash_key.frz =
            render_priv->state.frz * 0xFFFF;
        text_info->glyphs[text_info->length].hash_key.fax =
            render_priv->state.fax * 0xFFFF;
        text_info->glyphs[text_info->length].hash_key.fay =
            render_priv->state.fay * 0xFFFF;
        text_info->glyphs[text_info->length].hash_key.advance.x = pen.x;
        text_info->glyphs[text_info->length].hash_key.advance.y = pen.y;
        text_info->glyphs[text_info->length].hash_key.be =
            render_priv->state.be;
        text_info->glyphs[text_info->length].hash_key.blur =
            render_priv->state.blur;
        text_info->glyphs[text_info->length].hash_key.shadow_offset.x =
            double_to_d6(
                render_priv->state.shadow_x * render_priv->border_scale -
                (int) (render_priv->state.shadow_x *
                render_priv->border_scale));
        text_info->glyphs[text_info->length].hash_key.shadow_offset.y =
            double_to_d6(
                render_priv->state.shadow_y * render_priv->border_scale -
                (int) (render_priv->state.shadow_y *
                render_priv->border_scale));

		text_info->length++;

        render_priv->state.effect_type = EF_NONE;
        render_priv->state.effect_timing = 0;
        render_priv->state.effect_skip_timing = 0;

        if (drawing->hash) {
            ass_drawing_free(drawing);
            drawing = render_priv->state.drawing =
                ass_drawing_new(render_priv->fontconfig_priv,
                    render_priv->state.font,
                    render_priv->settings.hinting,
                    render_priv->ftlibrary);
	}
    }
	

	if (text_info->length == 0) {
		// no valid symbols in the event; this can be smth like {comment}
        free_render_context(render_priv);
		return 1;
	}
	// depends on glyph x coordinates being monotonous, so it should be done before line wrap
    process_karaoke_effects(render_priv);
	
	// alignments
    alignment = render_priv->state.alignment;
	halign = alignment & 3;
	valign = alignment & 12;

    MarginL =
        (event->MarginL) ? event->MarginL : render_priv->state.style->
        MarginL;
    MarginR =
        (event->MarginR) ? event->MarginR : render_priv->state.style->
        MarginR;
    MarginV =
        (event->MarginV) ? event->MarginV : render_priv->state.style->
        MarginV;

    if (render_priv->state.evt_type != EVENT_HSCROLL) {
        double max_text_width;

		// calculate max length of a line
        max_text_width =
            x2scr(render_priv,
                  render_priv->track->PlayResX - MarginR) -
            x2scr(render_priv, MarginL);

		// rearrange text in several lines
        wrap_lines_smart(render_priv, max_text_width);

		// align text
		last_break = -1;
		for (i = 1; i < text_info->length + 1; ++i) { // (text_info->length + 1) is the end of the last line
            if ((i == text_info->length)
                || text_info->glyphs[i].linebreak) {
                double width, shift = 0;
                glyph_info_t *first_glyph =
                    text_info->glyphs + last_break + 1;
				glyph_info_t* last_glyph = text_info->glyphs + i - 1;

                while ((last_glyph > first_glyph)
                       && ((last_glyph->symbol == '\n')
                           || (last_glyph->symbol == 0)))
					last_glyph --;

                width = d6_to_double(
                    last_glyph->pos.x + last_glyph->advance.x -
                    first_glyph->pos.x);
				if (halign == HALIGN_LEFT) { // left aligned, no action
					shift = 0;
				} else if (halign == HALIGN_RIGHT) { // right aligned
					shift = max_text_width - width;
				} else if (halign == HALIGN_CENTER) { // centered
                    shift = (max_text_width - width) / 2.0;
				}
				for (j = last_break + 1; j < i; ++j) {
                    text_info->glyphs[j].pos.x += double_to_d6(shift);
				}
				last_break = i - 1;
			}
		}
    } else {                    // render_priv->state.evt_type == EVENT_HSCROLL
        measure_text(render_priv);
	}
	
	// determing text bounding box
	compute_string_bbox(text_info, &bbox);
	
	// determine device coordinates for text
	
	// x coordinate for everything except positioned events
    if (render_priv->state.evt_type == EVENT_NORMAL ||
        render_priv->state.evt_type == EVENT_VSCROLL) {
        device_x = x2scr(render_priv, MarginL);
    } else if (render_priv->state.evt_type == EVENT_HSCROLL) {
        if (render_priv->state.scroll_direction == SCROLL_RL)
            device_x =
                x2scr(render_priv,
                      render_priv->track->PlayResX -
                      render_priv->state.scroll_shift);
        else if (render_priv->state.scroll_direction == SCROLL_LR)
            device_x =
                x2scr(render_priv,
                      render_priv->state.scroll_shift) - (bbox.xMax -
                                                          bbox.xMin);
	}
	// y coordinate for everything except positioned events
    if (render_priv->state.evt_type == EVENT_NORMAL ||
        render_priv->state.evt_type == EVENT_HSCROLL) {
		if (valign == VALIGN_TOP) { // toptitle
            device_y =
                y2scr_top(render_priv,
                          MarginV) + text_info->lines[0].asc;
		} else if (valign == VALIGN_CENTER) { // midtitle
            double scr_y =
                y2scr(render_priv, render_priv->track->PlayResY / 2.0);
            device_y = scr_y - (bbox.yMax + bbox.yMin) / 2.0;
		} else { // subtitle
            double scr_y;
			if (valign != VALIGN_SUB)
                ass_msg(render_priv->library, MSGL_V,
                       "Invalid valign, supposing 0 (subtitle)");
            scr_y =
                y2scr_sub(render_priv,
                          render_priv->track->PlayResY - MarginV);
			device_y = scr_y;
            device_y -= text_info->height;
            device_y += text_info->lines[0].asc;
		}
    } else if (render_priv->state.evt_type == EVENT_VSCROLL) {
        if (render_priv->state.scroll_direction == SCROLL_TB)
            device_y =
                y2scr(render_priv,
                      render_priv->state.clip_y0 +
                      render_priv->state.scroll_shift) - (bbox.yMax -
                                                          bbox.yMin);
        else if (render_priv->state.scroll_direction == SCROLL_BT)
            device_y =
                y2scr(render_priv,
                      render_priv->state.clip_y1 -
                      render_priv->state.scroll_shift);
	}
	// positioned events are totally different
    if (render_priv->state.evt_type == EVENT_POSITIONED) {
        double base_x = 0;
        double base_y = 0;
        ass_msg(render_priv->library, MSGL_DBG2, "positioned event at %f, %f",
               render_priv->state.pos_x, render_priv->state.pos_y);
        get_base_point(&bbox, alignment, &base_x, &base_y);
        device_x =
            x2scr_pos(render_priv, render_priv->state.pos_x) - base_x;
        device_y =
            y2scr_pos(render_priv, render_priv->state.pos_y) - base_y;
	}
	// fix clip coordinates (they depend on alignment)
    if (render_priv->state.evt_type == EVENT_NORMAL ||
        render_priv->state.evt_type == EVENT_HSCROLL ||
        render_priv->state.evt_type == EVENT_VSCROLL) {
        render_priv->state.clip_x0 =
            x2scr(render_priv, render_priv->state.clip_x0);
        render_priv->state.clip_x1 =
            x2scr(render_priv, render_priv->state.clip_x1);
		if (valign == VALIGN_TOP) {
            render_priv->state.clip_y0 =
                y2scr_top(render_priv, render_priv->state.clip_y0);
            render_priv->state.clip_y1 =
                y2scr_top(render_priv, render_priv->state.clip_y1);
		} else if (valign == VALIGN_CENTER) {
            render_priv->state.clip_y0 =
                y2scr(render_priv, render_priv->state.clip_y0);
            render_priv->state.clip_y1 =
                y2scr(render_priv, render_priv->state.clip_y1);
		} else if (valign == VALIGN_SUB) {
            render_priv->state.clip_y0 =
                y2scr_sub(render_priv, render_priv->state.clip_y0);
            render_priv->state.clip_y1 =
                y2scr_sub(render_priv, render_priv->state.clip_y1);
		}
    } else if (render_priv->state.evt_type == EVENT_POSITIONED) {
        render_priv->state.clip_x0 =
            x2scr_pos(render_priv, render_priv->state.clip_x0);
        render_priv->state.clip_x1 =
            x2scr_pos(render_priv, render_priv->state.clip_x1);
        render_priv->state.clip_y0 =
            y2scr_pos(render_priv, render_priv->state.clip_y0);
        render_priv->state.clip_y1 =
            y2scr_pos(render_priv, render_priv->state.clip_y1);
	}
	// calculate rotation parameters
	{
        double_vector_t center;
		
        if (render_priv->state.have_origin) {
            center.x = x2scr(render_priv, render_priv->state.org_x);
            center.y = y2scr(render_priv, render_priv->state.org_y);
		} else {
            double bx = 0., by = 0.;
            get_base_point(&bbox, alignment, &bx, &by);
			center.x = device_x + bx;
			center.y = device_y + by;
		}

		for (i = 0; i < text_info->length; ++i) {
			glyph_info_t* info = text_info->glyphs + i;

            if (info->hash_key.frx || info->hash_key.fry
                || info->hash_key.frz || info->hash_key.fax
                || info->hash_key.fay) {
                info->hash_key.shift_x = info->pos.x + double_to_d6(device_x - center.x);
                info->hash_key.shift_y =
                    -(info->pos.y + double_to_d6(device_y - center.y));
			} else {
				info->hash_key.shift_x = 0;
				info->hash_key.shift_y = 0;
			}
		}
	}

	// convert glyphs to bitmaps
    for (i = 0; i < text_info->length; ++i) {
        glyph_info_t *g = text_info->glyphs + i;
        g->hash_key.advance.x =
            double_to_d6(device_x - (int) device_x +
            d6_to_double(g->pos.x & SUBPIXEL_MASK)) & ~SUBPIXEL_ACCURACY;
        g->hash_key.advance.y =
            double_to_d6(device_y - (int) device_y +
            d6_to_double(g->pos.y & SUBPIXEL_MASK)) & ~SUBPIXEL_ACCURACY;
        get_bitmap_glyph(render_priv, text_info->glyphs + i);
    }

    memset(event_images, 0, sizeof(*event_images));
    event_images->top = device_y - text_info->lines[0].asc;
    event_images->height = text_info->height;
    event_images->detect_collisions = render_priv->state.detect_collisions;
	event_images->shift_direction = (valign == VALIGN_TOP) ? 1 : -1;
	event_images->event = event;
    event_images->imgs = render_text(render_priv, (int) device_x, (int) device_y);

    free_render_context(render_priv);
	
	return 0;
}

/**
 * \brief deallocate image list
 * \param img list pointer
 */
static void ass_free_images(ass_image_t *img)
{
	while (img) {
		ass_image_t* next = img->next;
		free(img);
		img = next;
	}
}

static void ass_reconfigure(ass_renderer_t* priv)
{
    priv->render_id++;
    priv->cache.glyph_cache =
        ass_glyph_cache_reset(priv->cache.glyph_cache);
    priv->cache.bitmap_cache =
        ass_bitmap_cache_reset(priv->cache.bitmap_cache);
    priv->cache.composite_cache =
        ass_composite_cache_reset(priv->cache.composite_cache);
	ass_free_images(priv->prev_images_root);
	priv->prev_images_root = 0;
}

void ass_set_frame_size(ass_renderer_t* priv, int w, int h)
{
	if (priv->settings.frame_width != w || priv->settings.frame_height != h) {
		priv->settings.frame_width = w;
		priv->settings.frame_height = h;
        if (priv->settings.aspect == 0.) {
			priv->settings.aspect = ((double)w) / h;
            priv->settings.pixel_ratio = ((double) w) / h;
        }
		ass_reconfigure(priv);
	}
}

void ass_set_margins(ass_renderer_t* priv, int t, int b, int l, int r)
{
	if (priv->settings.left_margin != l ||
	    priv->settings.right_margin != r ||
        priv->settings.top_margin != t
        || priv->settings.bottom_margin != b) {
		priv->settings.left_margin = l;
		priv->settings.right_margin = r;
		priv->settings.top_margin = t;
		priv->settings.bottom_margin = b;
		ass_reconfigure(priv);
	}
}

void ass_set_use_margins(ass_renderer_t* priv, int use)
{
	priv->settings.use_margins = use;
}

void ass_set_aspect_ratio(ass_renderer_t *priv, double ar, double par)
{
    if (priv->settings.aspect != ar || priv->settings.pixel_ratio != par) {
		priv->settings.aspect = ar;
        priv->settings.pixel_ratio = par;
		ass_reconfigure(priv);
	}
}

void ass_set_font_scale(ass_renderer_t* priv, double font_scale)
{
	if (priv->settings.font_size_coeff != font_scale) {
		priv->settings.font_size_coeff = font_scale;
		ass_reconfigure(priv);
	}
}

void ass_set_hinting(ass_renderer_t* priv, ass_hinting_t ht)
{
	if (priv->settings.hinting != ht) {
		priv->settings.hinting = ht;
		ass_reconfigure(priv);
	}
}

void ass_set_line_spacing(ass_renderer_t* priv, double line_spacing)
{
	priv->settings.line_spacing = line_spacing;
}

void ass_set_fonts(ass_renderer_t *priv, const char *default_font,
                   const char *default_family, int fc, const char *config,
                   int update)
{
		free(priv->settings.default_font);
		free(priv->settings.default_family);
	priv->settings.default_font = default_font ? strdup(default_font) : 0;
    priv->settings.default_family =
        default_family ? strdup(default_family) : 0;

	if (priv->fontconfig_priv)
		fontconfig_done(priv->fontconfig_priv);
    priv->fontconfig_priv =
        fontconfig_init(priv->library, priv->ftlibrary, default_family,
                        default_font, fc, config, update);
}

int ass_fonts_update(ass_renderer_t *render_priv)
{
    return fontconfig_update(render_priv->fontconfig_priv);
}

/**
 * \brief Start a new frame
 */
static int
ass_start_frame(ass_renderer_t *render_priv, ass_track_t *track,
                long long now)
{
    ass_settings_t *settings_priv = &render_priv->settings;
    cache_store_t *cache = &render_priv->cache;

    if (!render_priv->settings.frame_width
        && !render_priv->settings.frame_height)
		return 1; // library not initialized

    if (render_priv->library != track->library)
        return 1;

    free_list_clear(render_priv);

	if (track->n_events == 0)
		return 1; // nothing to do
	
    render_priv->width = settings_priv->frame_width;
    render_priv->height = settings_priv->frame_height;
    render_priv->orig_width =
        settings_priv->frame_width - settings_priv->left_margin -
        settings_priv->right_margin;
    render_priv->orig_height =
        settings_priv->frame_height - settings_priv->top_margin -
        settings_priv->bottom_margin;
    render_priv->orig_width_nocrop =
        settings_priv->frame_width - FFMAX(settings_priv->left_margin,
                                           0) -
        FFMAX(settings_priv->right_margin, 0);
    render_priv->orig_height_nocrop =
        settings_priv->frame_height - FFMAX(settings_priv->top_margin,
                                            0) -
        FFMAX(settings_priv->bottom_margin, 0);
    render_priv->track = track;
    render_priv->time = now;

    ass_lazy_track_init(render_priv);
	
    render_priv->font_scale = settings_priv->font_size_coeff *
        render_priv->orig_height / render_priv->track->PlayResY;
    if (render_priv->track->ScaledBorderAndShadow)
        render_priv->border_scale =
            ((double) render_priv->orig_height) /
            render_priv->track->PlayResY;
	else
        render_priv->border_scale = 1.;

    // PAR correction
    render_priv->font_scale_x = render_priv->settings.aspect /
                                render_priv->settings.pixel_ratio;

    render_priv->prev_images_root = render_priv->images_root;
    render_priv->images_root = 0;

    if (cache->bitmap_cache->cache_size > cache->bitmap_max_size) {
        ass_msg(render_priv->library, MSGL_V,
                "Hitting hard bitmap cache limit (was: %ld bytes), "
                "resetting.", (long) cache->bitmap_cache->cache_size);
        cache->bitmap_cache = ass_bitmap_cache_reset(cache->bitmap_cache);
        cache->composite_cache = ass_composite_cache_reset(
            cache->composite_cache);
        ass_free_images(render_priv->prev_images_root);
        render_priv->prev_images_root = 0;
    }

    if (cache->glyph_cache->count > cache->glyph_max) {
        ass_msg(render_priv->library, MSGL_V,
            "Hitting hard glyph cache limit (was: %ld glyphs), resetting.",
            (long) cache->glyph_cache->count);
        cache->glyph_cache = ass_glyph_cache_reset(cache->glyph_cache);
    }

	return 0;
}

static int cmp_event_layer(const void* p1, const void* p2)
{
	ass_event_t* e1 = ((event_images_t*)p1)->event;
	ass_event_t* e2 = ((event_images_t*)p2)->event;
	if (e1->Layer < e2->Layer)
		return -1;
	if (e1->Layer > e2->Layer)
		return 1;
	if (e1->ReadOrder < e2->ReadOrder)
		return -1;
	if (e1->ReadOrder > e2->ReadOrder)
		return 1;
	return 0;
}

#define MAX_EVENTS 100

static render_priv_t *get_render_priv(ass_renderer_t *render_priv,
                                      ass_event_t *event)
{
	if (!event->render_priv)
		event->render_priv = calloc(1, sizeof(render_priv_t));
	// FIXME: check render_id
    if (render_priv->render_id != event->render_priv->render_id) {
		memset(event->render_priv, 0, sizeof(render_priv_t));
        event->render_priv->render_id = render_priv->render_id;
	}
	return event->render_priv;
}

typedef struct {
	int a, b; // top and height
} segment_t;

static int overlap(segment_t* s1, segment_t* s2)
{
	if (s1->a >= s2->b || s2->a >= s1->b)
		return 0;
	return 1;
}

static int cmp_segment(const void* p1, const void* p2)
{
	return ((segment_t*)p1)->a - ((segment_t*)p2)->a;
}

static void
shift_event(ass_renderer_t *render_priv, event_images_t *ei, int shift)
{
	ass_image_t* cur = ei->imgs;
	while (cur) {
		cur->dst_y += shift;
		// clip top and bottom
		if (cur->dst_y < 0) {
			int clip = - cur->dst_y;
			cur->h -= clip;
			cur->bitmap += clip * cur->stride;
			cur->dst_y = 0;
		}
        if (cur->dst_y + cur->h >= render_priv->height) {
            int clip = cur->dst_y + cur->h - render_priv->height;
			cur->h -= clip;
		}
		if (cur->h <= 0) {
			cur->h = 0;
			cur->dst_y = 0;
		}
		cur = cur->next;
	}
	ei->top += shift;
}

// dir: 1 - move down
//      -1 - move up
static int fit_segment(segment_t* s, segment_t* fixed, int* cnt, int dir)
{
	int i;
	int shift = 0;

	if (dir == 1) // move down
		for (i = 0; i < *cnt; ++i) {
			if (s->b + shift <= fixed[i].a || s->a + shift >= fixed[i].b)
				continue;
			shift = fixed[i].b - s->a;
    } else                      // dir == -1, move up
		for (i = *cnt-1; i >= 0; --i) {
			if (s->b + shift <= fixed[i].a || s->a + shift >= fixed[i].b)
				continue;
			shift = fixed[i].a - s->b;
		}

	fixed[*cnt].a = s->a + shift;
	fixed[*cnt].b = s->b + shift;
	(*cnt)++;
	qsort(fixed, *cnt, sizeof(segment_t), cmp_segment);
	
	return shift;
}

static void
fix_collisions(ass_renderer_t *render_priv, event_images_t *imgs, int cnt)
{
	segment_t used[MAX_EVENTS];
	int cnt_used = 0;
	int i, j;

	// fill used[] with fixed events
	for (i = 0; i < cnt; ++i) {
		render_priv_t* priv;
        if (!imgs[i].detect_collisions)
            continue;
        priv = get_render_priv(render_priv, imgs[i].event);
		if (priv->height > 0) { // it's a fixed event
			segment_t s;
			s.a = priv->top;
			s.b = priv->top + priv->height;
			if (priv->height != imgs[i].height) { // no, it's not
                ass_msg(render_priv->library, MSGL_WARN,
                        "Warning! Event height has changed");
				priv->top = 0;
				priv->height = 0;
			}
			for (j = 0; j < cnt_used; ++j)
				if (overlap(&s, used + j)) { // no, it's not
					priv->top = 0;
					priv->height = 0;
				}
			if (priv->height > 0) { // still a fixed event
				used[cnt_used].a = priv->top;
				used[cnt_used].b = priv->top + priv->height;
				cnt_used ++;
                shift_event(render_priv, imgs + i, priv->top - imgs[i].top);
			}
		}
	}
	qsort(used, cnt_used, sizeof(segment_t), cmp_segment);

	// try to fit other events in free spaces
	for (i = 0; i < cnt; ++i) {
		render_priv_t* priv;
        if (!imgs[i].detect_collisions)
            continue;
        priv = get_render_priv(render_priv, imgs[i].event);
		if (priv->height == 0) { // not a fixed event
			int shift;
			segment_t s;
			s.a = imgs[i].top;
			s.b = imgs[i].top + imgs[i].height;
            shift =
                fit_segment(&s, used, &cnt_used, imgs[i].shift_direction);
            if (shift)
                shift_event(render_priv, imgs + i, shift);
			// make it fixed
			priv->top = imgs[i].top;
			priv->height = imgs[i].height;
		}
		
	}
}

/**
 * \brief compare two images
 * \param i1 first image
 * \param i2 second image
 * \return 0 if identical, 1 if different positions, 2 if different content
 */
static int ass_image_compare(ass_image_t *i1, ass_image_t *i2)
{
    if (i1->w != i2->w)
        return 2;
    if (i1->h != i2->h)
        return 2;
    if (i1->stride != i2->stride)
        return 2;
    if (i1->color != i2->color)
        return 2;
	if (i1->bitmap != i2->bitmap)
		return 2;
    if (i1->dst_x != i2->dst_x)
        return 1;
    if (i1->dst_y != i2->dst_y)
        return 1;
	return 0;
}

/**
 * \brief compare current and previous image list
 * \param priv library handle
 * \return 0 if identical, 1 if different positions, 2 if different content
 */
static int ass_detect_change(ass_renderer_t *priv)
{
	ass_image_t* img, *img2;
	int diff;

	img = priv->prev_images_root;
	img2 = priv->images_root;
	diff = 0;
	while (img && diff < 2) {
		ass_image_t* next, *next2;
		next = img->next;
		if (img2) {
			int d = ass_image_compare(img, img2);
            if (d > diff)
                diff = d;
			next2 = img2->next;
		} else {
			// previous list is shorter
			diff = 2;
			break;
		}
		img = next;
		img2 = next2;
	}

	// is the previous list longer?
	if (img2)
		diff = 2;

	return diff;
}

/**
 * \brief render a frame
 * \param priv library handle
 * \param track track
 * \param now current video timestamp (ms)
 * \param detect_change a value describing how the new images differ from the previous ones will be written here:
 *        0 if identical, 1 if different positions, 2 if different content.
 *        Can be NULL, in that case no detection is performed.
 */
ass_image_t *ass_render_frame(ass_renderer_t *priv, ass_track_t *track,
                              long long now, int *detect_change)
{
	int i, cnt, rc;
	event_images_t* last;
	ass_image_t** tail;
	
	// init frame
	rc = ass_start_frame(priv, track, now);
	if (rc != 0)
		return 0;

	// render events separately
	cnt = 0;
	for (i = 0; i < track->n_events; ++i) {
		ass_event_t* event = track->events + i;
        if ((event->Start <= now)
            && (now < (event->Start + event->Duration))) {
			if (cnt >= priv->eimg_size) {
				priv->eimg_size += 100;
                priv->eimg =
                    realloc(priv->eimg,
                            priv->eimg_size * sizeof(event_images_t));
			}
			rc = ass_render_event(priv, event, priv->eimg + cnt);
            if (!rc)
                ++cnt;
		}
	}

	// sort by layer
	qsort(priv->eimg, cnt, sizeof(event_images_t), cmp_event_layer);

	// call fix_collisions for each group of events with the same layer
	last = priv->eimg;
	for (i = 1; i < cnt; ++i)
		if (last->event->Layer != priv->eimg[i].event->Layer) {
			fix_collisions(priv, last, priv->eimg + i - last);
			last = priv->eimg + i;
		}
	if (cnt > 0)
		fix_collisions(priv, last, priv->eimg + cnt - last);

	// concat lists
	tail = &priv->images_root;
	for (i = 0; i < cnt; ++i) {
		ass_image_t* cur = priv->eimg[i].imgs;
		while (cur) {
			*tail = cur;
			tail = &cur->next;
			cur = cur->next;
		}
	}

	if (detect_change)
		*detect_change = ass_detect_change(priv);
	
	// free the previous image list
	ass_free_images(priv->prev_images_root);
	priv->prev_images_root = 0;

	return priv->images_root;
}
