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
 * $Id: mmsh.h,v 1.8 2007/12/11 20:24:48 jwrdegoede Exp $
 *
 * libmmsh public header
 */

#ifndef HAVE_MMSH_H
#define HAVE_MMSH_H

#include <inttypes.h>
#include <stdio.h>
#include <sys/types.h>
#include "mmsio.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct mmsh_s mmsh_t;

char*    mmsh_connect_common(int *s ,int *port, char *url, char **host, char **path, char **file);
mmsh_t*   mmsh_connect (mms_io_t *io, void *data, const char *url_, int bandwidth);

int      mmsh_read (mms_io_t *io, mmsh_t *instance, char *data, int len);
int      mmsh_time_seek (mms_io_t *io, mmsh_t *instance, double time_sec);
mms_off_t mmsh_seek (mms_io_t *io, mmsh_t *this, mms_off_t offset, int origin);
uint32_t mmsh_get_length (mmsh_t *instance);
double   mmsh_get_time_length (mmsh_t *this);
uint64_t mmsh_get_raw_time_length (mmsh_t *this);
mms_off_t mmsh_get_current_pos (mmsh_t *instance);
void     mmsh_close (mmsh_t *instance);

int      mmsh_peek_header (mmsh_t *instance, char *data, int maxsize);

uint32_t mmsh_get_asf_header_len (mmsh_t *this);

uint32_t mmsh_get_asf_packet_len (mmsh_t *this);

int      mmsh_get_seekable (mmsh_t *this);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
