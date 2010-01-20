/*
    $Id: mmc.h,v 1.30 2007/11/21 03:01:58 rocky Exp $

    Copyright (C) 2003, 2004, 2005, 2006, 2007 Rocky Bernstein <rocky@gnu.org>

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

/**
 *  \file mmc.h 
 * 
 *  \brief Common definitions for MMC (Multimedia Commands). Applications
 *  include this for direct MMC access.
*/

#ifndef __CDIO_MMC_H__
#define __CDIO_MMC_H__

#include "cdio.h"
#include "types.h"
#include "dvd.h"
#include "audio.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /** Set this to the maximum value in milliseconds that we will
      wait on an MMC command.  */
  extern uint32_t mmc_timeout_ms; 

  /** The default timeout (non-read) is 6 seconds. */
#define MMC_TIMEOUT_DEFAULT 6000

  /** Set this to the maximum value in milliseconds that we will
      wait on an MMC read command.  */
  extern uint32_t mmc_read_timeout_ms;

  /** The default read timeout is 3 minutes. */
#define MMC_READ_TIMEOUT_DEFAULT 3*60*1000


  /** \brief The opcode-portion (generic packet commands) of an MMC command.
    
  In general, those opcodes that end in 6 take a 6-byte command
  descriptor, those that end in 10 take a 10-byte
  descriptor and those that in in 12 take a 12-byte descriptor. 
  
  (Not that you need to know that, but it seems to be a
  big deal in the MMC specification.)
  
  */
  typedef enum {
  CDIO_MMC_GPCMD_INQUIRY 	        = 0x12, /**< Request drive 
						   information. */
  CDIO_MMC_GPCMD_MODE_SELECT_6	        = 0x15, /**< Select medium 
						   (6 bytes). */
  CDIO_MMC_GPCMD_MODE_SENSE_6 	        = 0x1a, /**< Get medium or device
						 information. Should be issued
						 before MODE SELECT to get
						 mode support or save current
						 settings. (6 bytes). */
  CDIO_MMC_GPCMD_START_STOP             = 0x1b, /**< Enable/disable Disc
						     operations. (6 bytes). */
  CDIO_MMC_GPCMD_ALLOW_MEDIUM_REMOVAL   = 0x1e, /**< Enable/disable Disc 
						   removal. (6 bytes). */

  /** Group 2 Commands (CDB's here are 10-bytes)   
   */
  CDIO_MMC_GPCMD_READ_10	        = 0x28, /**< Read data from drive 
						   (10 bytes). */
  CDIO_MMC_GPCMD_READ_SUBCHANNEL	= 0x42, /**< Read Sub-Channel data.
						   (10 bytes). */
  CDIO_MMC_GPCMD_READ_TOC               = 0x43, /**< READ TOC/PMA/ATIP. 
						   (10 bytes). */
  CDIO_MMC_GPCMD_READ_HEADER            = 0x44,
  CDIO_MMC_GPCMD_PLAY_AUDIO_10	        = 0x45, /**< Begin audio playing at
						   current position
						   (10 bytes). */
  CDIO_MMC_GPCMD_GET_CONFIGURATION      = 0x46, /**< Get drive Capabilities 
						   (10 bytes) */
  CDIO_MMC_GPCMD_PLAY_AUDIO_MSF	        = 0x47, /**< Begin audio playing at
						   specified MSF (10
						   bytes). */
  CDIO_MMC_GPCMD_PLAY_AUDIO_TI          = 0x48,
  CDIO_MMC_GPCMD_PLAY_TRACK_REL_10      = 0x49, /**< Play audio at the track
						   relative LBA. (10 bytes).
						   Doesn't seem to be part
						   of MMC standards but is
						   handled by Plextor drives.
						*/
						   
  CDIO_MMC_GPCMD_GET_EVENT_STATUS       = 0x4a, /**< Report events and 
						   Status. */
  CDIO_MMC_GPCMD_PAUSE_RESUME           = 0x4b, /**< Stop or restart audio 
						   playback. (10 bytes). 
						   Used with a PLAY command. */

  CDIO_MMC_GPCMD_READ_DISC_INFO	        = 0x51, /**< Get CD information.
						   (10 bytes). */
  CDIO_MMC_GPCMD_MODE_SELECT_10	        = 0x55, /**< Select medium 
						   (10-bytes). */
  CDIO_MMC_GPCMD_MODE_SENSE_10	        = 0x5a, /**< Get medium or device
						 information. Should be issued
						 before MODE SELECT to get
						 mode support or save current
						 settings. (6 bytes). */

  /** Group 5 Commands (CDB's here are 12-bytes) 
   */
  CDIO_MMC_GPCMD_PLAY_AUDIO_12	        = 0xa5, /**< Begin audio playing at
						   current position
						 (12 bytes) */
  CDIO_MMC_GPCMD_LOAD_UNLOAD	        = 0xa6, /**< Load/unload a Disc
						 (12 bytes) */
  CDIO_MMC_GPCMD_READ_12	        = 0xa8, /**< Read data from drive 
						   (12 bytes). */
  CDIO_MMC_GPCMD_PLAY_TRACK_REL_12      = 0xa9, /**< Play audio at the track
						   relative LBA. (12 bytes).
						   Doesn't seem to be part
						   of MMC standards but is
						   handled by Plextor drives.
						*/
  CDIO_MMC_GPCMD_READ_DVD_STRUCTURE     = 0xad, /**< Get DVD structure info
						   from media (12 bytes). */
  CDIO_MMC_GPCMD_READ_MSF	        = 0xb9, /**< Read almost any field 
						   of a CD sector at specified
						   MSF. (12 bytes). */
  CDIO_MMC_GPCMD_SET_SPEED	        = 0xbb, /**< Set drive speed 
						   (12 bytes). This is listed 
						   as optional in ATAPI 2.6, 
						   but is (curiously)
						   missing from Mt. Fuji, 
						   Table 57. It is mentioned
						   in Mt. Fuji Table 377 as an 
						   MMC command for SCSI
						   devices though...  Most
						   ATAPI drives support it. */
  CDIO_MMC_GPCMD_READ_CD	        = 0xbe, /**< Read almost any field 
						   of a CD sector at current
						   location. (12 bytes). */
  /** Vendor-unique Commands  
   */
  CDIO_MMC_GPCMD_CD_PLAYBACK_STATUS  = 0xc4 /**< SONY unique  = command */,
  CDIO_MMC_GPCMD_PLAYBACK_CONTROL    = 0xc9 /**< SONY unique  = command */,
  CDIO_MMC_GPCMD_READ_CDDA	     = 0xd8 /**< Vendor unique  = command */,
  CDIO_MMC_GPCMD_READ_CDXA	     = 0xdb /**< Vendor unique  = command */,
  CDIO_MMC_GPCMD_READ_ALL_SUBCODES   = 0xdf /**< Vendor unique  = command */
  } cdio_mmc_gpcmd_t;


  /** Read Subchannel states */
  typedef enum {
    CDIO_MMC_READ_SUB_ST_INVALID   = 0x00, /**< audio status not supported */
    CDIO_MMC_READ_SUB_ST_PLAY      = 0x11, /**< audio play operation in 
                                              progress */
    CDIO_MMC_READ_SUB_ST_PAUSED    = 0x12, /**< audio play operation paused */
    CDIO_MMC_READ_SUB_ST_COMPLETED = 0x13, /**< audio play successfully 
                                              completed */
    CDIO_MMC_READ_SUB_ST_ERROR     = 0x14, /**< audio play stopped due to 
                                              error */
    CDIO_MMC_READ_SUB_ST_NO_STATUS = 0x15, /**< no current audio status to
                                              return */
  } cdio_mmc_read_sub_state_t;

  /** Level values that can go into READ_CD */
  typedef enum {
    CDIO_MMC_READ_TYPE_ANY  =  0,  /**< All types */
    CDIO_MMC_READ_TYPE_CDDA =  1,  /**< Only CD-DA sectors */
    CDIO_MMC_READ_TYPE_MODE1 = 2,  /**< mode1 sectors (user data = 2048) */
    CDIO_MMC_READ_TYPE_MODE2 = 3,  /**< mode2 sectors form1 or form2 */
    CDIO_MMC_READ_TYPE_M2F1 =  4,  /**< mode2 sectors form1 */
    CDIO_MMC_READ_TYPE_M2F2 =  5   /**< mode2 sectors form2 */
  } cdio_mmc_read_cd_type_t;

  /** Format values for READ_TOC */
  typedef enum {
    CDIO_MMC_READTOC_FMT_TOC     =  0,
    CDIO_MMC_READTOC_FMT_SESSION =  1,  
    CDIO_MMC_READTOC_FMT_FULTOC  =  2,
    CDIO_MMC_READTOC_FMT_PMA     =  3,  /**< Q subcode data */
    CDIO_MMC_READTOC_FMT_ATIP    =  4,  /**< includes media type */
    CDIO_MMC_READTOC_FMT_CDTEXT  =  5   /**< CD-TEXT info  */
  } cdio_mmc_readtoc_t;
    
