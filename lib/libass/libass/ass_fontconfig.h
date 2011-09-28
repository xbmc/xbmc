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

#ifndef LIBASS_FONTCONFIG_H
#define LIBASS_FONTCONFIG_H

#include <stdint.h>
#include "ass_types.h"
#include "ass.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef CONFIG_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

typedef struct fc_instance FCInstance;

FCInstance *fontconfig_init(ASS_Library *library,
                            FT_Library ftlibrary, const char *family,
                            const char *path, int fc, const char *config,
                            int update);
char *fontconfig_select(ASS_Library *library, FCInstance *priv,
                        const char *family, int treat_family_as_pattern,
                        unsigned bold, unsigned italic, int *index,
                        uint32_t code);
void fontconfig_done(FCInstance *priv);
int fontconfig_update(FCInstance *priv);

#endif                          /* LIBASS_FONTCONFIG_H */
