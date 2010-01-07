/*
    $Id: sector.h,v 1.31 2005/01/04 04:33:36 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>

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
   \file sector.h 
   \brief Things related to CD-ROM layout: tracks, sector sizes, MSFs, LBAs.

  A CD-ROM physical sector size is 2048, 2052, 2056, 2324, 2332, 2336,
  2340, or 2352 bytes long.

  Sector types of the standard CD-ROM data formats:

\verbatim 
  format   sector type               user data size (bytes)
  -----------------------------------------------------------------------------
    1     (Red Book)    CD-DA          2352    (CDIO_CD_FRAMESIZE_RAW)
    2     (Yellow Book) Mode1 Form1    2048    (CDIO_CD_FRAMESIZE)
    3     (Yellow Book) Mode1 Form2    2336    (M2RAW_SECTOR_SIZE)
    4     (Green Book)  Mode2 Form1    2048    (CDIO_CD_FRAMESIZE)
    5     (Green Book)  Mode2 Form2    2328    (2324+4 spare bytes)
 
 
        The layout of the standard CD-ROM data formats:
  -----------------------------------------------------------------------------
  - audio (red):                  | audio_sample_bytes |
                                  |        2352        |
 
  - data (yellow, mode1):         | sync - head - data - EDC - zero - ECC |
                                  |  12  -   4  - 2048 -  4  -   8  - 276 |
 
  - data (yellow, mode2):         | sync - head - data |
                                 |  12  -   4  - 2336 |
 
  - XA data (green, mode2 form1): | sync - head - sub - data - EDC - ECC |
                                  |  12  -   4  -  8  - 2048 -  4  - 276 |
 
  - XA data (green, mode2 form2): | sync - head - sub - data - Spare |
                                  |  12  -   4  -  8  - 2324 -  4    |
\endverbatim
 

*/

#ifndef _CDIO_SECTOR_H_
#define _CDIO_SECTOR_H_

