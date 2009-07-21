/* grabbag - Convenience lib for various routines common to several tools
 * Copyright (C) 2006,2007  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "share/alloc.h"
#include "share/grabbag.h"
#include "FLAC/assert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* slightly different that strndup(): this always copies 'size' bytes starting from s into a NUL-terminated string. */
static char *local__strndup_(const char *s, size_t size)
{
	char *x = (char*)safe_malloc_add_2op_(size, /*+*/1);
	if(x) {
		memcpy(x, s, size);
		x[size] = '\0';
	}
	return x;
}

static FLAC__bool local__parse_type_(const char *s, size_t len, FLAC__StreamMetadata_Picture *picture)
{
	size_t i;
	FLAC__uint32 val = 0;

	picture->type = FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER;

	if(len == 0)
		return true; /* empty string implies default to 'front cover' */

	for(i = 0; i < len; i++) {
		if(s[i] >= '0' && s[i] <= '9')
			val = 10*val + (FLAC__uint32)(s[i] - '0');
		else
			return false;
	}

	if(i == len)
		picture->type = val;
	else
		return false;

	return true;
}

static FLAC__bool local__parse_resolution_(const char *s, size_t len, FLAC__StreamMetadata_Picture *picture)
{
	int state = 0;
	size_t i;
	FLAC__uint32 val = 0;

	picture->width = picture->height = picture->depth = picture->colors = 0;

	if(len == 0)
		return true; /* empty string implies client wants to get info from the file itself */

	for(i = 0; i < len; i++) {
		if(s[i] == 'x') {
			if(state == 0)
				picture->width = val;
			else if(state == 1)
				picture->height = val;
			else
				return false;
			state++;
			val = 0;
		}
		else if(s[i] == '/') {
			if(state == 2)
				picture->depth = val;
			else
				return false;
			state++;
			val = 0;
		}
		else if(s[i] >= '0' && s[i] <= '9')
			val = 10*val + (FLAC__uint32)(s[i] - '0');
		else
			return false;
	}

	if(state < 2)
		return false;
	else if(state == 2)
		picture->depth = val;
	else if(state == 3)
		picture->colors = val;
	else
		return false;
	if(picture->depth < 32 && 1u<<picture->depth < picture->colors)
		return false;

	return true;
}

static FLAC__bool local__extract_mime_type_(FLAC__StreamMetadata *obj)
{
	if(obj->data.picture.data_length >= 8 && 0 == memcmp(obj->data.picture.data, "\x89PNG\x0d\x0a\x1a\x0a", 8))
		return FLAC__metadata_object_picture_set_mime_type(obj, "image/png", /*copy=*/true);
	else if(obj->data.picture.data_length >= 6 && (0 == memcmp(obj->data.picture.data, "GIF87a", 6) || 0 == memcmp(obj->data.picture.data, "GIF89a", 6)))
		return FLAC__metadata_object_picture_set_mime_type(obj, "image/gif", /*copy=*/true);
	else if(obj->data.picture.data_length >= 2 && 0 == memcmp(obj->data.picture.data, "\xff\xd8", 2))
		return FLAC__metadata_object_picture_set_mime_type(obj, "image/jpeg", /*copy=*/true);
	return false;
}

