/*
    $Id: cdtext_private.h,v 1.1 2004/12/18 17:29:32 rocky Exp $

    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>

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

#ifndef __CDIO_CDTEXT_PRIVATE_H__
#define __CDIO_CDTEXT_PRIVATE_H__

#include <cdio/cdio.h>
#include <cdio/cdtext.h>

#define CDIO_CDTEXT_MAX_PACK_DATA  255
#define CDIO_CDTEXT_MAX_TEXT_DATA  12

/* From table J.2 - Pack Type Indicator Definitions from 
   Working Draft NCITS XXX T10/1364-D Revision 10G. November 12, 2001.
*/
/* Title of Alubm name (ID=0) or Track Titles (ID != 0) */
#define CDIO_CDTEXT_TITLE      0x80 

/* Name(s) of the performer(s) in ASCII */
#define CDIO_CDTEXT_PERFORMER  0x81

/* Name(s) of the songwriter(s) in ASCII */
#define CDIO_CDTEXT_SONGWRITER 0x82

/* Name(s) of the Composers in ASCII */
#define CDIO_CDTEXT_COMPOSER   0x83

/* Name(s) of the Arrangers in ASCII */
#define CDIO_CDTEXT_ARRANGER   0x84

/* Message(s) from content provider and/or artist in ASCII */
#define CDIO_CDTEXT_MESSAGE    0x85

/* Disc Identificatin information */
#define CDIO_CDTEXT_DISCID     0x86

/* Genre Identification and Genre Information */
#define CDIO_CDTEXT_GENRE      0x87

/* Table of Content Information */
#define CDIO_CDTEXT_TOC        0x88

/* Second Table of Content Information */
#define CDIO_CDTEXT_TOC2       0x89

/* 0x8A, 0x8B, 0x8C are reserved
   0x8D Reserved for content provider only.
 */

/* UPC/EAN code of the album and ISRC code of each track */
#define CDIO_CDTEXT_UPC        0x8E

/* Size information of the Block */
#define CDIO_CDTEXT_BLOCKSIZE  0x8F


#if defined(_XBOX) || defined(WIN32)
#pragma pack(1)
#else
PRAGMA_BEGIN_PACKED
#endif

struct CDText_data
{
  uint8_t  type;
  track_t  i_track;
  uint8_t  seq;
#ifdef WORDS_BIGENDIAN
  uint8_t  bDBC:             1;	 /* double byte character */
  uint8_t  block:            3;  /* block number 0..7 */
  uint8_t  characterPosition:4;  /* character position */
#else
  uint8_t  characterPosition:4;  /* character position */
  uint8_t  block            :3;	 /* block number 0..7 */
  uint8_t  bDBC             :1;	 /* double byte character */
#endif
  char     text[CDIO_CDTEXT_MAX_TEXT_DATA];
  uint8_t  crc[2];
} GNUC_PACKED;

#if defined(_XBOX) || defined(WIN32)
#pragma pack()
#else
  PRAGMA_END_PACKED
#endif

typedef struct CDText_data CDText_data_t;

typedef void (*set_cdtext_field_fn_t) (void *user_data, track_t i_track,
                                       track_t i_first_track,
                                       cdtext_field_t field, 
                                       const char *buffer);

/* 
   Internal routine to parse all CD-TEXT data retrieved.
*/       
bool cdtext_data_init(void *user_data, track_t i_first_track, 
                      unsigned char *wdata, 
                      set_cdtext_field_fn_t set_cdtext_field_fn);


#endif /* __CDIO_CDTEXT_PRIVATE_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
