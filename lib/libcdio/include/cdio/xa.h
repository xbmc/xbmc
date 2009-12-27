/*
    $Id: xa.h,v 1.9 2004/06/26 00:47:50 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>

    See also iso9660.h by Eric Youngdale (1993) and in cdrtools. These
    are 

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
/*!
   \file xa.h 
   \brief Things related to the ISO-9660 XA (Extended Attributes) format
*/


#ifndef __CDIO_XA_H__
#define __CDIO_XA_H__

#include <cdio/types.h>

#define ISO_XA_MARKER_STRING    "CD-XA001"
#define ISO_XA_MARKER_OFFSET    1024

/* XA attribute definitions */
#define XA_PERM_RSYS          0x0001   /**< System Group Read */
#define XA_PERM_XSYS          0x0004   /**< System Group Execute */

#define XA_PERM_RUSR          0x0010   /**< User (owner) Read */
#define XA_PERM_XUSR          0x0040   /**< User (owner) Execute */

#define XA_PERM_RGRP          0x0100   /**< Group Read */
#define XA_PERM_XGRP          0x0400   /**< Group Execute */

#define	XA_PERM_ROTH	      0x1000   /**< Other (world) Read */
#define	XA_PERM_XOTH	      0x4000   /**< Other (world) Execute */

#define XA_ATTR_MODE2FORM1     (1 << 11)
#define XA_ATTR_MODE2FORM2     (1 << 12)
#define XA_ATTR_INTERLEAVED    (1 << 13)
#define XA_ATTR_CDDA           (1 << 14)
#define XA_ATTR_DIRECTORY      (1 << 15)

/* some aggregations */
#define XA_PERM_ALL_READ       (XA_PERM_RUSR | XA_PERM_RSYS | XA_PERM_RGRP)
#define XA_PERM_ALL_EXEC       (XA_PERM_XUSR | XA_PERM_XSYS | XA_PERM_XGRP)
#define XA_PERM_ALL_ALL        (XA_PERM_ALL_READ | XA_PERM_ALL_EXEC)

#define XA_FORM1_DIR    (XA_ATTR_DIRECTORY | XA_ATTR_MODE2FORM1 | XA_PERM_ALL_ALL)
#define XA_FORM1_FILE   (XA_ATTR_MODE2FORM1 | XA_PERM_ALL_ALL)
#define XA_FORM2_FILE   (XA_ATTR_MODE2FORM2 | XA_PERM_ALL_ALL)

/*! \brief "Extended Architecture according to the Philips Yellow Book.
 
CD-ROM EXtended Architecture is a modification to the CD-ROM
specification that defines two new types of sectors.  CD-ROM XA was
developed jointly by Sony, Philips, and Microsoft, and announced in
August 1988. Its specifications were published in an extension to the
Yellow Book.  CD-i, Photo CD, Video CD and CD-EXTRA have all
subsequently been based on CD-ROM XA.

CD-XA defines another way of formatting sectors on a CD-ROM, including
headers in the sectors that describe the type (audio, video, data) and
some additional info (markers, resolution in case of a video or audio
sector, file numbers, etc).

The data written on a CD-XA is consistent with and can be in ISO-9660
file system format and therefore be readable by ISO-9660 file system
translators. But also a CD-I player can also read CD-XA discs even if
its own `Green Book' file system only resembles ISO 9660 and isn't
fully compatible. 

 Note structure is big-endian.
*/
typedef struct iso9660_xa 
{
  uint16_t group_id;      /**< 0 */
  uint16_t user_id;       /**< 0 */
  uint16_t attributes;    /**< XA_ATTR_ */ 
  uint8_t  signature[2];  /**< { 'X', 'A' } */
  uint8_t  filenum;       /**< file number, see also XA subheader */
  uint8_t  reserved[5];   /**< zero */
} GNUC_PACKED iso9660_xa_t;


/*!
  Returns a string which interpreting the extended attribute xa_attr. 
  For example:
  \verbatim
  d---1xrxrxr
  ---2--r-r-r
  -a--1xrxrxr
  \endverbatim
  
  A description of the characters in the string follows
  The 1st character is either "d" if the entry is a directory, or "-" if not
  The 2nd character is either "a" if the entry is CDDA (audio), or "-" if not
  The 3rd character is either "i" if the entry is interleaved, or "-" if not
  The 4th character is either "2" if the entry is mode2 form2 or "-" if not
  The 5th character is either "1" if the entry is mode2 form1 or "-" if not
  Note that an entry will either be in mode2 form1 or mode form2. That
  is you will either see "2-" or "-1" in the 4th & 5th positions.
  
  The 6th and 7th characters refer to permissions for a user while the
  the 8th and 9th characters refer to permissions for a group while, and 
  the 10th and 11th characters refer to permissions for everyone. 
  
  In each of these pairs the first character (6, 8, 10) is "x" if the 
  entry is executable. For a directory this means the directory is
  allowed to be listed or "searched".
  The second character of a pair (7, 9, 11) is "r" if the entry is allowed
  to be read. 
*/
const char *
iso9660_get_xa_attr_str (uint16_t xa_attr);
  
/*! 
  Allocates and initalizes a new iso9600_xa_t variable and returns
  it. The caller should free the returned result.

  @see iso9660_xa
*/
iso9660_xa_t *
iso9660_xa_init (iso9660_xa_t *_xa, uint16_t uid, uint16_t gid, uint16_t attr, 
		 uint8_t filenum);

#endif /* __CDIO_XA_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