/** Page codes for MODE SENSE and MODE SET. */
  typedef enum {
    CDIO_MMC_R_W_ERROR_PAGE	= 0x01,
    CDIO_MMC_WRITE_PARMS_PAGE	= 0x05,
    CDIO_MMC_CDR_PARMS_PAGE	= 0x0d,
    CDIO_MMC_AUDIO_CTL_PAGE	= 0x0e,
    CDIO_MMC_POWER_PAGE		= 0x1a,
    CDIO_MMC_FAULT_FAIL_PAGE	= 0x1c,
    CDIO_MMC_TO_PROTECT_PAGE	= 0x1d,
    CDIO_MMC_CAPABILITIES_PAGE	= 0x2a,
    CDIO_MMC_ALL_PAGES		= 0x3f,
  } cdio_mmc_mode_page_t;
    

#if defined(_XBOX) || defined(WIN32)
  #pragma pack(1)
#else
  PRAGMA_BEGIN_PACKED
#endif
  struct mmc_audio_volume_entry_s 
  {
    uint8_t  selection; /* Only the lower 4 bits are used. */
    uint8_t  volume;
  } GNUC_PACKED;

  typedef struct mmc_audio_volume_entry_s mmc_audio_volume_entry_t;
  
  /** This struct is used by cdio_audio_get_volume and cdio_audio_set_volume */
  struct mmc_audio_volume_s
  {
    mmc_audio_volume_entry_t port[4];
  } GNUC_PACKED;

  typedef struct mmc_audio_volume_s mmc_audio_volume_t;
  
