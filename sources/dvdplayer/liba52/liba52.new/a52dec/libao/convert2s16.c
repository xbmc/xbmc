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

#include "a52.h"
#include "audio_out_internal.h"

#include <stdio.h>

static inline int16_t convert (int32_t i)
{
#ifdef LIBA52_FIXED
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
    if (flags & A52_LFE)
	return 6;
    else if (flags & 1)	/* center channel */
	return 5;
    else if ((flags & A52_CHANNEL_MASK) == A52_2F2R)
	return 4;
    else
	return 2;
}

void convert2s16_multi (convert_t * _f, int16_t * s16, int flags)
{
    int i;
    int32_t * f = (int32_t *) _f;

    switch (flags) {
    case A52_MONO:
    case A52_CHANNEL1:
    case A52_CHANNEL2:
	for (i = 0; i < 256; i++) {
	    s16[5*i] = s16[5*i+1] = s16[5*i+2] = s16[5*i+3] = 0;
	    s16[5*i+4] = convert (f[i]);
	}
	break;
    case A52_CHANNEL:
    case A52_STEREO:
    case A52_DOLBY:
	convert2s16_2 (_f, s16);
	break;
    case A52_3F:
	for (i = 0; i < 256; i++) {
	    s16[5*i] = convert (f[i]);
	    s16[5*i+1] = convert (f[i+512]);
	    s16[5*i+2] = s16[5*i+3] = 0;
	    s16[5*i+4] = convert (f[i+256]);
	}
	break;
    case A52_2F2R:
	convert2s16_4 (_f, s16);
	break;
    case A52_3F2R:
	convert2s16_5 (_f, s16);
	break;
    case A52_MONO | A52_LFE:
    case A52_CHANNEL1 | A52_LFE:
    case A52_CHANNEL2 | A52_LFE:
	for (i = 0; i < 256; i++) {
	    s16[6*i] = s16[6*i+1] = s16[6*i+2] = s16[6*i+3] = 0;
	    s16[6*i+4] = convert (f[i+256]);
	    s16[6*i+5] = convert (f[i]);
	}
	break;
    case A52_CHANNEL | A52_LFE:
    case A52_STEREO | A52_LFE:
    case A52_DOLBY | A52_LFE:
	for (i = 0; i < 256; i++) {
	    s16[6*i] = convert (f[i+256]);
	    s16[6*i+1] = convert (f[i+512]);
	    s16[6*i+2] = s16[6*i+3] = s16[6*i+4] = 0;
	    s16[6*i+5] = convert (f[i]);
	}
	break;
    case A52_3F | A52_LFE:
	for (i = 0; i < 256; i++) {
	    s16[6*i] = convert (f[i+256]);
	    s16[6*i+1] = convert (f[i+768]);
	    s16[6*i+2] = s16[6*i+3] = 0;
	    s16[6*i+4] = convert (f[i+512]);
	    s16[6*i+5] = convert (f[i]);
	}
	break;
    case A52_2F2R | A52_LFE:
	for (i = 0; i < 256; i++) {
	    s16[6*i] = convert (f[i+256]);
	    s16[6*i+1] = convert (f[i+512]);
	    s16[6*i+2] = convert (f[i+768]);
	    s16[6*i+3] = convert (f[i+1024]);
	    s16[6*i+4] = 0;
	    s16[6*i+5] = convert (f[i]);
	}
	break;
    case A52_3F2R | A52_LFE:
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

void convert2s16_wav (convert_t * _f, int16_t * s16, int flags)
{
    int i;
    int32_t * f = (int32_t *) _f;

    switch (flags) {
    case A52_MONO:
    case A52_CHANNEL1:
    case A52_CHANNEL2:
	convert2s16_1 (_f, s16);
	break;
    case A52_CHANNEL:
    case A52_STEREO:
    case A52_DOLBY:
	convert2s16_2 (_f, s16);
	break;
    case A52_3F:
	for (i = 0; i < 256; i++) {
	    s16[3*i] = convert (f[i]);
	    s16[3*i+1] = convert (f[i+512]);
	    s16[3*i+2] = convert (f[i+256]);
	}
	break;
    case A52_2F1R:
	convert2s16_3 (_f, s16);
	break;
    case A52_3F1R:
	for (i = 0; i < 256; i++) {
	    s16[4*i] = convert (f[i]);
	    s16[4*i+1] = convert (f[i+512]);
	    s16[4*i+2] = convert (f[i+256]);
	    s16[4*i+3] = convert (f[i+768]);
	}
	break;
    case A52_2F2R:
	convert2s16_4 (_f, s16);
	break;
    case A52_3F2R:
	for (i = 0; i < 256; i++) {
	    s16[5*i] = convert (f[i]);
	    s16[5*i+1] = convert (f[i+512]);
	    s16[5*i+2] = convert (f[i+256]);
	    s16[5*i+3] = convert (f[i+768]);
	    s16[5*i+4] = convert (f[i+1024]);
	}
	break;
    case A52_MONO | A52_LFE:
    case A52_CHANNEL1 | A52_LFE:
    case A52_CHANNEL2 | A52_LFE:
	for (i = 0; i < 256; i++) {
	    s16[2*i] = convert (f[i+256]);
	    s16[2*i+1] = convert (f[i]);
	}
	break;
    case A52_CHANNEL | A52_LFE:
    case A52_STEREO | A52_LFE:
    case A52_DOLBY | A52_LFE:
	for (i = 0; i < 256; i++) {
	    s16[3*i] = convert (f[i+256]);
	    s16[3*i+1] = convert (f[i+512]);
	    s16[3*i+2] = convert (f[i]);
	}
	break;
    case A52_3F | A52_LFE:
	for (i = 0; i < 256; i++) {
	    s16[4*i] = convert (f[i+256]);
	    s16[4*i+1] = convert (f[i+768]);
	    s16[4*i+2] = convert (f[i+512]);
	    s16[4*i+3] = convert (f[i]);
	}
	break;
    case A52_2F1R | A52_LFE:
	for (i = 0; i < 256; i++) {
	    s16[4*i] = convert (f[i+256]);
	    s16[4*i+1] = convert (f[i+512]);
	    s16[4*i+2] = convert (f[i]);
	    s16[4*i+3] = convert (f[i+768]);
	}
    case A52_3F1R | A52_LFE:
	for (i = 0; i < 256; i++) {
	    s16[5*i] = convert (f[i+256]);
	    s16[5*i+1] = convert (f[i+768]);
	    s16[5*i+2] = convert (f[i+512]);
	    s16[5*i+3] = convert (f[i]);
	    s16[5*i+4] = convert (f[i+1024]);
	}
	break;
    case A52_2F2R | A52_LFE:
	for (i = 0; i < 256; i++) {
	    s16[5*i] = convert (f[i+256]);
	    s16[5*i+1] = convert (f[i+512]);
	    s16[5*i+2] = convert (f[i]);
	    s16[5*i+3] = convert (f[i+768]);
	    s16[5*i+4] = convert (f[i+1024]);
	}
	break;
    case A52_3F2R | A52_LFE:
	for (i = 0; i < 256; i++) {
	    s16[6*i] = convert (f[i+256]);
	    s16[6*i+1] = convert (f[i+768]);
	    s16[6*i+2] = convert (f[i+512]);
	    s16[6*i+3] = convert (f[i]);
	    s16[6*i+4] = convert (f[i+1024]);
	    s16[6*i+5] = convert (f[i+1280]);
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
