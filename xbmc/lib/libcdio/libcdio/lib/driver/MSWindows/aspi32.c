/*
    $Id: aspi32.c,v 1.10 2006/03/17 03:10:53 rocky Exp $

    Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>

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

/* This file contains Win32-specific code and implements low-level 
   control of the CD drive via the ASPI API.
   Inspired by vlc's cdrom.h code 
*/

#ifdef HAVE_CONFIG_H 
# include "config.h"
#endif

static const char _rcsid[] = "$Id: aspi32.c,v 1.10 2006/03/17 03:10:53 rocky Exp $";

#include <cdio/cdio.h>
#include <cdio/sector.h>
#include <cdio/util.h>
#include <cdio/mmc.h>
#include "cdio_assert.h"

#include <string.h>

#ifdef HAVE_WIN32_CDROM

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <windows.h>
#include "win32.h"

#include <sys/stat.h>
#include <sys/types.h>
#include "aspi32.h"
#include "cdtext_private.h"

/* Amount of time we are willing to wait for an operation to complete.
   10 seconds? 
*/
#define OP_TIMEOUT_MS 10000

static const 
char *aspierror(int nErrorCode) 
{
  switch (nErrorCode)
    {
    case SS_PENDING:
      return "SRB being processed";
      break;
    case SS_COMP:
      return "SRB completed without error";
      break;
    case SS_ABORTED:
      return "SRB aborted";
      break;
    case SS_ABORT_FAIL:
      return "Unable to abort SRB";
      break;
    case SS_ERR:
      return "SRB completed with error";
      break;
    case SS_INVALID_CMD:
      return "Invalid ASPI command";
      break;
    case SS_INVALID_HA:
      return "Invalid host adapter number";
      break;
    case SS_NO_DEVICE:
      return "SCSI device not installed";
      break;
    case SS_INVALID_SRB:
      return "Invalid parameter set in SRB";
      break;
    case SS_OLD_MANAGER:
      return "ASPI manager doesn't support";
      break;
    case SS_ILLEGAL_MODE:
      return "Unsupported MS Windows mode";
      break;
    case SS_NO_ASPI:
      return "No ASPI managers";
      break;
    case SS_FAILED_INIT:
      return "ASPI for windows failed init";
      break;
    case SS_ASPI_IS_BUSY:
      return "No resources available to execute command.";
      break;
    case SS_BUFFER_TOO_BIG:
      return "Buffer size is too big to handle.";
      break;
    case SS_MISMATCHED_COMPONENTS:
      return "The DLLs/EXEs of ASPI don't version check";
      break;
    case SS_NO_ADAPTERS:
      return "No host adapters found";
      break;
    case SS_INSUFFICIENT_RESOURCES:
      return "Couldn't allocate resources needed to init";
      break;
    case SS_ASPI_IS_SHUTDOWN:
      return "Call came to ASPI after PROCESS_DETACH";
      break;
    case SS_BAD_INSTALL:
      return "The DLL or other components are installed wrong.";
      break;
    default: 
      return "Unknown ASPI error.";
    }
}

/* General ioctl() CD-ROM command function */
static bool 
mciSendCommand_aspi(int id, UINT msg, DWORD flags, void *arg)
{
  MCIERROR mci_error;
  
  mci_error = mciSendCommand(id, msg, flags, (DWORD)arg);
  if ( mci_error ) {
    char error[256];
    
    mciGetErrorString(mci_error, error, 256);
    cdio_warn("mciSendCommand() error: %s", error);
  }
  return(mci_error == 0);
}

/*
  See if the ASPI DLL is loadable. If so pointers are returned
  and we return true. Return false if there was a problem.
 */