#if defined(_XBOX) || defined(WIN32)
  #pragma pack()
#else
  PRAGMA_END_PACKED
#endif
  

/** Return type codes for GET_CONFIGURATION. */
typedef enum {
  CDIO_MMC_GET_CONF_ALL_FEATURES     = 0,  /**< all features without regard
                                              to currency. */
  CDIO_MMC_GET_CONF_CURRENT_FEATURES = 1,  /**< features which are currently
                                              in effect (e.g. based on
                                              medium inserted). */
  CDIO_MMC_GET_CONF_NAMED_FEATURE    = 2   /**< just the feature named in
                                              the GET_CONFIGURATION cdb. */
} cdio_mmc_get_conf_t;
  

/** FEATURE codes used in GET CONFIGURATION. */

typedef enum {
  CDIO_MMC_FEATURE_PROFILE_LIST     = 0x000, /**< Profile List Feature */
  CDIO_MMC_FEATURE_CORE             = 0x001, 
  CDIO_MMC_FEATURE_MORPHING         = 0x002, /**< Report/prevent operational
                                                  changes  */
  CDIO_MMC_FEATURE_REMOVABLE_MEDIUM = 0x003, /**< Removable Medium Feature */
  CDIO_MMC_FEATURE_WRITE_PROTECT    = 0x004, /**< Write Protect Feature */
  CDIO_MMC_FEATURE_RANDOM_READABLE  = 0x010, /**< Random Readable Feature */
  CDIO_MMC_FEATURE_MULTI_READ       = 0x01D, /**< Multi-Read Feature */
  CDIO_MMC_FEATURE_CD_READ          = 0x01E, /**< CD Read Feature */
  CDIO_MMC_FEATURE_DVD_READ         = 0x01F, /**< DVD Read Feature */
  CDIO_MMC_FEATURE_RANDOM_WRITABLE  = 0x020, /**< Random Writable Feature */
  CDIO_MMC_FEATURE_INCR_WRITE       = 0x021, /**< Incremental Streaming 
						Writable Feature */
  CDIO_MMC_FEATURE_SECTOR_ERASE     = 0x022, /**< Sector Erasable Feature */
  CDIO_MMC_FEATURE_FORMATABLE       = 0x023, /**< Formattable Feature */
  CDIO_MMC_FEATURE_DEFECT_MGMT      = 0x024, /**< Management Ability of the 
						Logical Unit/media system to
						provide an apparently 
						defect-free space.*/
  CDIO_MMC_FEATURE_WRITE_ONCE       = 0x025, /**< Write Once
						   Feature */
  CDIO_MMC_FEATURE_RESTRICT_OVERW   = 0x026, /**< Restricted Overwrite
						Feature */
  CDIO_MMC_FEATURE_CD_RW_CAV        = 0x027, /**< CD-RW CAV Write Feature */
  CDIO_MMC_FEATURE_MRW              = 0x028, /**< MRW Feature */
  CDIO_MMC_FEATURE_ENHANCED_DEFECT  = 0x029, /**< Enhanced Defect Reporting */
  CDIO_MMC_FEATURE_DVD_PRW          = 0x02A, /**< DVD+RW Feature */
  CDIO_MMC_FEATURE_DVD_PR           = 0x02B, /**< DVD+R Feature */
  CDIO_MMC_FEATURE_RIGID_RES_OVERW  = 0x02C, /**< Rigid Restricted Overwrite */
  CDIO_MMC_FEATURE_CD_TAO           = 0x02D, /**< CD Track at Once */ 
  CDIO_MMC_FEATURE_CD_SAO           = 0x02E, /**< CD Mastering (Session at 
						Once) */
  CDIO_MMC_FEATURE_DVD_R_RW_WRITE   = 0x02F, /**< DVD-R/RW Write */
  CDIO_MMC_FEATURE_CD_RW_MEDIA_WRITE= 0x037, /**< CD-RW Media Write Support */
  CDIO_MMC_FEATURE_DVD_PR_2_LAYER   = 0x03B, /**< DVD+R Double Layer */
  CDIO_MMC_FEATURE_POWER_MGMT       = 0x100, /**< Initiator and device directed
						power management */
  CDIO_MMC_FEATURE_CDDA_EXT_PLAY    = 0x103, /**< Ability to play audio CDs 
						via the Logical Unit's own
						analog output */
  CDIO_MMC_FEATURE_MCODE_UPGRADE    = 0x104, /* Ability for the device to 
						accept  new microcode via
						the interface */
  CDIO_MMC_FEATURE_TIME_OUT         = 0x105, /**< Ability to respond to all
						commands within a specific 
						time */
  CDIO_MMC_FEATURE_DVD_CSS          = 0x106, /**< Ability to perform DVD
						CSS/CPPM authentication and
						RPC */
  CDIO_MMC_FEATURE_RT_STREAMING     = 0x107, /**< Ability to read and write 
						using Initiator requested
						performance parameters   */
  CDIO_MMC_FEATURE_LU_SN            = 0x108, /**< The Logical Unit has a unique
						identifier. */
  CDIO_MMC_FEATURE_FIRMWARE_DATE    = 0x1FF, /**< Firmware creation date 
						report */
} cdio_mmc_feature_t;
				
