/*
 * Copyright (C) 2007 Hans de Goede <j.w.r.degoede@hhs.nl>
 *
 * This file is part of libmms a free mms protocol library
 *
 * libmms is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libmss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * libmms public header
 */

/*
 * mmsx is a small wrapper around the mms and mmsh protocol implementations
 * in libmms. The mmsx functions provide transparent access to both protocols
 * so that programs who wish to support both can do so with a single code path
 * if desired.
 */
 
#ifndef HAVE_MMSX_H
#define HAVE_MMSX_H

#include <inttypes.h>
#include <libmms/mmsio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct mmsx_s mmsx_t;

mmsx_t*   mmsx_connect (mms_io_t *io, void *data, const char *url, int bandwidth);

int       mmsx_read (mms_io_t *io, mmsx_t *instance, char *data, int len);
int       mmsx_time_seek (mms_io_t *io, mmsx_t *instance, double time_sec);
mms_off_t mmsx_seek (mms_io_t *io, mmsx_t *instance, mms_off_t offset, int origin);
/* return total playback time in seconds */
double    mmsx_get_time_length (mmsx_t *instance);
/* return raw total playback time in 100 nanosecs (10^-7) */
uint64_t  mmsx_get_raw_time_length (mmsx_t *instance);
uint32_t  mmsx_get_length (mmsx_t *instance);
void      mmsx_close (mmsx_t *instance);

int       mmsx_peek_header (mmsx_t *instance, char *data, int maxsize);

mms_off_t mmsx_get_current_pos (mmsx_t *instance);

uint32_t  mmsx_get_asf_header_len (mmsx_t *);

uint64_t  mmsx_get_asf_packet_len (mmsx_t *);

int       mmsx_get_seekable (mmsx_t *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
 