static bool
have_aspi( HMODULE *hASPI, 
	   long (**lpGetSupport)( void ), 
	   long (**lpSendCommand)( void* ) )
{
  /* check if aspi is available */
  *hASPI = LoadLibrary( "wnaspi32.dll" );

  if( *hASPI == NULL ) {
    cdio_debug("Unable to load ASPI DLL");
    return false;
  }

  *lpGetSupport =  (void *) GetProcAddress( *hASPI,
					    "GetASPI32SupportInfo" );
  *lpSendCommand = (void *) GetProcAddress( *hASPI,
					     "SendASPI32Command" );
  
  /* make sure that we've got both function addresses */
  if( *lpGetSupport == NULL || *lpSendCommand == NULL ) {
    cdio_debug("Unable to get ASPI function pointers");
    FreeLibrary( *hASPI );
    return false;
  }

  return true;
}

/*! 
  Get disc type associated with cd object.
*/
discmode_t
get_discmode_aspi (_img_private_t *p_env)
{
  track_t i_track;
  discmode_t discmode=CDIO_DISC_MODE_NO_INFO;

  /* See if this is a DVD. */
  cdio_dvd_struct_t dvd;  /* DVD READ STRUCT for layer 0. */

  dvd.physical.type = CDIO_DVD_STRUCT_PHYSICAL;
  dvd.physical.layer_num = 0;
  if (0 == mmc_get_dvd_struct_physical_private (p_env, &run_mmc_cmd_aspi,
						&dvd)) {
    switch(dvd.physical.layer[0].book_type) {
    case CDIO_DVD_BOOK_DVD_ROM:  return CDIO_DISC_MODE_DVD_ROM;
    case CDIO_DVD_BOOK_DVD_RAM:  return CDIO_DISC_MODE_DVD_RAM;
    case CDIO_DVD_BOOK_DVD_R:    return CDIO_DISC_MODE_DVD_R;
    case CDIO_DVD_BOOK_DVD_RW:   return CDIO_DISC_MODE_DVD_RW;
    case CDIO_DVD_BOOK_DVD_PR:   return CDIO_DISC_MODE_DVD_PR;
    case CDIO_DVD_BOOK_DVD_PRW:  return CDIO_DISC_MODE_DVD_PRW;
    default: return CDIO_DISC_MODE_DVD_OTHER;
    }
  }

  if (!p_env->gen.toc_init) 
    read_toc_aspi (p_env);

  if (!p_env->gen.toc_init) 
    return CDIO_DISC_MODE_NO_INFO;

  for (i_track = p_env->gen.i_first_track; 
       i_track < p_env->gen.i_first_track + p_env->gen.i_tracks ; 
       i_track ++) {
    track_format_t track_fmt=get_track_format_aspi(p_env, i_track);

    switch(track_fmt) {
    case TRACK_FORMAT_AUDIO:
      switch(discmode) {
	case CDIO_DISC_MODE_NO_INFO:
	  discmode = CDIO_DISC_MODE_CD_DA;
	  break;
	case CDIO_DISC_MODE_CD_DA:
	case CDIO_DISC_MODE_CD_MIXED: 
	case CDIO_DISC_MODE_ERROR: 
	  /* No change*/
	  break;
      default:
	  discmode = CDIO_DISC_MODE_CD_MIXED;
      }
      break;
    case TRACK_FORMAT_XA:
      switch(discmode) {
	case CDIO_DISC_MODE_NO_INFO:
	  discmode = CDIO_DISC_MODE_CD_XA;
	  break;
	case CDIO_DISC_MODE_CD_XA:
	case CDIO_DISC_MODE_CD_MIXED: 
	case CDIO_DISC_MODE_ERROR: 
	  /* No change*/
	  break;
      default:
	discmode = CDIO_DISC_MODE_CD_MIXED;
      }
      break;
    case TRACK_FORMAT_DATA:
      switch(discmode) {
	case CDIO_DISC_MODE_NO_INFO:
	  discmode = CDIO_DISC_MODE_CD_DATA;
	  break;
	case CDIO_DISC_MODE_CD_DATA:
	case CDIO_DISC_MODE_CD_MIXED: 
	case CDIO_DISC_MODE_ERROR: 
	  /* No change*/
	  break;
      default:
	discmode = CDIO_DISC_MODE_CD_MIXED;
      }
      break;
    case TRACK_FORMAT_ERROR:
    default:
      discmode = CDIO_DISC_MODE_ERROR;
    }
  }
  return discmode;
}