/** Profile profile codes used in GET_CONFIGURATION - PROFILE LIST. */
typedef enum {
  CDIO_MMC_FEATURE_PROF_NON_REMOVABLE = 0x0001, /**< Re-writable disk, capable
						   of changing behavior */
  CDIO_MMC_FEATURE_PROF_REMOVABLE     = 0x0002, /**< disk Re-writable; with 
						   removable  media */
  CDIO_MMC_FEATURE_PROF_MO_ERASABLE   = 0x0003, /**< Erasable Magneto-Optical
						   disk with sector erase
						   capability */
  CDIO_MMC_FEATURE_PROF_MO_WRITE_ONCE = 0x0004, /**< Write Once Magneto-Optical
						   write once */
  CDIO_MMC_FEATURE_PROF_AS_MO         = 0x0005, /**< Advance Storage
						   Magneto-Optical */
  CDIO_MMC_FEATURE_PROF_CD_ROM        = 0x0008, /**< Read only Compact Disc
						   capable */
  CDIO_MMC_FEATURE_PROF_CD_R          = 0x0009, /**< Write once Compact Disc
						   capable */
  CDIO_MMC_FEATURE_PROF_CD_RW         = 0x000A, /**< CD-RW Re-writable
						   Compact Disc capable */
  CDIO_MMC_FEATURE_PROF_DVD_ROM       = 0x0010, /**< Read only DVD */
  CDIO_MMC_FEATURE_PROF_DVD_R_SEQ     = 0x0011, /**< Re-recordable DVD using
						   Sequential recording */
  CDIO_MMC_FEATURE_PROF_DVD_RAM       = 0x0012, /**< Re-writable DVD */
  CDIO_MMC_FEATURE_PROF_DVD_RW_RO     = 0x0013, /**< Re-recordable DVD using
						   Restricted Overwrite */
  CDIO_MMC_FEATURE_PROF_DVD_RW_SEQ    = 0x0014, /**< Re-recordable DVD using
						   Sequential recording */
  CDIO_MMC_FEATURE_PROF_DVD_PRW       = 0x001A, /**< DVD+RW - DVD ReWritable */
  CDIO_MMC_FEATURE_PROF_DVD_PR        = 0x001B, /**< DVD+R - DVD Recordable */
  CDIO_MMC_FEATURE_PROF_DDCD_ROM      = 0x0020, /**< Read only  DDCD */
  CDIO_MMC_FEATURE_PROF_DDCD_R        = 0x0021, /**< DDCD-R Write only DDCD */
  CDIO_MMC_FEATURE_PROF_DDCD_RW       = 0x0022, /**< Re-Write only DDCD */
  CDIO_MMC_FEATURE_PROF_DVD_PR2       = 0x002B, /**< DVD+R - DVD Recordable 
                                                     double layer */
  CDIO_MMC_FEATURE_PROF_NON_CONFORM   = 0xFFFF, /**< The Logical Unit does not
						   conform to any Profile. */
} cdio_mmc_feature_profile_t;
  
typedef enum {
  CDIO_MMC_FEATURE_INTERFACE_UNSPECIFIED = 0,
  CDIO_MMC_FEATURE_INTERFACE_SCSI        = 1,
  CDIO_MMC_FEATURE_INTERFACE_ATAPI       = 2,
  CDIO_MMC_FEATURE_INTERFACE_IEEE_1394   = 3,
  CDIO_MMC_FEATURE_INTERFACE_IEEE_1394A  = 4,
  CDIO_MMC_FEATURE_INTERFACE_FIBRE_CH    = 5
} cdio_mmc_feature_interface_t;
  

/** The largest Command Descriptor Block (CDB) size.
    The possible sizes are 6, 10, and 12 bytes.
 */
#define MAX_CDB_LEN 12

/** \brief A Command Descriptor Block (CDB) used in sending MMC 
    commands.
 */
