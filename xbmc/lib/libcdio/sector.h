/*
    $Id$

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>

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
/* 
   Things related to CDROM layout. Sector sizes, MSFs, LBAs, 
*/

#ifndef _CDIO_SECTOR_H_
#define _CDIO_SECTOR_H_

#ifdef __cplusplus
    extern "C" {
#endif

#include "types.h"

#define CDIO_PREGAP_SECTORS 150
#define CDIO_POSTGAP_SECTORS 150

/*
 * A CD-ROM physical sector size is 2048, 2052, 2056, 2324, 2332, 2336, 
 * 2340, or 2352 bytes long.  

*         Sector types of the standard CD-ROM data formats:
 *
 * format   sector type               user data size (bytes)
 * -----------------------------------------------------------------------------
 *   1     (Red Book)    CD-DA          2352    (CDIO_CD_FRAMESIZE_RAW)
 *   2     (Yellow Book) Mode1 Form1    2048    (M1F1_SECTOR_SIZE)
 *   3     (Yellow Book) Mode1 Form2    2336    (M2RAW_SECTOR_SIZE)
 *   4     (Green Book)  Mode2 Form1    2048    (M2F1_SECTOR_SIZE)
 *   5     (Green Book)  Mode2 Form2    2328    (2324+4 spare bytes)
 *
 *
 *       The layout of the standard CD-ROM data formats:
 * -----------------------------------------------------------------------------
 * - audio (red):                  | audio_sample_bytes |
 *                                 |        2352        |
 *
 * - data (yellow, mode1):         | sync - head - data - EDC - zero - ECC |
 *                                 |  12  -   4  - 2048 -  4  -   8  - 276 |
 *
 * - data (yellow, mode2):         | sync - head - data |
 *                                 |  12  -   4  - 2336 |
 *
 * - XA data (green, mode2 form1): | sync - head - sub - data - EDC - ECC |
 *                                 |  12  -   4  -  8  - 2048 -  4  - 276 |
 *
 * - XA data (green, mode2 form2): | sync - head - sub - data - Spare |
 *                                 |  12  -   4  -  8  - 2324 -  4    |
 *
 */

/* 
   Some generally useful CD-ROM information -- mostly based on the above.
   This is from linux.h - not to slight other OS's. This was the first
   place I came across such useful stuff.
*/
#define CDIO_CD_MINS           74 /* max. minutes per CD, not really a limit */
#define CDIO_CD_SECS_PER_MIN   60 /* seconds per minute */
#define CDIO_CD_FRAMES_PER_SEC 75 /* frames per second */
#define CDIO_CD_SYNC_SIZE      12 /* 12 sync bytes per raw data frame */
#define CDIO_CD_CHUNK_SIZE     24 /* lowest-level "data bytes piece" */
#define CDIO_CD_NUM_OF_CHUNKS  98 /* chunks per frame */
#define CDIO_CD_FRAMESIZE_SUB  96 /* subchannel data "frame" size */
#define CDIO_CD_HEADER_SIZE     4 /* header (address) bytes per raw data 
                                     frame */
#define CDIO_CD_SUBHEADER_SIZE  8 /* subheader bytes per raw XA data frame */
#define CDIO_CD_EDC_SIZE        4 /* bytes EDC per most raw data frame types */
#define CDIO_CD_M1F1_ZERO_SIZE  8 /* bytes zero per yellow book mode 1 frame */
#define CDIO_CD_ECC_SIZE      276 /* bytes ECC per most raw data frame types */
#define CDIO_CD_FRAMESIZE    2048 /* bytes per frame, "cooked" mode */
#define CDIO_CD_FRAMESIZE_RAW 2352/* bytes per frame, "raw" mode */
#define CDIO_CD_FRAMESIZE_RAWER 2646 /* The maximum possible returned bytes */ 
#define CDIO_CD_FRAMESIZE_RAW1 (CD_FRAMESIZE_RAW-CD_SYNC_SIZE) /*2340*/
#define CDIO_CD_FRAMESIZE_RAW0 (CD_FRAMESIZE_RAW-CD_SYNC_SIZE-CD_HEAD_SIZE) /*2336*/

/* "before data" part of raw XA (green, mode2) frame */
#define CDIO_CD_XA_HEADER (CDIO_CD_HEADER_SIZE+CDIO_CD_SUBHEADER_SIZE) 

/* "after data" part of raw XA (green, mode2 form1) frame */
#define CDIO_CD_XA_TAIL   (CDIO_CD_EDC_SIZE+CDIO_CD_ECC_SIZE) 

/* "before data" sync bytes + header of XA (green, mode2) frame */
#define CDIO_CD_XA_SYNC_HEADER   (CDIO_CD_SYNC_SIZE+CDIO_CD_XA_HEADER) 

/* CD-ROM address types (Linux cdrom_tocentry.cdte_format) */
#define	CDIO_CDROM_LBA 0x01 /* "logical block": first frame is #0 */
#define	CDIO_CDROM_MSF 0x02 /* "minute-second-frame": binary, not bcd here! */

#define	CDIO_CDROM_DATA_TRACK	0x04

/* The leadout track is always 0xAA, regardless of # of tracks on disc */
#define	CDIO_CDROM_LEADOUT_TRACK 0xAA

#define M2F2_SECTOR_SIZE    2324
#define M2SUB_SECTOR_SIZE   2332
#define M2RAW_SECTOR_SIZE   2336

#define CDIO_CD_MAX_TRACKS 99 

#define CDIO_CD_FRAMES_PER_MIN \
   (CDIO_CD_FRAMES_PER_SEC*CDIO_CD_SECS_PER_MIN)

#define CDIO_CD_74MIN_SECTORS (UINT32_C(74)*CDIO_CD_FRAMES_PER_MIN)
#define CDIO_CD_80MIN_SECTORS (UINT32_C(80)*CDIO_CD_FRAMES_PER_MIN)
#define CDIO_CD_90MIN_SECTORS (UINT32_C(90)*CDIO_CD_FRAMES_PER_MIN)

#define CDIO_CD_MAX_SECTORS  \
   (UINT32_C(100)*CDIO_CD_FRAMES_PER_MIN-CDIO_PREGAP_SECTORS)

#define msf_t_SIZEOF 3

/* warning, returns new allocated string */
char *cdio_lba_to_msf_str (lba_t lba);

lba_t cdio_lba_to_lsn (lba_t lba);

void  cdio_lba_to_msf(lba_t lba, msf_t *msf);

lba_t cdio_lsn_to_lba (lsn_t lsn);

void  cdio_lsn_to_msf (lsn_t lsn, msf_t *msf);

lba_t
cdio_msf_to_lba (const msf_t *msf);

lsn_t
cdio_msf_to_lsn (const msf_t *msf);

#ifdef __cplusplus
    }
#endif

#endif /* _CDIO_SECTOR_H_ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