static FLAC__bool local__extract_resolution_color_info_(FLAC__StreamMetadata_Picture *picture)
{
	const FLAC__byte *data = picture->data;
	FLAC__uint32 len = picture->data_length;

	if(0 == strcmp(picture->mime_type, "image/png")) {
		/* c.f. http://www.w3.org/TR/PNG/ */
		FLAC__bool need_palette = false; /* if IHDR has color_type=3, we need to also read the PLTE chunk to get the #colors */
		if(len < 8 || memcmp(data, "\x89PNG\x0d\x0a\x1a\x0a", 8))
			return false;
		/* try to find IHDR chunk */
		data += 8;
		len -= 8;
		while(len > 12) { /* every PNG chunk must be at least 12 bytes long */
			const FLAC__uint32 clen = (FLAC__uint32)data[0] << 24 | (FLAC__uint32)data[1] << 16 | (FLAC__uint32)data[2] << 8 | (FLAC__uint32)data[3];
			if(0 == memcmp(data+4, "IHDR", 4) && clen == 13) {
				unsigned color_type = data[17];
				picture->width = (FLAC__uint32)data[8] << 24 | (FLAC__uint32)data[9] << 16 | (FLAC__uint32)data[10] << 8 | (FLAC__uint32)data[11];
				picture->height = (FLAC__uint32)data[12] << 24 | (FLAC__uint32)data[13] << 16 | (FLAC__uint32)data[14] << 8 | (FLAC__uint32)data[15];
				if(color_type == 3) {
					/* even though the bit depth for color_type==3 can be 1,2,4,or 8,
					 * the spec in 11.2.2 of http://www.w3.org/TR/PNG/ says that the
					 * sample depth is always 8
					 */
					picture->depth = 8 * 3u;
					need_palette = true;
					data += 12 + clen;
					len -= 12 + clen;
				}
				else {
					if(color_type == 0) /* greyscale, 1 sample per pixel */
						picture->depth = (FLAC__uint32)data[16];
					if(color_type == 2) /* truecolor, 3 samples per pixel */
						picture->depth = (FLAC__uint32)data[16] * 3u;
					if(color_type == 4) /* greyscale+alpha, 2 samples per pixel */
						picture->depth = (FLAC__uint32)data[16] * 2u;
					if(color_type == 6) /* truecolor+alpha, 4 samples per pixel */
						picture->depth = (FLAC__uint32)data[16] * 4u;
					picture->colors = 0;
					return true;
				}
			}
			else if(need_palette && 0 == memcmp(data+4, "PLTE", 4)) {
				picture->colors = clen / 3u;
				return true;
			}
			else if(clen + 12 > len)
				return false;
			else {
				data += 12 + clen;
				len -= 12 + clen;
			}
		}
	}
	else if(0 == strcmp(picture->mime_type, "image/jpeg")) {
		/* c.f. http://www.w3.org/Graphics/JPEG/itu-t81.pdf and Q22 of http://www.faqs.org/faqs/jpeg-faq/part1/ */
		if(len < 2 || memcmp(data, "\xff\xd8", 2))
			return false;
		data += 2;
		len -= 2;
		while(1) {
			/* look for sync FF byte */
			for( ; len > 0; data++, len--) {
				if(*data == 0xff)
					break;
			}
			if(len == 0)
				return false;
			/* eat any extra pad FF bytes before marker */
			for( ; len > 0; data++, len--) {
				if(*data != 0xff)
					break;
			}
			if(len == 0)
				return false;
			/* if we hit SOS or EOI, bail */
			if(*data == 0xda || *data == 0xd9)
				return false;
			/* looking for some SOFn */
			else if(memchr("\xc0\xc1\xc2\xc3\xc5\xc6\xc7\xc9\xca\xcb\xcd\xce\xcf", *data, 13)) {
				data++; len--; /* skip marker byte */
				if(len < 2)
					return false;
				else {
					const FLAC__uint32 clen = (FLAC__uint32)data[0] << 8 | (FLAC__uint32)data[1];
					if(clen < 8 || len < clen)
						return false;
					picture->width = (FLAC__uint32)data[5] << 8 | (FLAC__uint32)data[6];
					picture->height = (FLAC__uint32)data[3] << 8 | (FLAC__uint32)data[4];
					picture->depth = (FLAC__uint32)data[2] * (FLAC__uint32)data[7];
					picture->colors = 0;
					return true;
				}
			}
			/* else skip it */
			else {
				data++; len--; /* skip marker byte */
				if(len < 2)
					return false;
				else {
					const FLAC__uint32 clen = (FLAC__uint32)data[0] << 8 | (FLAC__uint32)data[1];
					if(clen < 2 || len < clen)
						return false;
					data += clen;
					len -= clen;
				}
			}
		}
	}
	else if(0 == strcmp(picture->mime_type, "image/gif")) {
		/* c.f. http://www.w3.org/Graphics/GIF/spec-gif89a.txt */
		if(len < 14)
			return false;
		if(memcmp(data, "GIF87a", 6) && memcmp(data, "GIF89a", 6))
			return false;
#if 0
		/* according to the GIF spec, even if the GCTF is 0, the low 3 bits should still tell the total # colors used */
		if(data[10] & 0x80 == 0)
			return false;
#endif
		picture->width = (FLAC__uint32)data[6] | ((FLAC__uint32)data[7] << 8);
		picture->height = (FLAC__uint32)data[8] | ((FLAC__uint32)data[9] << 8);
#if 0
		/* this value doesn't seem to be reliable... */
		picture->depth = (((FLAC__uint32)(data[10] & 0x70) >> 4) + 1) * 3u;
#else
		/* ...just pessimistically assume it's 24-bit color without scanning all the color tables */
		picture->depth = 8u * 3u;
#endif
		picture->colors = 1u << ((FLAC__uint32)(data[10] & 0x07) + 1u);
		return true;
	}
	return false;
}

