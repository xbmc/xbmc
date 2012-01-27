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

#ifndef LIBASS_TYPES_H
#define LIBASS_TYPES_H

#include <stdint.h>

#define VALIGN_SUB 0
#define VALIGN_CENTER 8
#define VALIGN_TOP 4
#define HALIGN_LEFT 1
#define HALIGN_CENTER 2
#define HALIGN_RIGHT 3

/* Opaque objects internally used by libass.  Contents are private. */
typedef struct ass_renderer ASS_Renderer;
typedef struct render_priv ASS_RenderPriv;
typedef struct parser_priv ASS_ParserPriv;
typedef struct ass_library ASS_Library;

/* ASS Style: line */
typedef struct ass_style {
    char *Name;
    char *FontName;
    double FontSize;
    uint32_t PrimaryColour;
    uint32_t SecondaryColour;
    uint32_t OutlineColour;
    uint32_t BackColour;
    int Bold;
    int Italic;
    int Underline;
    int StrikeOut;
    double ScaleX;
    double ScaleY;
    double Spacing;
    int Angle;
    int BorderStyle;
    double Outline;
    double Shadow;
    int Alignment;
    int MarginL;
    int MarginR;
    int MarginV;
    int Encoding;
    int treat_fontname_as_pattern;
} ASS_Style;

/*
 * ASS_Event corresponds to a single Dialogue line;
 * text is stored as-is, style overrides will be parsed later.
 */
typedef struct ass_event {
    long long Start;            // ms
    long long Duration;         // ms

    int ReadOrder;
    int Layer;
    int Style;
    char *Name;
    int MarginL;
    int MarginR;
    int MarginV;
    char *Effect;
    char *Text;

    ASS_RenderPriv *render_priv;
} ASS_Event;

/*
 * ass track represent either an external script or a matroska subtitle stream
 * (no real difference between them); it can be used in rendering after the
 * headers are parsed (i.e. events format line read).
 */
typedef struct ass_track {
    int n_styles;           // amount used
    int max_styles;         // amount allocated
    int n_events;
    int max_events;
    ASS_Style *styles;    // array of styles, max_styles length, n_styles used
    ASS_Event *events;    // the same as styles

    char *style_format;     // style format line (everything after "Format: ")
    char *event_format;     // event format line

    enum {
        TRACK_TYPE_UNKNOWN = 0,
        TRACK_TYPE_ASS,
        TRACK_TYPE_SSA
    } track_type;

    // Script header fields
    int PlayResX;
    int PlayResY;
    double Timer;
    int WrapStyle;
    int ScaledBorderAndShadow;
    int Kerning;

    int default_style;      // index of default style
    char *name;             // file name in case of external subs, 0 for streams

    ASS_Library *library;
    ASS_ParserPriv *parser_priv;
} ASS_Track;

#endif /* LIBASS_TYPES_H */
