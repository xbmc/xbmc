/*****************************************************************************
 * device.h: DVD device access
 *****************************************************************************
 * Copyright (C) 1998-2002 VideoLAN
 *
 * Authors: Stéphane Borel <stef@via.ecp.fr>
 *          Sam Hocevar <sam@zoy.org>
 *          Håkan Hjort <d95hjort@dtek.chalmers.se>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdcss; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *****************************************************************************/

#ifndef DVDCSS_DEVICE_H
#define DVDCSS_DEVICE_H

#include "config.h"

/*****************************************************************************
 * iovec structure: vectored data entry
 *****************************************************************************/
#ifndef HAVE_SYS_UIO_H
#   include <io.h>                                                 /* read() */
struct iovec
{
    void *iov_base;     /* Pointer to data. */
    size_t iov_len;     /* Length of data.  */
};
#else
#   include <sys/types.h>
#   include <sys/uio.h>                                      /* struct iovec */
#endif

#include "dvdcss/dvdcss.h"

#if !defined(WIN32) && !defined(__OS2__)
#   define DVDCSS_RAW_OPEN
#endif


/*****************************************************************************
 * Device reading prototypes
 *****************************************************************************/
int  _dvdcss_use_ioctls ( dvdcss_t );
void _dvdcss_check      ( dvdcss_t );
int  _dvdcss_open       ( dvdcss_t );
int  _dvdcss_close      ( dvdcss_t );

/*****************************************************************************
 * Device reading prototypes, raw-device specific
 *****************************************************************************/
int _dvdcss_raw_open     ( dvdcss_t, char const * );

#endif /* DVDCSS_DEVICE_H */