const char *
is_cdrom_aspi(const char drive_letter) 
{
  static char psz_win32_drive[7];
  HMODULE hASPI = NULL;
  long (*lpGetSupport)( void ) = NULL;
  long (*lpSendCommand)( void* ) = NULL;
  DWORD dwSupportInfo;
  int i_adapter, i_hostadapters;
  char c_drive;
  int i_rc;

  if ( !have_aspi(&hASPI, &lpGetSupport, &lpSendCommand) )
    return NULL;

  /* ASPI support seems to be there. */
  
  dwSupportInfo = lpGetSupport();

  i_rc = HIBYTE( LOWORD ( dwSupportInfo ) );

  if( SS_COMP != i_rc ) {
    cdio_debug("ASPI: %s", aspierror(i_rc));
    FreeLibrary( hASPI );
    return NULL;
  }
  
  i_hostadapters = LOBYTE( LOWORD( dwSupportInfo ) );
  if( i_hostadapters == 0 ) {
    FreeLibrary( hASPI );
    return NULL;
  }
  
  c_drive = toupper(drive_letter) - 'A';
  
  for( i_adapter = 0; i_adapter < i_hostadapters; i_adapter++ ) {
    struct SRB_GetDiskInfo srbDiskInfo;
    int i_target;
    SRB_HAInquiry srbInquiry;
    
    srbInquiry.SRB_Cmd         = SC_HA_INQUIRY;
    srbInquiry.SRB_HaId        = i_adapter;
    
    lpSendCommand( (void*) &srbInquiry );
    
    if( srbInquiry.SRB_Status != SS_COMP ) continue;
    if( !srbInquiry.HA_Unique[3]) srbInquiry.HA_Unique[3]=8;
    
    for(i_target=0; i_target < srbInquiry.HA_Unique[3]; i_target++)
      {
	int i_lun;
	for( i_lun=0; i_lun<8; i_lun++)
	  {
	    srbDiskInfo.SRB_Cmd         = SC_GET_DISK_INFO;
	    srbDiskInfo.SRB_Flags       = 0;
	    srbDiskInfo.SRB_Hdr_Rsvd    = 0;
	    srbDiskInfo.SRB_HaId        = i_adapter;
	    srbDiskInfo.SRB_Target      = i_target;
	    srbDiskInfo.SRB_Lun         = i_lun;
	    
	    lpSendCommand( (void*) &srbDiskInfo );
	    
	    if( (srbDiskInfo.SRB_Status == SS_COMP) &&
		(srbDiskInfo.SRB_Int13HDriveInfo == c_drive) ) {
	      /* Make sure this is a CD-ROM device. */
	      struct SRB_GDEVBlock   srbGDEVBlock;
	      
	      memset( &srbGDEVBlock, 0, sizeof(struct SRB_GDEVBlock) );
	      srbGDEVBlock.SRB_Cmd    = SC_GET_DEV_TYPE;
	      srbDiskInfo.SRB_HaId    = i_adapter;
	      srbGDEVBlock.SRB_Target = i_target;
	      srbGDEVBlock.SRB_Lun    = i_lun;
	      
	      lpSendCommand( (void*) &srbGDEVBlock );
	      
	      if( ( srbGDEVBlock.SRB_Status == SS_COMP ) &&
		  ( srbGDEVBlock.SRB_DeviceType == DTYPE_CDROM ) ) {
		sprintf( psz_win32_drive, "%c:", drive_letter );
		FreeLibrary( hASPI );
		return(psz_win32_drive);
	      }
	    }
	  }
      }
  }
  FreeLibrary( hASPI );
  return NULL;
}

/*!
  Initialize CD device.
 */
