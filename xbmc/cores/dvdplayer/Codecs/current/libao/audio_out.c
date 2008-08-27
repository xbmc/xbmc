/*
 * audio_out.c
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
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

#include <stdlib.h>
#include <inttypes.h>

#include "a52.h"
#include "audio_out.h"

extern ao_open_t ao_oss_open;
extern ao_open_t ao_ossdolby_open;
extern ao_open_t ao_oss4_open;
extern ao_open_t ao_oss6_open;
extern ao_open_t ao_solaris_open;
extern ao_open_t ao_solarisdolby_open;
extern ao_open_t ao_al_open;
extern ao_open_t ao_aldolby_open;
extern ao_open_t ao_al4_open;
extern ao_open_t ao_al6_open;
extern ao_open_t ao_win_open;
extern ao_open_t ao_windolby_open;
extern ao_open_t ao_wav_open;
extern ao_open_t ao_wavdolby_open;
extern ao_open_t ao_aif_open;
extern ao_open_t ao_aifdolby_open;
extern ao_open_t ao_peak_open;
extern ao_open_t ao_peakdolby_open;
extern ao_open_t ao_null_open;
extern ao_open_t ao_null4_open;
extern ao_open_t ao_null6_open;
extern ao_open_t ao_float_open;

static ao_driver_t audio_out_drivers[] = {
#ifdef LIBAO_OSS
    {"oss", ao_oss_open},
    {"ossdolby", ao_ossdolby_open},
    {"oss4", ao_oss4_open},
    {"oss6", ao_oss6_open},
#endif
#ifdef LIBAO_SOLARIS
    {"solaris", ao_solaris_open},
    {"solarisdolby", ao_solarisdolby_open},
#endif
#ifdef LIBAO_AL
    {"al", ao_al_open},
    {"aldolby", ao_aldolby_open},
    {"al4", ao_al4_open},
    {"al6", ao_al6_open},
#endif
#ifdef LIBAO_WIN
    {"win", ao_win_open},
    {"windolby", ao_windolby_open},
#endif
    {"wav", ao_wav_open},
    {"wavdolby", ao_wavdolby_open},
    {"aif", ao_aif_open},
    {"aifdolby", ao_aifdolby_open},
    {"peak", ao_peak_open},
    {"peakdolby", ao_peakdolby_open},
    {"null", ao_null_open},
    {"null4", ao_null4_open},
    {"null6", ao_null6_open},
    {"float", ao_float_open},
    {NULL, NULL}
};

ao_driver_t * ao_drivers (void)
{
    return audio_out_drivers;
}
