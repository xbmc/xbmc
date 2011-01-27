/*
 * Copyright (C) 2002-2003 the xine project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: mms.h,v 1.15 2007/12/11 20:24:48 jwrdegoede Exp $
 *
 * libmms public header
 */

/* TODO/dexineification:
 * + functions needed:
 * 	- _x_io_*()
 * 	- xine_malloc() [?]
 * 	- xine_fast_memcpy() [?]
 */

#ifndef HAVE_MMS_H
#define HAVE_MMS_H

#include <inttypes.h>
#include <stdio.h>
#include <sys/types.h>

/* #include "xine_internal.h" */

#include "bswap.h"
#include "mmsio.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct mms_s mms_t;

int      mms_sock_init (void);
mms_t*   mms_connect (mms_io_t *io, void *data, const char *url, int bandwidth);

int      mms_read (mms_io_t *io, mms_t *instance, char *data, int len);
int      mms_request_time_seek (mms_io_t *io, mms_t *instance, double time_sec);
int      mms_time_seek (mms_io_t *io, mms_t *instance, double time_sec);
int      mms_request_packet_seek (mms_io_t *io, mms_t *instance,
                                  unsigned long packet_seq);
/*
 * mms_seek() will try to seek using mms_request_packet_seek(), if the server
 * ignore the packet seek command, it will return unchanged current_pos, rather
 * than trying to mms_read() until the destination pos is reached.  This is to
 * let the caller, by itself, to decide to choose the alternate method, such
 * as, mms_time_seek() and/or mms_read() until the destination pos is reached.
 * One can do binary search using time offset (mms_time_seek()) as a search
 * index, to approach the desired byte offset.  It is to systematically guess
 * the time offset to reach for the byte offset.
 */
mms_off_t mms_seek (mms_io_t *io, mms_t *instance, mms_off_t offset, int origin);
/* return total playback time in seconds */
double   mms_get_time_length (mms_t *instance);
/* return raw total playback time in 100 nanosecs (10^-7) */
uint64_t mms_get_raw_time_length (mms_t *instance);
uint32_t mms_get_length (mms_t *instance);
void     mms_close (mms_t *instance);

int      mms_peek_header (mms_t *instance, char *data, int maxsize);

mms_off_t mms_get_current_pos (mms_t *instance);

uint32_t mms_get_asf_header_len (mms_t *instance);

uint64_t mms_get_asf_packet_len (mms_t *instance);

int      mms_get_seekable (mms_t *instance);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