bool
init_aspi (_img_private_t *env)
{
  HMODULE hASPI = NULL;
  long (*lpGetSupport)( void ) = NULL;
  long (*lpSendCommand)( void* ) = NULL;
  DWORD dwSupportInfo;
  int i_adapter, i_hostadapters;
  char c_drive;
  int i_rc;

  if (2 == strlen(env->gen.source_name) && isalpha(env->gen.source_name[0]) ) 
  {
    c_drive = env->gen.source_name[0];
  } else if ( 6 == strlen(env->gen.source_name) 
	      && isalpha(env->gen.source_name[4] )) {
    c_drive = env->gen.source_name[4];
  } else {
    c_drive = 'C';
  }
  
  if ( !have_aspi(&hASPI, &lpGetSupport, &lpSendCommand) )
    return false;

  /* ASPI support seems to be there. */
  
  dwSupportInfo = lpGetSupport();
  
  i_rc = HIBYTE( LOWORD ( dwSupportInfo ) );

  if( SS_COMP != i_rc ) {
    cdio_info("ASPI: %s", aspierror(i_rc));
    FreeLibrary( hASPI );
    return false;
  }
  
  i_hostadapters = LOBYTE( LOWORD( dwSupportInfo ) );
  if( i_hostadapters == 0 ) {
    FreeLibrary( hASPI );
    return false;
  }
  
  c_drive = toupper(c_drive) - 'A';
  
  for( i_adapter = 0; i_adapter < i_hostadapters; i_adapter++ ) {
    struct SRB_GetDiskInfo srbDiskInfo;
    int i_target;
    SRB_HAInquiry srbInquiry;
    
    srbInquiry.SRB_Cmd         = SC_HA_INQUIRY;
    srbInquiry.SRB_HaId        = i_adapter;
    
    lpSendCommand( (void*) &srbInquiry );
    
    if( srbInquiry.SRB_Status != SS_COMP ) continue;
    if( !srbInquiry.HA_Unique[3]) srbInquiry.HA_Unique[3]=8;
    
    for(i_target=0; i_target < srbInquiry.HA_Unique[3]; i_target++)
      {
	int i_lun;
	for (i_lun = 0; i_lun < 8; i_lun++ ) {
	  srbDiskInfo.SRB_Cmd         = SC_GET_DISK_INFO;
	  srbDiskInfo.SRB_Flags       = 0;
	  srbDiskInfo.SRB_Hdr_Rsvd    = 0;
	  srbDiskInfo.SRB_HaId        = i_adapter;
	  srbDiskInfo.SRB_Target      = i_target;
	  srbDiskInfo.SRB_Lun         = i_lun;
	  
	  lpSendCommand( (void*) &srbDiskInfo );
	  
	  if( (srbDiskInfo.SRB_Status == SS_COMP) ) {
	    
	    if (srbDiskInfo.SRB_Int13HDriveInfo != c_drive)
	      {
		continue;
	      } else {
		/* Make sure this is a CD-ROM device. */
		struct SRB_GDEVBlock   srbGDEVBlock;
		
		memset( &srbGDEVBlock, 0, sizeof(struct SRB_GDEVBlock) );
		srbGDEVBlock.SRB_Cmd    = SC_GET_DEV_TYPE;
		srbGDEVBlock.SRB_HaId   = i_adapter;
		srbGDEVBlock.SRB_Target = i_target;
		
		lpSendCommand( (void*) &srbGDEVBlock );
		
		if( ( srbGDEVBlock.SRB_Status == SS_COMP ) &&
		    ( srbGDEVBlock.SRB_DeviceType == DTYPE_CDROM ) ) {
		  env->i_sid = MAKEWORD( i_adapter, i_target );
		  env->hASPI = (long)hASPI;
		  env->lpSendCommand = lpSendCommand;
		  env->b_aspi_init   = true;
		  env->i_lun         = i_lun;
		  cdio_debug("Using ASPI layer");
		  
		  return true;
		} else {
		  FreeLibrary( hASPI );
		  cdio_debug( "%c: is not a CD-ROM drive",
			      env->gen.source_name[0] );
		  return false;
		}
	      }
	  }
	}
    }
  }
  
  FreeLibrary( hASPI );
  cdio_debug( "Unable to get HaId and target (ASPI)" );
  return false;
}

