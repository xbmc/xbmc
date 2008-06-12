/*
 * convert2s16.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of a52dec, a free ATSC A-52 stream decoder.
 * See http://liba52.sourceforge.net/ for updates.
 *
 * a52dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * a52dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <inttypes.h>

#include "dts.h"
#include "audio_out_internal.h"

#include <stdio.h>

static inline int16_t convert (int32_t i)
{
#ifdef LIBDTS_FIXED
    i >>= 15;
#else
    i -= 0x43c00000;
#endif
    return (i > 32767) ? 32767 : ((i < -32768) ? -32768 : i);
}

void convert2s16_1 (convert_t * _f, int16_t * s16)
{
    int i;
    int32_t * f = (int32_t *) _f;

    for (i = 0; i < 256; i++) {
	s16[i] = convert (f[i]);
    }
}

void convert2s16_2 (convert_t * _f, int16_t * s16)
{
    int i;
    int32_t * f = (int32_t *) _f;

    for (i = 0; i < 256; i++) {
	s16[2*i] = convert (f[i]);
	s16[2*i+1] = convert (f[i+256]);
    }
}

void convert2s16_3 (convert_t * _f, int16_t * s16)
{
    int i;
    int32_t * f = (int32_t *) _f;

    for (i = 0; i < 256; i++) {
	s16[3*i] = convert (f[i]);
	s16[3*i+1] = convert (f[i+256]);
	s16[3*i+2] = convert (f[i+512]);
    }
}

void convert2s16_4 (convert_t * _f, int16_t * s16)
{
    int i;
    int32_t * f = (int32_t *) _f;

    for (i = 0; i < 256; i++) {
	s16[4*i] = convert (f[i]);
	s16[4*i+1] = convert (f[i+256]);
	s16[4*i+2] = convert (f[i+512]);
	s16[4*i+3] = convert (f[i+768]);
    }
}

void convert2s16_5 (convert_t * _f, int16_t * s16)
{
    int i;
    int32_t * f = (int32_t *) _f;

    for (i = 0; i < 256; i++) {
	s16[5*i] = convert (f[i]);
	s16[5*i+1] = convert (f[i+256]);
	s16[5*i+2] = convert (f[i+512]);
	s16[5*i+3] = convert (f[i+768]);
	s16[5*i+4] = convert (f[i+1024]);
    }
}

int channels_multi (int flags)
{
    if (flags & DTS_LFE)
	return 6;
    else if (flags & 1)	/* center channel */
	return 5;
    else if ((flags & DTS_CHANNEL_MASK) == DTS_2F2R)
	return 4;
    else
	return 2;
}

void convert2s16_multi (convert_t * _f, int16_t * s16, int flags)
{
    int i;
    int32_t * f = (int32_t *) _f;

    switch (flags) {
    case DTS_MONO:
	for (i = 0; i < 256; i++) {
	    s16[5*i] = s16[5*i+1] = s16[5*i+2] = s16[5*i+3] = 0;
	    s16[5*i+4] = convert (f[i]);
	}
	break;
    case DTS_CHANNEL:
    case DTS_STEREO:
    case DTS_DOLBY:
	convert2s16_2 (_f, s16);
	break;
    case DTS_3F:
	for (i = 0; i < 256; i++) {
	    s16[5*i] = convert (f[i]);
	    s16[5*i+1] = convert (f[i+512]);
	    s16[5*i+2] = s16[5*i+3] = 0;
	    s16[5*i+4] = convert (f[i+256]);
	}
	break;
    case DTS_2F2R:
	convert2s16_4 (_f, s16);
	break;
    case DTS_3F2R:
	convert2s16_5 (_f, s16);
	break;
    case DTS_MONO | DTS_LFE:
	for (i = 0; i < 256; i++) {
	    s16[6*i] = s16[6*i+1] = s16[6*i+2] = s16[6*i+3] = 0;
	    s16[6*i+4] = convert (f[i+256]);
	    s16[6*i+5] = convert (f[i]);
	}
	break;
    case DTS_CHANNEL | DTS_LFE:
    case DTS_STEREO | DTS_LFE:
    case DTS_DOLBY | DTS_LFE:
	for (i = 0; i < 256; i++) {
	    s16[6*i] = convert (f[i+256]);
	    s16[6*i+1] = convert (f[i+512]);
	    s16[6*i+2] = s16[6*i+3] = s16[6*i+4] = 0;
	    s16[6*i+5] = convert (f[i]);
	}
	break;
    case DTS_3F | DTS_LFE:
	for (i = 0; i < 256; i++) {
	    s16[6*i] = convert (f[i+256]);
	    s16[6*i+1] = convert (f[i+768]);
	    s16[6*i+2] = s16[6*i+3] = 0;
	    s16[6*i+4] = convert (f[i+512]);
	    s16[6*i+5] = convert (f[i]);
	}
	break;
    case DTS_2F2R | DTS_LFE:
	for (i = 0; i < 256; i++) {
	    s16[6*i] = convert (f[i+256]);
	    s16[6*i+1] = convert (f[i+512]);
	    s16[6*i+2] = convert (f[i+768]);
	    s16[6*i+3] = convert (f[i+1024]);
	    s16[6*i+4] = 0;
	    s16[6*i+5] = convert (f[i]);
	}
	break;
    case DTS_3F2R | DTS_LFE:
	for (i = 0; i < 256; i++) {
	    s16[6*i] = convert (f[i+256]);
	    s16[6*i+1] = convert (f[i+768]);
	    s16[6*i+2] = convert (f[i+1024]);
	    s16[6*i+3] = convert (f[i+1280]);
	    s16[6*i+4] = convert (f[i+512]);
	    s16[6*i+5] = convert (f[i]);
	}
	break;
    }
}

void s16_swap (int16_t * s16, int channels)
{
    int i;
    uint16_t * u16 = (uint16_t *) s16;

    for (i = 0; i < 256 * channels; i++)
	u16[i] = (u16[i] >> 8) | (u16[i] << 8);
}

void s32_swap (int32_t * s32, int channels)
{
    int i;
    uint32_t * u32 = (uint32_t *) s32;

    for (i = 0; i < 256 * channels; i++)
	u32[i] = (u32[i] << 24) | ((u32[i] << 8)&0xFF0000) |
                 ((u32[i] >> 8)&0xFF00) | (u32[i] >> 24);
}