typedef struct mmc_cdb_s {
  uint8_t field[MAX_CDB_LEN];
} mmc_cdb_t;
  
  /** \brief Format of header block in data returned from an MMC
    GET_CONFIGURATION command.
  */
  typedef struct mmc_feature_list_header_s {
    unsigned char length_msb;
    unsigned char length_1sb;
    unsigned char length_2sb;
    unsigned char length_lsb;
    unsigned char reserved1;
    unsigned char reserved2;
    unsigned char profile_msb;
    unsigned char profile_lsb;
  } cdio_mmc_feature_list_header_t;

  /** An enumeration indicating whether an MMC command is sending
    data or getting data.
  */
  typedef enum mmc_direction_s {
    SCSI_MMC_DATA_READ,
    SCSI_MMC_DATA_WRITE
  } cdio_mmc_direction_t;
  
  typedef struct mmc_subchannel_s
  {
    uint8_t       reserved;
    uint8_t       audio_status;
    uint16_t      data_length; /**< Really ISO 9660 7.2.2 */
    uint8_t	  format;
    uint8_t	  address:	4;
    uint8_t	  control:	4;
    uint8_t	  track;
    uint8_t	  index;
    uint8_t       abs_addr[4];
    uint8_t       rel_addr[4];
  } cdio_mmc_subchannel_t;
  
#define CDIO_MMC_SET_COMMAND(cdb, command)      \
  cdb[0] = command
  
#define CDIO_MMC_SET_READ_TYPE(cdb, sector_type) \
  cdb[1] = (sector_type << 2)
  
#define CDIO_MMC_GETPOS_LEN16(p, pos)           \
  (p[pos]<<8) + p[pos+1]
  
#define CDIO_MMC_GET_LEN16(p)                   \
  (p[0]<<8) + p[1]
  