/*!
  Run a SCSI MMC command. 
 
  env	        private CD structure 
  i_timeout_ms  time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  p_buf	        Buffer for data, both sending and receiving
  i_buf	        Size of buffer
  e_direction	direction the transfer is to go.
  cdb	        CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.

  We return 0 if command completed successfully.
 */
int
run_mmc_cmd_aspi( void *p_user_data, unsigned int i_timeout_ms,
		  unsigned int i_cdb, const mmc_cdb_t * p_cdb,  
		  cdio_mmc_direction_t e_direction, 
		  unsigned int i_buf, /*in/out*/ void *p_buf )
{
  const _img_private_t *p_env = p_user_data;
  HANDLE hEvent;
  struct SRB_ExecSCSICmd ssc;

  /* Create the transfer completion event */
  hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
  if( hEvent == NULL ) {
    cdio_info("CreateEvent failed");
    return 1;
  }
  
  memset( &ssc, 0, sizeof( ssc ) );

  ssc.SRB_Cmd         = SC_EXEC_SCSI_CMD;

  ssc.SRB_Flags       = SCSI_MMC_DATA_READ == e_direction ? 
    SRB_DIR_IN | SRB_EVENT_NOTIFY : SRB_DIR_OUT | SRB_EVENT_NOTIFY;

  ssc.SRB_HaId        = LOBYTE( p_env->i_sid );
  ssc.SRB_Target      = HIBYTE( p_env->i_sid );
  ssc.SRB_Lun         = p_env->i_lun;
  ssc.SRB_SenseLen    = SENSE_LEN;
  
  ssc.SRB_PostProc = (LPVOID) hEvent;
  ssc.SRB_CDBLen      = i_cdb;
  
  /* Result buffer */
  ssc.SRB_BufPointer  = p_buf;
  ssc.SRB_BufLen      = i_buf;

  memcpy( ssc.CDBByte, p_cdb, i_cdb );
  
  ResetEvent( hEvent );
  p_env->lpSendCommand( (void*) &ssc );
  
  /* If the command has still not been processed, wait until it's
   * finished */
  if( ssc.SRB_Status == SS_PENDING ) {
    WaitForSingleObject( hEvent, msecs2secs(i_timeout_ms) );
  }
  CloseHandle( hEvent );
  
  /* check that the transfer went as planned */
  if( ssc.SRB_Status != SS_COMP ) {
    cdio_info("ASPI: %s", aspierror(ssc.SRB_Status));
    return 2;
  }

  return 0;
}


/*!
   Reads nblocks sectors from cd device into data starting from lsn.
   Returns 0 if no error. 
 */
static int
read_sectors_aspi (_img_private_t *p_env, void *data, lsn_t lsn, 
		   int sector_type, unsigned int nblocks)
{
  mmc_cdb_t cdb = {{0, }};
  unsigned int i_buf;

  int sync        = 0;
  int header_code = 2;
  int i_user_data = 1;
  int edc_ecc     = 0;
  int error_field = 0;
  
#if 0  
  sector_type = 0; /*all types */
#endif
  
  /* Set up passthrough command */
  CDIO_MMC_SET_COMMAND      (cdb.field, CDIO_MMC_GPCMD_READ_CD);
  CDIO_MMC_SET_READ_TYPE    (cdb.field, sector_type);
  CDIO_MMC_SET_READ_LBA     (cdb.field, lsn);
  CDIO_MMC_SET_READ_LENGTH24(cdb.field, nblocks);
  
#if 1
  cdb.field[ 9 ] = (sync << 7) |
    (header_code << 5) |
    (i_user_data << 4) |
    (edc_ecc << 3) |
    (error_field << 1);
  /* ssc.CDBByte[ 9 ] = READ_CD_USERDATA_MODE2; */
#else 
  CDIO_MMC_SET_MAIN_CHANNEL_SELECTION_BITS(cmd,
					   CDIO_MMC_MCSB_ALL_HEADERS);
#endif
  
  switch (sector_type) {
  case CDIO_MMC_READ_TYPE_ANY: 
  case CDIO_MMC_READ_TYPE_CDDA: 
    i_buf = CDIO_CD_FRAMESIZE_RAW;
    break;
  case CDIO_MMC_READ_TYPE_M2F1:
    i_buf = CDIO_CD_FRAMESIZE;
    break;
  case CDIO_MMC_READ_TYPE_M2F2:
    i_buf = 2324;
    break;
  case CDIO_MMC_READ_TYPE_MODE1:
    i_buf = CDIO_CD_FRAMESIZE;
    break;
  default:
    i_buf = CDIO_CD_FRAMESIZE_RAW;
  }

  return run_mmc_cmd_aspi(p_env, OP_TIMEOUT_MS, 
			  mmc_get_cmd_len(cdb.field[0]), 
			  &cdb, SCSI_MMC_DATA_READ, i_buf*nblocks, data);
}

