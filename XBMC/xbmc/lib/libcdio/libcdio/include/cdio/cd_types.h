/*
    $Id: cd_types.h,v 1.17 2006/04/12 09:30:14 rocky Exp $

    Copyright (C) 2003, 2006 Rocky Bernstein <rocky@cpan.org>
    Copyright (C) 1996,1997,1998  Gerd Knorr <kraxel@bytesex.org>
         and       Heiko Eiﬂfeldt <heiko@hexco.de>

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

/** \file cd_types.h 
 *  \brief Header for routines which automatically determine the Compact Disc
 *  format and possibly filesystem on the CD.
 *         
 */

#ifndef __CDIO_CD_TYPES_H__
#define __CDIO_CD_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Filesystem types we understand. The highest-numbered fs type should
 *  be less than CDIO_FS_MASK defined below.
 */
  typedef enum {
    CDIO_FS_AUDIO                = 1, /**< audio only - not really a 
                                         filesystem */
    CDIO_FS_HIGH_SIERRA	         = 2, /**< High-Sierra Filesystem */
    CDIO_FS_ISO_9660	         = 3, /**< ISO 9660 filesystem */
    CDIO_FS_INTERACTIVE	         = 4,
    CDIO_FS_HFS		         = 5, /**< file system used on the Macintosh 
                                         system in MacOS 6 through MacOS 9
                                         and deprecated in OSX. */
    CDIO_FS_UFS		         = 6, /**< Generic Unix file system derived
                                         from the Berkeley fast file 
                                         system. */
    
    /**<
     * EXT2 was the GNU/Linux native filesystem for early kernels. Newer
     * GNU/Linux OS's may use EXT3 which is EXT2 with a journal. 
     */
    CDIO_FS_EXT2		 = 7,

    CDIO_FS_ISO_HFS              = 8,  /**< both HFS & ISO-9660 filesystem */
    CDIO_FS_ISO_9660_INTERACTIVE = 9,  /**< both CD-RTOS and ISO filesystem */


    /**<
     * The 3DO is, technically, a set of specifications created by the 3DO
     * company.  These specs are for making a 3DO Interactive Multiplayer
     * which uses a CD-player. Panasonic in the early 90's was the first
     * company to manufacture and market a 3DO player. 
     */
    CDIO_FS_3DO		        = 10,


    /**<
       Microsoft X-BOX CD.
    */
    CDIO_FS_XISO 		= 11,
    CDIO_FS_UDFX 		= 12,
    CDIO_FS_UDF 		= 13,
    CDIO_FS_ISO_UDF             = 14
  } cdio_fs_t;


/** 
 * Macro to extract just the FS type portion defined above 
*/
#define CDIO_FSTYPE(fs) (fs & CDIO_FS_MASK)

/**
 *  Bit masks for the classes of CD-images. These are generally
 *  higher-level than the fs-type information above and may be determined
 *  based of the fs type information. This 
 */
  typedef enum {
    CDIO_FS_MASK	      =	  0x000f, /**< Note: this should be 2**n-1 and
                                               and greater than the highest 
                                               CDIO_FS number above */
    CDIO_FS_ANAL_XA           =	  0x00010, /**< eXtended Architecture format */
    CDIO_FS_ANAL_MULTISESSION =   0x00020, /**< CD has multisesion */
    CDIO_FS_ANAL_PHOTO_CD     =	  0x00040, /**< Is a Kodak Photo CD */
    CDIO_FS_ANAL_HIDDEN_TRACK =   0x00080, /**< Hidden track at the 
                                               beginning of the CD */
    CDIO_FS_ANAL_CDTV         =	  0x00100,
    CDIO_FS_ANAL_BOOTABLE     =   0x00200, /**< CD is bootable */
    CDIO_FS_ANAL_VIDEOCD      =   0x00400, /**< VCD 1.1 */
    CDIO_FS_ANAL_ROCKRIDGE    =   0x00800, /**< Has Rock Ridge Extensions to
                                               ISO 9660, */
    CDIO_FS_ANAL_JOLIET       =   0x01000, /**< Microsoft Joliet extensions 
                                                to ISO 9660, */
    CDIO_FS_ANAL_SVCD         =   0x02000, /**< Super VCD or Choiji Video CD */
    CDIO_FS_ANAL_CVD          =   0x04000, /**< Choiji Video CD */
    CDIO_FS_ANAL_XISO         =   0x08000, /**< XBOX CD */
    CDIO_FS_ANAL_ISO9660_ANY  =   0x10000, /**< Any sort fo ISO9660 FS */
    CDIO_FS_ANAL_VCD_ANY      =   (CDIO_FS_ANAL_VIDEOCD|CDIO_FS_ANAL_SVCD|
                                   CDIO_FS_ANAL_CVD),
    CDIO_FS_MATCH_ALL         =  ~CDIO_FS_MASK /**< bitmask which can
                                                 be used by
                                                 cdio_get_devices to
                                                 specify matching any
                                                 sort of CD. */
  } cdio_fs_cap_t;
    

#define CDIO_FS_UNKNOWN	            CDIO_FS_MASK

/**
 * 
 */
#define CDIO_FS_MATCH_ALL            (cdio_fs_anal_t) (~CDIO_FS_MASK)


/*!
  \brief The type used to return analysis information from
  cdio_guess_cd_type. 

  These fields make sense only for when an ISO-9660 filesystem is used.
 */
typedef struct 
{
  unsigned int  joliet_level;  /**< If has Joliet extensions, this is the
                                  associated level number (i.e. 1, 2, or 3). */
  char          iso_label[33]; /**< This is 32 + 1 for null byte at the end in 
				    formatting the string */
  unsigned int  isofs_size;
  uint8_t       UDFVerMinor;   /**< For UDF filesystems only */
  uint8_t       UDFVerMajor;   /**< For UDF filesystems only */
} cdio_iso_analysis_t;

/**
 *  Try to determine what kind of CD-image and/or filesystem we
 *  have at track track_num. Return information about the CD image
 *  is returned in iso_analysis and the return value.
 */
cdio_fs_anal_t cdio_guess_cd_type(const CdIo_t *cdio, int start_session, 
				  track_t track_num, 
				  /*out*/ cdio_iso_analysis_t *iso_analysis);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** The below variables are trickery to force the above enum symbol
    values to be recorded in debug symbol tables. They are used to
    allow one to refer to the enumeration value names in the typedefs
    above in a debugger and debugger expressions.
*/
extern cdio_fs_cap_t debug_cdio_fs_cap;
extern cdio_fs_t     debug_cdio_fs;

#endif /* __CDIO_CD_TYPES_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