#ifdef __cplusplus
    extern "C" {
#endif

#include <cdio/types.h>

/*! track modes (Table 350) 
   reference: MMC-3 draft revsion - 10g
*/
typedef enum {
	AUDIO,				/**< 2352 byte block length */
	MODE1,				/**< 2048 byte block length */
	MODE1_RAW,			/**< 2352 byte block length */
	MODE2,				/**< 2336 byte block length */
	MODE2_FORM1,			/**< 2048 byte block length */
	MODE2_FORM2,			/**< 2324 byte block length */
	MODE2_FORM_MIX,			/**< 2336 byte block length */
	MODE2_RAW			/**< 2352 byte block length */
} trackmode_t;

/*! disc modes. The first combined from MMC-3 5.29.2.8 (Send CUESHEET)
  and GNU/Linux /usr/include/linux/cdrom.h and we've added DVD.
 */
typedef enum {
	CDIO_DISC_MODE_CD_DA,		/**< CD-DA */
	CDIO_DISC_MODE_CD_DATA,	        /**< CD-ROM form 1 */
	CDIO_DISC_MODE_CD_XA,	        /**< CD-ROM XA form2 */
	CDIO_DISC_MODE_CD_MIXED,	/**< Some combo of above. */
        CDIO_DISC_MODE_DVD_ROM,         /**< DVD ROM (e.g. movies) */
        CDIO_DISC_MODE_DVD_RAM,         /**< DVD-RAM */
        CDIO_DISC_MODE_DVD_R,           /**< DVD-R */
        CDIO_DISC_MODE_DVD_RW,          /**< DVD-RW */
        CDIO_DISC_MODE_DVD_PR,          /**< DVD+R */
        CDIO_DISC_MODE_DVD_PRW,         /**< DVD+RW */
        CDIO_DISC_MODE_DVD_OTHER,       /**< Unknown/unclassified DVD type */
        CDIO_DISC_MODE_NO_INFO,
        CDIO_DISC_MODE_ERROR,
	CDIO_DISC_MODE_CD_I	        /**< CD-i. */
} discmode_t;

/*! Information that can be obtained through a Read Subchannel
    command.
 */
#define CDIO_SUBCHANNEL_SUBQ_DATA		0
#define CDIO_SUBCHANNEL_CURRENT_POSITION	1
#define CDIO_SUBCHANNEL_MEDIA_CATALOG	        2
#define CDIO_SUBCHANNEL_TRACK_ISRC		3

/*! track flags
 * Q Sub-channel Control Field (4.2.3.3)
 */
typedef enum {
	NONE = 			0x00,	/* no flags set */
	PRE_EMPHASIS =		0x01,	/* audio track recorded with pre-emphasis */
	COPY_PERMITTED =	0x02,	/* digital copy permitted */
	DATA =			0x04,	/* data track */
	FOUR_CHANNEL_AUDIO =	0x08,	/* 4 audio channels */
	SCMS =			0x10	/* SCMS (5.29.2.7) */
} flag_t;

#define CDIO_PREGAP_SECTORS  150
#define CDIO_POSTGAP_SECTORS 150

/* 
   Some generally useful CD-ROM information -- mostly based on the above.
   This is from linux.h - not to slight other OS's. This was the first
   place I came across such useful stuff.
*/
#define CDIO_CD_MINS              74   /**< max. minutes per CD, not really
                                         a limit */
#define CDIO_CD_SECS_PER_MIN      60   /**< seconds per minute */
#define CDIO_CD_FRAMES_PER_SEC    75   /**< frames per second */
#define CDIO_CD_SYNC_SIZE         12   /**< 12 sync bytes per raw data frame */
#define CDIO_CD_CHUNK_SIZE        24   /**< lowest-level "data bytes piece" */
#define CDIO_CD_NUM_OF_CHUNKS     98   /**< chunks per frame */
#define CDIO_CD_FRAMESIZE_SUB     96   /**< subchannel data "frame" size */
#define CDIO_CD_HEADER_SIZE        4   /**< header (address) bytes per raw
                                          data frame */
#define CDIO_CD_SUBHEADER_SIZE     8   /**< subheader bytes per raw XA data 
                                            frame */
#define CDIO_CD_EDC_SIZE           4   /**< bytes EDC per most raw data
                                          frame types */
#define CDIO_CD_M1F1_ZERO_SIZE     8   /**< bytes zero per yellow book mode
                                          1 frame */
#define CDIO_CD_ECC_SIZE         276   /**< bytes ECC per most raw data frame 
                                          types */
#define CDIO_CD_FRAMESIZE       2048   /**< bytes per frame, "cooked" mode */
#define CDIO_CD_FRAMESIZE_RAW   2352   /**< bytes per frame, "raw" mode */
#define CDIO_CD_FRAMESIZE_RAWER 2646   /**< The maximum possible returned 
                                          bytes */ 
#define CDIO_CD_FRAMESIZE_RAW1 (CDIO_CD_CD_FRAMESIZE_RAW-CDIO_CD_SYNC_SIZE) /*2340*/
#define CDIO_CD_FRAMESIZE_RAW0 (CDIO_CD_FRAMESIZE_RAW-CDIO_CD_SYNC_SIZE-CDIO_CD_HEADER_SIZE) /*2336*/

/*! "before data" part of raw XA (green, mode2) frame */
#define CDIO_CD_XA_HEADER (CDIO_CD_HEADER_SIZE+CDIO_CD_SUBHEADER_SIZE) 

/*! "after data" part of raw XA (green, mode2 form1) frame */
#define CDIO_CD_XA_TAIL   (CDIO_CD_EDC_SIZE+CDIO_CD_ECC_SIZE) 

/*! "before data" sync bytes + header of XA (green, mode2) frame */
#define CDIO_CD_XA_SYNC_HEADER   (CDIO_CD_SYNC_SIZE+CDIO_CD_XA_HEADER) 

/*! CD-ROM address types (GNU/Linux e.g. cdrom_tocentry.cdte_format) */
#define	CDIO_CDROM_LBA 0x01 /**< "logical block": first frame is #0 */
#define	CDIO_CDROM_MSF 0x02 /**< "minute-second-frame": binary, not
                               BCD here! */

/*! CD-ROM track format types (GNU/Linux cdte_ctrl) */
#define	CDIO_CDROM_DATA_TRACK	0x04
#define	CDIO_CDROM_CDI_TRACK	0x10
#define	CDIO_CDROM_XA_TRACK	0x20

#define M2F2_SECTOR_SIZE    2324
#define M2SUB_SECTOR_SIZE   2332
#define M2RAW_SECTOR_SIZE   2336

/*! Largest CD session number */
#define CDIO_CD_MAX_SESSIONS    99 
/*! Smallest CD session number */
#define CDIO_CD_MIN_SESSION_NO   1

/*! Largest LSN in a CD */
#define CDIO_CD_MAX_LSN   450150
/*! Smallest LSN in a CD */
#define CDIO_CD_MIN_LSN  -450150


#define CDIO_CD_FRAMES_PER_MIN \
   (CDIO_CD_FRAMES_PER_SEC*CDIO_CD_SECS_PER_MIN)

#define CDIO_CD_74MIN_SECTORS (UINT32_C(74)*CDIO_CD_FRAMES_PER_MIN)
#define CDIO_CD_80MIN_SECTORS (UINT32_C(80)*CDIO_CD_FRAMES_PER_MIN)
#define CDIO_CD_90MIN_SECTORS (UINT32_C(90)*CDIO_CD_FRAMES_PER_MIN)

#define CDIO_CD_MAX_SECTORS  \
   (UINT32_C(100)*CDIO_CD_FRAMES_PER_MIN-CDIO_PREGAP_SECTORS)

#define msf_t_SIZEOF 3

/*! 
  Convert an LBA into a string representation of the MSF.
  \warning cdio_lba_to_msf_str returns new allocated string */
char *cdio_lba_to_msf_str (lba_t lba);

/*! 
  Convert an MSF into a string representation of the MSF.
  \warning cdio_msf_to_msf_str returns new allocated string */
char *cdio_msf_to_str (const msf_t *msf);

/*! 
  Convert an LBA into the corresponding LSN.
*/
lba_t cdio_lba_to_lsn (lba_t lba);

/*! 
  Convert an LBA into the corresponding MSF.
*/
void  cdio_lba_to_msf(lba_t lba, msf_t *msf);

/*! 
  Convert an LSN into the corresponding LBA.
  CDIO_INVALID_LBA is returned if there is an error.
*/
lba_t cdio_lsn_to_lba (lsn_t lsn);

/*! 
  Convert an LSN into the corresponding MSF.
*/
void  cdio_lsn_to_msf (lsn_t lsn, msf_t *msf);

/*! 
  Convert a MSF into the corresponding LBA.
  CDIO_INVALID_LBA is returned if there is an error.
*/
lba_t cdio_msf_to_lba (const msf_t *msf);

/*! 
  Convert a MSF into the corresponding LSN.
  CDIO_INVALID_LSN is returned if there is an error.
*/
lsn_t cdio_msf_to_lsn (const msf_t *msf);

/*!  
  Convert a MSF - broken out as 3 integer components into the
  corresponding LBA.  
  CDIO_INVALID_LBA is returned if there is an error.
*/
lba_t cdio_msf3_to_lba (unsigned int minutes, unsigned int seconds, 
                        unsigned int frames);
      
/*! 
  Convert a string of the form MM:SS:FF into the corresponding LBA.
  CDIO_INVALID_LBA is returned if there is an error.
*/
lba_t cdio_mmssff_to_lba (const char *psz_mmssff);
      
/*! 
  Return true if discmode is some sort of CD.
*/
bool cdio_is_discmode_cdrom (discmode_t discmode);
      
/*! 
  Return true if discmode is some sort of DVD.
*/
bool cdio_is_discmode_dvd (discmode_t discmode);
      

#ifdef __cplusplus
    }
#endif

static inline bool discmode_is_cd(discmode_t discmode) 
{
  switch (discmode) {
  case CDIO_DISC_MODE_CD_DA:
  case CDIO_DISC_MODE_CD_DATA:
  case CDIO_DISC_MODE_CD_XA:
  case CDIO_DISC_MODE_CD_MIXED:
  case CDIO_DISC_MODE_CD_I:
    return true;
  default: 
    return false;
  }
}

static inline bool discmode_is_dvd(discmode_t discmode) 
{
  switch (discmode) {
  case CDIO_DISC_MODE_DVD_ROM:
  case CDIO_DISC_MODE_DVD_RAM:
  case CDIO_DISC_MODE_DVD_R:
  case CDIO_DISC_MODE_DVD_RW:
  case CDIO_DISC_MODE_DVD_PR:
  case CDIO_DISC_MODE_DVD_PRW:
  case CDIO_DISC_MODE_DVD_OTHER:
    return true;
  default: 
    return false;
  }
}


#endif /* _CDIO_SECTOR_H_ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