/*!
   Reads an audio device into data starting from lsn.
   Returns 0 if no error. 
 */
int
read_audio_sectors_aspi (_img_private_t *p_env, void *data, lsn_t lsn, 
			 unsigned int i_blocks) 
{
  if (read_sectors_aspi(p_env, data, lsn, CDIO_MMC_READ_TYPE_CDDA, i_blocks)) {
    return read_sectors_aspi(p_env, data, lsn, 
			     CDIO_MMC_READ_TYPE_ANY, i_blocks);
  }
  return 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
int
read_mode2_sector_aspi (_img_private_t *p_env, void *data, lsn_t lsn, 
			 bool b_form2)
{
  return read_sectors_aspi(p_env, data, lsn, b_form2 
			       ? CDIO_MMC_READ_TYPE_M2F2 
			       : CDIO_MMC_READ_TYPE_M2F1, 
			       1);
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
int
read_mode1_sector_aspi (_img_private_t *p_env, void *data, lsn_t lsn, 
			 bool b_form2)
{
  return read_sectors_aspi(p_env, data, lsn, CDIO_MMC_READ_TYPE_MODE1, 1);
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
bool
read_toc_aspi (_img_private_t *p_env) 
{
  mmc_cdb_t cdb = {{0, }};
  unsigned char tocheader[ 4 ];
  int      i_status;
  
  /* Operation code */
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_TOC);
  
  /* Format */
  cdb.field[ 2 ] = CDIO_MMC_READTOC_FMT_TOC;
  
  /* Starting track */
  CDIO_MMC_SET_START_TRACK(cdb.field, 0);
  
  CDIO_MMC_SET_READ_LENGTH16(cdb.field, sizeof(tocheader));

  i_status = run_mmc_cmd_aspi (p_env, OP_TIMEOUT_MS,
			       mmc_get_cmd_len(cdb.field[0]), 
			       &cdb, SCSI_MMC_DATA_READ, 
			       sizeof(tocheader), &tocheader);
  
  if (0 != i_status) return false;
  
  p_env->gen.i_first_track = tocheader[2];
  p_env->gen.i_tracks  = tocheader[3] - tocheader[2] + 1;
  
  {
    int i, j, i_toclength;
    unsigned char *p_fulltoc;
    
    i_toclength = 4 /* header */ + tocheader[0] +
      ((unsigned int) tocheader[1] << 8);
    
    p_fulltoc = malloc( i_toclength );
    
    if( p_fulltoc == NULL ) {
      cdio_error( "out of memory" );
      return false;
    }
    
    CDIO_MMC_SET_READ_LENGTH16(cdb.field, i_toclength);

    i_status = run_mmc_cmd_aspi (p_env, OP_TIMEOUT_MS,
				 mmc_get_cmd_len(cdb.field[0]), 
				 &cdb, SCSI_MMC_DATA_READ, 
				 i_toclength, p_fulltoc);
    if( 0 != i_status ) {
      p_env->gen.i_tracks = 0;
    }

    j = p_env->gen.i_first_track;
    
    for( i = 0 ; i <= p_env->gen.i_tracks ; i++, j++ ) {
      int i_index = 8 + 8 * i;
      p_env->tocent[ i ].start_lsn = ((int)p_fulltoc[ i_index ] << 24) +
	((int)p_fulltoc[ i_index+1 ] << 16) +
	((int)p_fulltoc[ i_index+2 ] << 8) +
	(int)p_fulltoc[ i_index+3 ];
      p_env->tocent[i].Control   = (UCHAR)p_fulltoc[ 1 + 8 * i ];
      
      set_track_flags(&(p_env->gen.track_flags[j]), p_env->tocent[i].Control);
    
      cdio_debug( "p_sectors: %i %lu", 
		  i, (unsigned long int) p_env->tocent[i].start_lsn );
    }
    
    free( p_fulltoc );
  }
  
  p_env->gen.toc_init = true;
  return true;
}

/* Eject media will eventually get removed from _cdio_win32.c */
#if 0
/*!
  Eject media. Return 1 if successful, 0 otherwise.
 */
int 
wnaspi32_eject_media (void *user_data) {

  _img_private_t *env = user_data;


  MCI_OPEN_PARMS op;
  MCI_STATUS_PARMS st;
  DWORD i_flags;
  char psz_drive[4];
  int ret;
    
  memset( &op, 0, sizeof(MCI_OPEN_PARMS) );
  op.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
  strcpy( psz_drive, "X:" );
  psz_drive[0] = env->gen.source_name[0];
  op.lpstrElementName = psz_drive;
  
  /* Set the flags for the device type */
  i_flags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
    MCI_OPEN_ELEMENT | MCI_OPEN_SHAREABLE;
  
  if( mciSendCommand_aspi( 0, MCI_OPEN, i_flags, &op ) ) {
    st.dwItem = MCI_STATUS_READY;
    /* Eject disc */
    ret = mciSendCommand_aspi( op.wDeviceID, MCI_SET, 
				 MCI_SET_DOOR_OPEN, 0 ) != 0;
    /* Release access to the device */
    mciSendCommand_aspi( op.wDeviceID, MCI_CLOSE, MCI_WAIT, 0 );
  } else 
    ret = 0;
  
  return ret;
}
#endif

/*!  
  Get format of track. 
*/
track_format_t
get_track_format_aspi(const _img_private_t *p_env, track_t track_num) 
{
  MCI_OPEN_PARMS op;
  MCI_STATUS_PARMS st;
  DWORD i_flags;
  int ret;

  memset( &op, 0, sizeof(MCI_OPEN_PARMS) );
  op.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
  op.lpstrElementName = p_env->gen.source_name;
  
  /* Set the flags for the device type */
  i_flags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
    MCI_OPEN_ELEMENT | MCI_OPEN_SHAREABLE;
  
  if( mciSendCommand_aspi( 0, MCI_OPEN, i_flags, &op ) ) {
    st.dwItem  = MCI_CDA_STATUS_TYPE_TRACK;
    st.dwTrack = track_num;
    i_flags = MCI_TRACK | MCI_STATUS_ITEM ;
    ret = mciSendCommand_aspi( op.wDeviceID, MCI_STATUS, i_flags, &st );
    
    /* Release access to the device */
    mciSendCommand_aspi( op.wDeviceID, MCI_CLOSE, MCI_WAIT, 0 );
    
    switch(st.dwReturn) {
    case MCI_CDA_TRACK_AUDIO:
      return TRACK_FORMAT_AUDIO;
    case MCI_CDA_TRACK_OTHER:
      return TRACK_FORMAT_DATA;
    default:
      return TRACK_FORMAT_XA;
    }
  }
  return TRACK_FORMAT_ERROR;
}

#endif /* HAVE_WIN32_CDROM */
