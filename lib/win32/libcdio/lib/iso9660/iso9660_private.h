/*
    $Id: iso9660_private.h,v 1.1 2004/12/18 17:29:32 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>

    See also iso9660.h by Eric Youngdale (1993).

    Copyright 1993 Yggdrasil Computing, Incorporated
    Copyright (c) 1999,2000 J. Schilling

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __CDIO_ISO9660_PRIVATE_H__
#define __CDIO_ISO9660_PRIVATE_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cdio/types.h>

#define ISO_VERSION             1

#if defined(_XBOX) || defined(WIN32)
#pragma pack(1)
#else
PRAGMA_BEGIN_PACKED
#endif

struct iso_volume_descriptor {
  uint8_t  type;      /* 711 */
  char     id[5];
  uint8_t  version;   /* 711 */
  char     data[2041];
} GNUC_PACKED;

#define struct_iso_volume_descriptor_SIZEOF ISO_BLOCKSIZE

#define struct_iso9660_pvd_SIZEOF ISO_BLOCKSIZE

/*
 * XXX JS: The next structure has an odd length!
 * Some compilers (e.g. on Sun3/mc68020) padd the structures to even length.
 * For this reason, we cannot use sizeof (struct iso_path_table) or
 * sizeof (struct iso_directory_record) to compute on disk sizes.
 * Instead, we use offsetof(..., name) and add the name size.
 * See mkisofs.h
 */

/* We use this to help us look up the parent inode numbers. */

struct iso_path_table {
  uint8_t  name_len; /* 711 */
  uint8_t  xa_len;   /* 711 */
  uint32_t extent;   /* 731/732 */
  uint16_t parent;   /* 721/722 */
  char     name[EMPTY_ARRAY_SIZE];
} GNUC_PACKED;

#define struct_iso_path_table_SIZEOF 8

#define struct_iso9660_dir_SIZEOF 33

#if defined(_XBOX) || defined(WIN32)
#pragma pack()
#else
PRAGMA_END_PACKED
#endif

#endif /* __CDIO_ISO9660_PRIVATE_H__ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