#define CDIO_MMC_GET_LEN32(p) \
  (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
  
#define CDIO_MMC_SET_LEN16(cdb, pos, len)       \
  cdb[pos  ] = (len >>  8) & 0xff;              \
  cdb[pos+1] = (len      ) & 0xff

#define CDIO_MMC_SET_READ_LBA(cdb, lba)         \
  cdb[2] = (lba >> 24) & 0xff; \
  cdb[3] = (lba >> 16) & 0xff; \
  cdb[4] = (lba >>  8) & 0xff; \
  cdb[5] = (lba      ) & 0xff

#define CDIO_MMC_SET_START_TRACK(cdb, command) \
  cdb[6] = command

#define CDIO_MMC_SET_READ_LENGTH24(cdb, len) \
  cdb[6] = (len >> 16) & 0xff; \
  cdb[7] = (len >>  8) & 0xff; \
  cdb[8] = (len      ) & 0xff

#define CDIO_MMC_SET_READ_LENGTH16(cdb, len) \
  CDIO_MMC_SET_LEN16(cdb, 7, len)

#define CDIO_MMC_SET_READ_LENGTH8(cdb, len) \
  cdb[8] = (len      ) & 0xff

#define CDIO_MMC_MCSB_ALL_HEADERS 0xf

#define CDIO_MMC_SET_MAIN_CHANNEL_SELECTION_BITS(cdb, val) \
  cdb[9] = val << 3;

/**
  Read Audio Subchannel information
  
  @param p_cdio the CD object to be acted upon.
  @param p_subchannel place for returned subchannel information
*/
driver_return_code_t
mmc_audio_read_subchannel (CdIo_t *p_cdio, 
                           /*out*/ cdio_subchannel_t *p_subchannel);
  
  /**
    Return a string containing the name of the audio state as returned from
    the Q_SUBCHANNEL.
  */
  const char *mmc_audio_state2str( uint8_t i_audio_state );
  
  /**
    Eject using MMC commands. If CD-ROM is "locked" we'll unlock it.
    Command is not "immediate" -- we'll wait for the command to complete.
    For a more general (and lower-level) routine, @see mmc_start_stop_media.
  */
  driver_return_code_t mmc_eject_media( const CdIo_t *p_cdio );
  
  /**
    Return a string containing the name of the given feature
  */
  const char *mmc_feature2str( int i_feature );
  
  /**
    Return a string containing the name of the given feature
  */
  const char *mmc_feature_profile2str( int i_feature_profile );
  
  /**  
    Return the length in bytes of the Command Descriptor
    Buffer (CDB) for a given MMC command. The length will be 
    either 6, 10, or 12. 
  */
  uint8_t mmc_get_cmd_len(uint8_t mmc_cmd);
  
  /**
    Get the block size used in read requests, via MMC.
    @return the blocksize if > 0; error if <= 0
  */
  int mmc_get_blocksize ( CdIo_t *p_cdio );
  
  /**
   * Close tray using a MMC START STOP command.
   */
  driver_return_code_t mmc_close_tray( CdIo_t *p_cdio );
  
  /**  
    Get the lsn of the end of the CD
    
    @return the lsn. On error return CDIO_INVALID_LSN.
  */
  lsn_t mmc_get_disc_last_lsn( const CdIo_t *p_cdio );
  
  /** 
    Return the discmode as reported by the MMC Read (FULL) TOC
    command.
    
    Information was obtained from Section 5.1.13 (Read TOC/PMA/ATIP)
    pages 56-62 from the MMC draft specification, revision 10a
    at http://www.t10.org/ftp/t10/drafts/mmc/mmc-r10a.pdf See
    especially tables 72, 73 and 75.
  */
  discmode_t mmc_get_discmode( const CdIo_t *p_cdio );
  
  
  /**
    Get drive capabilities for a device.
    @return the drive capabilities.
  */
  void mmc_get_drive_cap ( CdIo_t *p_cdio,
                           /*out*/ cdio_drive_read_cap_t  *p_read_cap,
                           /*out*/ cdio_drive_write_cap_t *p_write_cap,
                           /*out*/ cdio_drive_misc_cap_t  *p_misc_cap);
  
  typedef enum {
    CDIO_MMC_LEVEL_WEIRD,
    CDIO_MMC_LEVEL_1,
    CDIO_MMC_LEVEL_2,
    CDIO_MMC_LEVEL_3,
    CDIO_MMC_LEVEL_NONE
  } cdio_mmc_level_t;
  
  /**
    Get the MMC level supported by the device.
  */
  cdio_mmc_level_t mmc_get_drive_mmc_cap(CdIo_t *p_cdio);
  

  /** 
    Get the DVD type associated with cd object.
    
    @return the DVD discmode.
  */
  discmode_t mmc_get_dvd_struct_physical ( const CdIo_t *p_cdio, 
                                           cdio_dvd_struct_t *s);
  
  /*! 
    Return results of media status
    @param p_cdio the CD object to be acted upon.
    @param out_buf media status code from operation
    @return DRIVER_OP_SUCCESS (0) if we got the status.
    return codes are the same as driver_return_code_t
   */
  int mmc_get_event_status(const CdIo_t *p_cdio, uint8_t out_buf[2]);

  /*! 
    Find out if media tray is open or closed.
    @param p_cdio the CD object to be acted upon.
    @return 1 if media is open, 0 if closed. Error
    return codes are the same as driver_return_code_t
  */
  int mmc_get_tray_status ( const CdIo_t *p_cdio );

  /** 
    Get the CD-ROM hardware info via an MMC INQUIRY command.
    
    @return true if we were able to get hardware info, false if we had
    an error.
  */
  bool mmc_get_hwinfo ( const CdIo_t *p_cdio, 
                        /* out*/ cdio_hwinfo_t *p_hw_info );
  
  
  /** 
    Find out if media has changed since the last call.
    @param p_cdio the CD object to be acted upon.
    @return 1 if media has changed since last call, 0 if not. Error
    return codes are the same as driver_return_code_t
  */
  int mmc_get_media_changed(const CdIo_t *p_cdio);
  
  /**
    Get the media catalog number (MCN) from the CD via MMC.
    
    @return the media catalog number r NULL if there is none or we
    don't have the ability to get it.
    
    Note: string is malloc'd so caller has to free() the returned
    string when done with it.
    
  */
  char * mmc_get_mcn ( const CdIo_t *p_cdio );
  
  /** Get the output port volumes and port selections used on AUDIO PLAY
      commands via a MMC MODE SENSE command using the CD Audio Control
      Page.
  */
  driver_return_code_t mmc_audio_get_volume (CdIo_t *p_cdio,  /*out*/
                                             mmc_audio_volume_t *p_volume);
  
  /**
    Report if CD-ROM has a praticular kind of interface (ATAPI, SCSCI, ...)
    Is it possible for an interface to have serveral? If not this 
    routine could probably return the single mmc_feature_interface_t.
    @return true if we have the interface and false if not.
  */
  bool_3way_t mmc_have_interface( CdIo_t *p_cdio, 
                                  cdio_mmc_feature_interface_t e_interface );
  
  /** Run a MODE_SENSE command (6- or 10-byte version) 
    and put the results in p_buf 
    @return DRIVER_OP_SUCCESS if we ran the command ok.
  */
  int mmc_mode_sense( CdIo_t *p_cdio, /*out*/ void *p_buf, int i_size, 
                      int page);
  
  
  /** Run a MODE_SENSE command (10-byte version) 
    and put the results in p_buf 
    @return DRIVER_OP_SUCCESS if we ran the command ok.
  */
  int  mmc_mode_sense_10( CdIo_t *p_cdio, /*out*/ void *p_buf, int i_size, 
                          int page);
  
  /** Run a MODE_SENSE command (6-byte version) 
    and put the results in p_buf 
    @return DRIVER_OP_SUCCESS if we ran the command ok.
  */
  int  mmc_mode_sense_6( CdIo_t *p_cdio, /*out*/ void *p_buf, int i_size, 
                         int page);
  
  /** Issue a MMC READ_CD command.
    
  @param p_cdio  object to read from 
  
  @param p_buf   Place to store data. The caller should ensure that 
  p_buf can hold at least i_blocksize * i_blocks  bytes.
  
  @param i_lsn   sector to read 
  
  @param expected_sector_type restricts reading to a specific CD
  sector type.  Only 3 bits with values 1-5 are used:
    0 all sector types
    1 CD-DA sectors only 
    2 Mode 1 sectors only
    3 Mode 2 formless sectors only. Note in contrast to all other
      values an MMC CD-ROM is not required to support this mode.
    4 Mode 2 Form 1 sectors only
    5 Mode 2 Form 2 sectors only

  @param b_digital_audio_play Control error concealment when the
  data being read is CD-DA.  If the data being read is not CD-DA,
  this parameter is ignored.  If the data being read is CD-DA and
  DAP is false zero, then the user data returned should not be
  modified by flaw obscuring mechanisms such as audio data mute and
  interpolate.  If the data being read is CD-DA and DAP is true,
  then the user data returned should be modified by flaw obscuring
  mechanisms such as audio data mute and interpolate.  
  
  b_sync_header return the sync header (which will probably have
  the same value as CDIO_SECTOR_SYNC_HEADER of size
  CDIO_CD_SYNC_SIZE).
  
  @param header_codes Header Codes refer to the sector header and
  the sub-header that is present in mode 2 formed sectors: 
  
   0 No header information is returned.  
   1 The 4-byte sector header of data sectors is be returned, 
   2 The 8-byte sector sub-header of mode 2 formed sectors is
     returned.  
   3 Both sector header and sub-header (12 bytes) is returned.  
   The Header preceeds the rest of the bytes (e.g. user-data bytes) 
   that might get returned.
   
   @param b_user_data  Return user data if true. 
   
   For CD-DA, the User Data is CDIO_CD_FRAMESIZE_RAW bytes.

   For Mode 1, The User Data is ISO_BLOCKSIZE bytes beginning at
   offset CDIO_CD_HEADER_SIZE+CDIO_CD_SUBHEADER_SIZE.
   
   For Mode 2 formless, The User Data is M2RAW_SECTOR_SIZE bytes
   beginning at offset CDIO_CD_HEADER_SIZE+CDIO_CD_SUBHEADER_SIZE.
   
   For data Mode 2, form 1, User Data is ISO_BLOCKSIZE bytes beginning at
   offset CDIO_CD_XA_SYNC_HEADER.
   
   For data Mode 2, form 2, User Data is 2 324 bytes beginning at
   offset CDIO_CD_XA_SYNC_HEADER.
   
   @param b_sync 

   @param b_edc_ecc true if we return EDC/ECC error detection/correction bits.
   
   The presence and size of EDC redundancy or ECC parity is defined
   according to sector type: 
   
   CD-DA sectors have neither EDC redundancy nor ECC parity.  
   
   Data Mode 1 sectors have 288 bytes of EDC redundancy, Pad, and
   ECC parity beginning at offset 2064.
   
   Data Mode 2 formless sectors have neither EDC redundancy nor ECC
   parity
   
   Data Mode 2 form 1 sectors have 280 bytes of EDC redundancy and
   ECC parity beginning at offset 2072
   
   Data Mode 2 form 2 sectors optionally have 4 bytes of EDC
   redundancy beginning at offset 2348.
   
   
   @param c2_error_information If true associate a bit with each
   sector for C2 error The resulting bit field is ordered exactly as
   the main channel bytes.  Each 8-bit boundary defines a byte of
   flag bits.
   
   @param subchannel_selection subchannel-selection bits
   
     0  No Sub-channel data shall be returned.  (0 bytes)
     1  RAW P-W Sub-channel data shall be returned.  (96 byte)
     2  Formatted Q sub-channel data shall be transferred (16 bytes)
     3  Reserved     
     4  Corrected and de-interleaved R-W sub-channel (96 bytes)
     5-7  Reserved

   @param i_blocksize size of the a block expected to be returned
     
   @param i_blocks number of blocks expected to be returned.
     
  */
  driver_return_code_t
  mmc_read_cd ( const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
                int expected_sector_type, bool b_digital_audio_play,
                bool b_sync, uint8_t header_codes, bool b_user_data, 
                bool b_edc_ecc, uint8_t c2_error_information, 
                uint8_t subchannel_selection, uint16_t i_blocksize, 
                uint32_t i_blocks );
  
  /** Read just the user data part of some sort of data sector (via 
    mmc_read_cd). 
    
    @param p_cdio object to read from

    @param p_buf place to read data into.  The caller should make sure
                 this location can store at least CDIO_CD_FRAMESIZE,
                 M2RAW_SECTOR_SIZE, or M2F2_SECTOR_SIZE depending on
                 the kind of sector getting read. If you don't know
                 whether you have a Mode 1/2, Form 1/ Form 2/Formless
                 sector best to reserve space for the maximum,
                 M2RAW_SECTOR_SIZE.

    @param i_lsn sector to read
    @param i_blocksize size of each block
    @param i_blocks number of blocks to read

  */
  driver_return_code_t mmc_read_data_sectors ( CdIo_t *p_cdio, void *p_buf, 
                                               lsn_t i_lsn, 
                                               uint16_t i_blocksize,
                                               uint32_t i_blocks );
  
  /** Read sectors using SCSI-MMC GPCMD_READ_CD. 
      Can read only up to 25 blocks.
   */
  driver_return_code_t mmc_read_sectors ( const CdIo_t *p_cdio, void *p_buf, 
                                          lsn_t i_lsn,  int read_sector_type, 
                                          uint32_t i_blocks);
  
  /**
    Run a Multimedia command (MMC). 
    
    @param p_cdio	 CD structure set by cdio_open().
    @param i_timeout_ms  time in milliseconds we will wait for the command
                         to complete. 
    @param p_cdb	 CDB bytes. All values that are needed should be set 
                         on input. We'll figure out what the right CDB length 
                         should be.
    @param e_direction   direction the transfer is to go.
    @param i_buf	 Size of buffer
    @param p_buf	 Buffer for data, both sending and receiving.

    @return 0 if command completed successfully.
  */
  int mmc_run_cmd( const CdIo_t *p_cdio, unsigned int i_timeout_ms,
                   const mmc_cdb_t *p_cdb,
                   cdio_mmc_direction_t e_direction, unsigned int i_buf, 
                   /*in/out*/ void *p_buf );

  /**
    Run a Multimedia command (MMC) specifying the CDB length.
    The motivation here is for example ot use in is an undocumented 
    debug command for LG drives (namely E7), whose length is being 
    miscalculated by mmc_get_cmd_len(); it doesn't follow the usual 
    code number to length conventions. Patch supplied by SukkoPera.

    @param p_cdio	 CD structure set by cdio_open().
    @param i_timeout_ms  time in milliseconds we will wait for the command
                         to complete. 
    @param p_cdb	 CDB bytes. All values that are needed should be set 
                         on input. 
    @param i_cdb	 number of CDB bytes. 
    @param e_direction   direction the transfer is to go.
    @param i_buf	 Size of buffer
    @param p_buf	 Buffer for data, both sending and receiving.

    @return 0 if command completed successfully.
  */
  int mmc_run_cmd_len( const CdIo_t *p_cdio, unsigned int i_timeout_ms,
                   const mmc_cdb_t *p_cdb, unsigned int i_cdb,
                   cdio_mmc_direction_t e_direction, unsigned int i_buf,
                   /*in/out*/ void *p_buf );

  /**
    Set the block size for subsequest read requests, via MMC.
  */
  driver_return_code_t mmc_set_blocksize ( const CdIo_t *p_cdio, 
                                           uint16_t i_blocksize);
  
  /**
    Set the drive speed in CD-ROM speed units.

    @param p_cdio	   CD structure set by cdio_open().
    @param i_drive_speed   speed in CD-ROM speed units. Note this
                           not Kbs as would be used in the MMC spec or
		           in mmc_set_speed(). To convert CD-ROM speed units 
			   to Kbs, multiply the number by 176 (for raw data)
			   and by 150 (for filesystem data). On many CD-ROM 
			   drives, specifying a value too large will result 
			   in using the fastest speed.

    @return the drive speed if greater than 0. -1 if we had an error. is -2
    returned if this is not implemented for the current driver.

     @see cdio_set_speed and mmc_set_speed
  */
  driver_return_code_t mmc_set_drive_speed( const CdIo_t *p_cdio, 
                                            int i_drive_speed );
  
  /**
    Set the drive speed in K bytes per second.

    @param p_cdio	 CD structure set by cdio_open().
    @param i_Kbs_speed   speed in K bytes per second. Note this is 
                         not in standard CD-ROM speed units, e.g.
                         1x, 4x, 16x as it is in cdio_set_speed.
                         To convert CD-ROM speed units to Kbs,
                         multiply the number by 176 (for raw data)
                         and by 150 (for filesystem data). 
                         Also note that ATAPI specs say that a value
                         less than 176 will result in an error.
                         On many CD-ROM drives,
                         specifying a value too large will result in using
                         the fastest speed.

    @return the drive speed if greater than 0. -1 if we had an error. is -2
    returned if this is not implemented for the current driver.

    @see cdio_set_speed and mmc_set_drive_speed
  */
  driver_return_code_t mmc_set_speed( const CdIo_t *p_cdio, 
                                      int i_Kbs_speed );
  
  /**
    Load or Unload media using a MMC START STOP command. 
    
    @param p_cdio  the CD object to be acted upon.
    @param b_eject eject if true and close tray if false
    @param b_immediate wait or don't wait for operation to complete
    @param power_condition Set CD-ROM to idle/standby/sleep. If nonzero
    eject/load is ignored, so set to 0 if you want to eject or load.
    
    @see mmc_eject_media or mmc_close_tray
  */
  driver_return_code_t
  mmc_start_stop_media(const CdIo_t *p_cdio, bool b_eject, bool b_immediate,
                       uint8_t power_condition);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** The below variables are trickery to force the above enum symbol
    values to be recorded in debug symbol tables. They are used to
    allow one to refer to the enumeration value names in the typedefs
    above in a debugger and debugger expressions
*/
extern cdio_mmc_feature_t           debug_cdio_mmc_feature;
extern cdio_mmc_feature_interface_t debug_cdio_mmc_feature_interface;
extern cdio_mmc_feature_profile_t   debug_cdio_mmc_feature_profile;
extern cdio_mmc_get_conf_t          debug_cdio_mmc_get_conf;
extern cdio_mmc_gpcmd_t             debug_cdio_mmc_gpcmd;
extern cdio_mmc_read_sub_state_t    debug_cdio_mmc_read_sub_state;
extern cdio_mmc_read_cd_type_t      debug_cdio_mmc_read_cd_type;
extern cdio_mmc_readtoc_t           debug_cdio_mmc_readtoc;
extern cdio_mmc_mode_page_t         debug_cdio_mmc_mode_page;
  
#endif /* __MMC_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