FLAC__StreamMetadata *grabbag__picture_parse_specification(const char *spec, const char **error_message)
{
	FLAC__StreamMetadata *obj;
	int state = 0;
	static const char *error_messages[] = {
		"memory allocation error",
		"invalid picture specification",
		"invalid picture specification: can't parse resolution/color part",
		"unable to extract resolution and color info from URL, user must set explicitly",
		"unable to extract resolution and color info from file, user must set explicitly",
		"error opening picture file",
		"error reading picture file",
		"invalid picture type",
		"unable to guess MIME type from file, user must set explicitly",
		"type 1 icon must be a 32x32 pixel PNG"
	};

	FLAC__ASSERT(0 != spec);
	FLAC__ASSERT(0 != error_message);

	/* double protection */
	if(0 == spec)
		return 0;
	if(0 == error_message)
		return 0;

	*error_message = 0;

	if(0 == (obj = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PICTURE)))
		*error_message = error_messages[0];

	if(strchr(spec, '|')) { /* full format */
		const char *p;
		char *q;
		for(p = spec; *error_message==0 && *p; ) {
			if(*p == '|') {
				switch(state) {
					case 0: /* type */
						if(!local__parse_type_(spec, p-spec, &obj->data.picture))
							*error_message = error_messages[7];
						break;
					case 1: /* mime type */
						if(p-spec) { /* if blank, we'll try to guess later from the picture data */
							if(0 == (q = local__strndup_(spec, p-spec)))
								*error_message = error_messages[0];
							else if(!FLAC__metadata_object_picture_set_mime_type(obj, q, /*copy=*/false))
								*error_message = error_messages[0];
						}
						break;
					case 2: /* description */
						if(0 == (q = local__strndup_(spec, p-spec)))
							*error_message = error_messages[0];
						else if(!FLAC__metadata_object_picture_set_description(obj, (FLAC__byte*)q, /*copy=*/false))
							*error_message = error_messages[0];
						break;
					case 3: /* resolution/color (e.g. [300x300x16[/1234]] */
						if(!local__parse_resolution_(spec, p-spec, &obj->data.picture))
							*error_message = error_messages[2];
						break;
					default:
						*error_message = error_messages[1];
						break;
				}
				p++;
				spec = p;
				state++;
			}
			else
				p++;
		}
	}
	else { /* simple format, filename only, everything else guessed */
		if(!local__parse_type_("", 0, &obj->data.picture)) /* use default picture type */
			*error_message = error_messages[7];
		/* leave MIME type to be filled in later */
		/* leave description empty */
		/* leave the rest to be filled in later: */
		else if(!local__parse_resolution_("", 0, &obj->data.picture))
			*error_message = error_messages[2];
		else
			state = 4;
	}

	/* parse filename, read file, try to extract resolution/color info if needed */
	if(*error_message == 0) {
		if(state != 4)
			*error_message = error_messages[1];
		else { /* 'spec' points to filename/URL */
			if(0 == strcmp(obj->data.picture.mime_type, "-->")) { /* magic MIME type means URL */
				if(!FLAC__metadata_object_picture_set_data(obj, (FLAC__byte*)spec, strlen(spec), /*copy=*/true))
					*error_message = error_messages[0];
				else if(obj->data.picture.width == 0 || obj->data.picture.height == 0 || obj->data.picture.depth == 0)
					*error_message = error_messages[3];
			}
			else { /* regular picture file */
				const off_t size = grabbag__file_get_filesize(spec);
				if(size < 0)
					*error_message = error_messages[5];
				else {
					FLAC__byte *buffer = (FLAC__byte*)safe_malloc_(size);
					if(0 == buffer)
						*error_message = error_messages[0];
					else {
						FILE *f = fopen(spec, "rb");
						if(0 == f)
							*error_message = error_messages[5];
						else {
							if(fread(buffer, 1, size, f) != (size_t)size)
								*error_message = error_messages[6];
							fclose(f);
							if(0 == *error_message) {
								if(!FLAC__metadata_object_picture_set_data(obj, buffer, size, /*copy=*/false))
									*error_message = error_messages[6];
								/* try to extract MIME type if user left it blank */
								else if(*obj->data.picture.mime_type == '\0' && !local__extract_mime_type_(obj))
									*error_message = error_messages[8];
								/* try to extract resolution/color info if user left it blank */
								else if((obj->data.picture.width == 0 || obj->data.picture.height == 0 || obj->data.picture.depth == 0) && !local__extract_resolution_color_info_(&obj->data.picture))
									*error_message = error_messages[4];
							}
						}
					}
				}
			}
		}
	}

	if(*error_message == 0) {
		if(
			obj->data.picture.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FILE_ICON_STANDARD && 
			(
				(strcmp(obj->data.picture.mime_type, "image/png") && strcmp(obj->data.picture.mime_type, "-->")) ||
				obj->data.picture.width != 32 ||
				obj->data.picture.height != 32
			)
		)
			*error_message = error_messages[9];
	}

	if(*error_message && obj) {
		FLAC__metadata_object_delete(obj);
		obj = 0;
	}

	return obj;
}
