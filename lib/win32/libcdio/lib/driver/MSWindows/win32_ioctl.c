/*
    $Id: win32_ioctl.c,v 1.8 2005/01/27 11:08:55 rocky Exp $

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

/* This file contains Win32-specific code using the DeviceIoControl
   access method.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: win32_ioctl.c,v 1.8 2005/01/27 11:08:55 rocky Exp $";

#ifdef HAVE_WIN32_CDROM

#if defined (_XBOX) || defined (WIN32)
#include <windows.h>
#include "inttypes.h"
#include <my_ntddscsi.h>
#define FORMAT_ERROR(i_err, psz_msg) \
   psz_msg=(char *)LocalAlloc(LMEM_ZEROINIT, 255); \
   sprintf(psz_msg, "error file %s: line %d %d\n", \
	   __FILE__, __LINE__, i_err)
#else
#include <ddk/ntddstor.h>
#include <ddk/ntddscsi.h>
#include <ddk/scsi.h>
#define FORMAT_ERROR(i_err, psz_msg) \
   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, \
		 NULL, i_err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
		 (LPSTR) psz_msg, 0, NULL)
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stddef.h>  /* offsetof() macro */
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>

#include <cdio/cdio.h>
#include <cdio/sector.h>
#include "cdio_assert.h"
#include <cdio/scsi_mmc.h>
#include "cdtext_private.h"
#include "cdio/logging.h"

/* Win32 DeviceIoControl specifics */
/***** FIXME: #include ntddcdrm.h from Wine, but probably need to 
   modify it a little.
*/

#ifndef IOCTL_CDROM_BASE
#    define IOCTL_CDROM_BASE FILE_DEVICE_CD_ROM
#endif
#ifndef IOCTL_CDROM_READ_TOC
#define IOCTL_CDROM_READ_TOC \
  CTL_CODE(IOCTL_CDROM_BASE, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif
#ifndef IOCTL_CDROM_RAW_READ
#define IOCTL_CDROM_RAW_READ CTL_CODE(IOCTL_CDROM_BASE, 0x000F, \
                                      METHOD_OUT_DIRECT, FILE_READ_ACCESS)
#endif

#ifndef IOCTL_CDROM_READ_Q_CHANNEL
#define IOCTL_CDROM_READ_Q_CHANNEL \
  CTL_CODE(IOCTL_CDROM_BASE, 0x000B, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

typedef struct {
   SCSI_PASS_THROUGH Spt;
   ULONG Filler;
   UCHAR SenseBuf[32];
   UCHAR DataBuf[512];
} SCSI_PASS_THROUGH_WITH_BUFFERS;

typedef struct _TRACK_DATA {
    UCHAR Format;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TrackNumber;
    UCHAR Reserved1;
    UCHAR Address[4];
} TRACK_DATA, *PTRACK_DATA;

typedef struct _CDROM_TOC {
    UCHAR Length[2];
    UCHAR FirstTrack;
    UCHAR LastTrack;
    TRACK_DATA TrackData[CDIO_CD_MAX_TRACKS+1];
} CDROM_TOC, *PCDROM_TOC;

typedef struct _TRACK_DATA_FULL {
    UCHAR SessionNumber;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TNO;
    UCHAR POINT;  /* Tracknumber (of session?) or lead-out/in (0xA0, 0xA1, 0xA2)  */ 
    UCHAR Min;  /* Only valid if disctype is CDDA ? */
    UCHAR Sec;  /* Only valid if disctype is CDDA ? */
    UCHAR Frame;  /* Only valid if disctype is CDDA ? */
    UCHAR Zero;  /* Always zero */
    UCHAR PMIN;  /* start min, if POINT is a track; if lead-out/in 0xA0: First Track */
    UCHAR PSEC;
    UCHAR PFRAME;
} TRACK_DATA_FULL, *PTRACK_DATA_FULL;

typedef struct _CDROM_TOC_FULL {
    UCHAR Length[2];
    UCHAR FirstSession;
    UCHAR LastSession;
    TRACK_DATA_FULL TrackData[CDIO_CD_MAX_TRACKS+3];
} CDROM_TOC_FULL, *PCDROM_TOC_FULL;

typedef enum _TRACK_MODE_TYPE {
    YellowMode2,
    XAForm2,
    CDDA
} TRACK_MODE_TYPE, *PTRACK_MODE_TYPE;

typedef struct __RAW_READ_INFO {
    LARGE_INTEGER DiskOffset;
    ULONG SectorCount;
    TRACK_MODE_TYPE TrackMode;
} RAW_READ_INFO, *PRAW_READ_INFO;

typedef struct _CDROM_SUB_Q_DATA_FORMAT {
    UCHAR               Format;
    UCHAR               Track;
} CDROM_SUB_Q_DATA_FORMAT, *PCDROM_SUB_Q_DATA_FORMAT;

typedef struct _SUB_Q_HEADER {
  UCHAR Reserved;
  UCHAR AudioStatus;
  UCHAR DataLength[2];
} SUB_Q_HEADER, *PSUB_Q_HEADER;

typedef struct _SUB_Q_MEDIA_CATALOG_NUMBER {
  SUB_Q_HEADER Header;
  UCHAR FormatCode;
  UCHAR Reserved[3];
  UCHAR Reserved1 : 7;
  UCHAR Mcval :1;
  UCHAR MediaCatalog[15];
} SUB_Q_MEDIA_CATALOG_NUMBER, *PSUB_Q_MEDIA_CATALOG_NUMBER;

#include "win32.h"

#define OP_TIMEOUT_MS 60 

/*!
  Run a SCSI MMC command. 
 
  env	        private CD structure 
  i_timeout     time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  p_buf	        Buffer for data, both sending and receiving
  i_buf	        Size of buffer
  e_direction	direction the transfer is to go.
  cdb	        CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.

  Return 0 if command completed successfully.
 */
int
run_scsi_cmd_win32ioctl( void *p_user_data, 
			 unsigned int i_timeout_ms,
			 unsigned int i_cdb, const scsi_mmc_cdb_t * p_cdb,
			 scsi_mmc_direction_t e_direction, 
			 unsigned int i_buf, /*in/out*/ void *p_buf )
{
  const _img_private_t *p_env = p_user_data;
  SCSI_PASS_THROUGH_DIRECT sptd;
  bool success;
  DWORD dwBytesReturned;
  
  sptd.Length  = sizeof(sptd);
  sptd.PathId  = 0;      /* SCSI card ID will be filled in automatically */
  sptd.TargetId= 0;      /* SCSI target ID will also be filled in */
  sptd.Lun=0;            /* SCSI lun ID will also be filled in */
  sptd.CdbLength         = i_cdb;
  sptd.SenseInfoLength   = 0; /* Don't return any sense data */
  sptd.DataIn            = SCSI_MMC_DATA_READ == e_direction ? 
    SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT; 
  sptd.DataTransferLength= i_buf; 
  sptd.TimeOutValue      = msecs2secs(i_timeout_ms);
  sptd.DataBuffer        = (void *) p_buf;
  sptd.SenseInfoOffset   = 0;

  memcpy(sptd.Cdb, p_cdb, i_cdb);

  /* Send the command to drive */
  success=DeviceIoControl(p_env->h_device_handle,
			  IOCTL_SCSI_PASS_THROUGH_DIRECT,               
			  (void *)&sptd, 
			  (DWORD)sizeof(SCSI_PASS_THROUGH_DIRECT),
			  NULL, 0,                        
			  &dwBytesReturned,
			  NULL);

  if(! success) {
    char *psz_msg = NULL;
    long int i_err = GetLastError();
    FORMAT_ERROR(i_err, psz_msg);
    cdio_info("Error: %s", psz_msg);
    LocalFree(psz_msg);
    return 1;
  }
  return 0;
}

/*! 
  Get disc type associated with cd object.
*/
discmode_t
get_discmode_win32ioctl (_img_private_t *p_env)
{
  track_t i_track;
  discmode_t discmode=CDIO_DISC_MODE_NO_INFO;

  /* See if this is a DVD. */
  cdio_dvd_struct_t dvd;  /* DVD READ STRUCT for layer 0. */

  dvd.physical.type = CDIO_DVD_STRUCT_PHYSICAL;
  dvd.physical.layer_num = 0;
  if (0 == scsi_mmc_get_dvd_struct_physical_private (p_env, 
						     &run_scsi_cmd_win32ioctl,
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
    read_toc_win32ioctl (p_env);

  if (!p_env->gen.toc_init) 
    return CDIO_DISC_MODE_NO_INFO;

  for (i_track = p_env->gen.i_first_track; 
       i_track < p_env->gen.i_first_track + p_env->gen.i_tracks ; 
       i_track ++) {
    track_format_t track_fmt=get_track_format_win32ioctl(p_env, i_track);

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

/*
  Returns a string that can be used in a CreateFile call if 
  c_drive letter is a character. If not NULL is returned.
 */

const char *
is_cdrom_win32ioctl(const char c_drive_letter) 
{
#ifdef _XBOX
	char sz_win32_drive_full[] = "\\\\.\\X:";
	sz_win32_drive_full[4] = c_drive_letter;
	return strdup(sz_win32_drive_full);
#else
    UINT uDriveType;
    char sz_win32_drive[4];
    
    sz_win32_drive[0]= c_drive_letter;
    sz_win32_drive[1]=':';
    sz_win32_drive[2]='\\';
    sz_win32_drive[3]='\0';

    uDriveType = GetDriveType(sz_win32_drive);

    switch(uDriveType) {
    case DRIVE_CDROM: {
        char sz_win32_drive_full[] = "\\\\.\\X:";
	sz_win32_drive_full[4] = c_drive_letter;
	return strdup(sz_win32_drive_full);
    }
    default:
        cdio_debug("Drive %c is not a CD-ROM", c_drive_letter);
        return NULL;
    }
#endif
}
  
/*!
   Reads an audio device using the DeviceIoControl method into data
   starting from lsn.  Returns 0 if no error.
 */
int
read_audio_sectors_win32ioctl (_img_private_t *p_env, void *data, lsn_t lsn, 
			       unsigned int nblocks) 
{
  DWORD dwBytesReturned;
  RAW_READ_INFO cdrom_raw;
  
  /* Initialize CDROM_RAW_READ structure */
  cdrom_raw.DiskOffset.QuadPart = (long long) CDIO_CD_FRAMESIZE_RAW * lsn;
  cdrom_raw.SectorCount = nblocks;
  cdrom_raw.TrackMode = CDDA;
  
  if( DeviceIoControl( p_env->h_device_handle,
		       IOCTL_CDROM_RAW_READ, &cdrom_raw,
		       sizeof(RAW_READ_INFO), data,
		       CDIO_CD_FRAMESIZE_RAW * nblocks,
		       &dwBytesReturned, NULL ) == 0 ) {
    char *psz_msg = NULL;
    long int i_err = GetLastError();
    FORMAT_ERROR(i_err, psz_msg);
    if (psz_msg) {
      cdio_info("Error reading audio-mode lsn %lu\n%s (%ld))", 
		(long unsigned int) lsn, psz_msg, i_err);
    } else {
      cdio_info("Error reading audio-mode lsn %lu\n (%ld))",
		(long unsigned int) lsn, i_err);
    }
    LocalFree(psz_msg);
    return 1;
  }
  return 0;
}

/*!
   Reads a single sector from a dvd using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
static int
read_sector_dvd (const _img_private_t *p_env, void *p_buf, lsn_t lsn) 
{
  scsi_mmc_cdb_t cdb = {{0, }};

  /* Read CDB12 command.  The values were taken from MMC1 draft paper. */
  CDIO_MMC_SET_COMMAND      (cdb.field, CDIO_MMC_GPCMD_READ_12);
  CDIO_MMC_SET_READ_LBA     (cdb.field, cdio_lsn_to_lba(lsn));
  CDIO_MMC_SET_READ_LENGTH24(cdb.field, CDIO_CD_FRAMESIZE);

  return run_scsi_cmd_win32ioctl(p_env, OP_TIMEOUT_MS,
				     scsi_mmc_get_cmd_len(cdb.field[0]), 
				     &cdb, SCSI_MMC_DATA_READ, 
				     CDIO_CD_FRAMESIZE, p_buf);
}

/*!
   Reads a single raw sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
static int
read_raw_sector (const _img_private_t *p_env, void *p_buf, lsn_t lsn) 
{
	BOOL bResult;
	DWORD dwRead, dwPtrLow, dwError;
	DWORD dwSectorSize = CDIO_CD_FRAMESIZE;
	LARGE_INTEGER Displacement;

	Displacement.QuadPart = ((INT64)lsn) * dwSectorSize;

	dwPtrLow=SetFilePointer(p_env->h_device_handle, Displacement.LowPart, &Displacement.HighPart, FILE_BEGIN);
	if (dwPtrLow == INVALID_SET_FILE_POINTER && (dwError = GetLastError()) != NO_ERROR )
	{ 
		cdio_error ("SetFilePointer() to lsn=%d failed with GetLastError()=%d", lsn, GetLastError());
		return 1;
	}  

	bResult=ReadFile(p_env->h_device_handle, p_buf, dwSectorSize, &dwRead, NULL);

	if (bResult &&  dwRead == 0 ) 
	{ 
		cdio_error ("ReadFile() on lsn=%d failed with GetLastError()=%d", lsn, GetLastError()); 
		return 1;
	}

	return 0;

  //scsi_mmc_cdb_t cdb = {{0, }};

  ///* ReadCD CDB10 command.  The values were taken from MMC1 draft paper. */
  //CDIO_MMC_SET_COMMAND      (cdb.field, CDIO_MMC_GPCMD_READ_CD);
  //CDIO_MMC_SET_READ_LBA     (cdb.field, cdio_lsn_to_lba(lsn));
  //CDIO_MMC_SET_READ_LENGTH24(cdb.field, CDIO_CD_FRAMESIZE_RAW);

  //cdb.field[9]=0xF8;  /* Raw read, 2352 bytes per sector */
  //
  //return run_scsi_cmd_win32ioctl(p_env, OP_TIMEOUT_MS,
		//		     scsi_mmc_get_cmd_len(cdb.field[0]), 
		//		     &cdb, SCSI_MMC_DATA_READ, 
		//		     CDIO_CD_FRAMESIZE_RAW, p_buf);
}

/*!
   Reads a single mode2 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int
read_mode2_sector_win32ioctl (const _img_private_t *p_env, void *p_data, 
			      lsn_t lsn, bool b_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int ret = read_raw_sector (p_env, buf, lsn);

  if ( 0 != ret) return ret;

  memcpy (p_data,
	  buf/* + CDIO_CD_SYNC_SIZE + CDIO_CD_XA_HEADER*/,
	  b_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);
  
  return 0;

}

/*!
   Reads a single mode2 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int
read_mode1_sector_win32ioctl (const _img_private_t *env, void *data, 
			      lsn_t lsn, bool b_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int ret = read_raw_sector (env, buf, lsn);

  if ( 0 != ret) return ret;

  memcpy (data, 
	  buf /* + CDIO_CD_SYNC_SIZE+CDIO_CD_HEADER_SIZE*/,
	  b_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);  

  return 0;

}

/*!
  Initialize internal structures for CD device.
 */
bool
init_win32ioctl (_img_private_t *env)
{
#ifdef WIN32
  OSVERSIONINFO ov;
#endif

#ifdef _XBOX
  ANSI_STRING filename;
  OBJECT_ATTRIBUTES attributes;
  IO_STATUS_BLOCK status;
  HANDLE hDevice;
  NTSTATUS error;
#else
  unsigned int len=strlen(env->gen.source_name);
  char psz_win32_drive[7];
  DWORD dw_access_flags;
#endif
  
  cdio_debug("using winNT/2K/XP ioctl layer");

#ifdef WIN32
  memset(&ov,0,sizeof(OSVERSIONINFO));
  ov.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
  GetVersionEx(&ov);
  
  if((ov.dwPlatformId==VER_PLATFORM_WIN32_NT) &&        
     (ov.dwMajorVersion>4))
    dw_access_flags = GENERIC_READ|GENERIC_WRITE;  /* add gen write on W2k/XP */
  else dw_access_flags = GENERIC_READ;
#endif

  if (cdio_is_device_win32(env->gen.source_name)) 
  {
#ifdef _XBOX
    //	Use XBOX cdrom, no matter what drive letter is given.
    RtlInitAnsiString(&filename,"\\Device\\Cdrom0");
    InitializeObjectAttributes(&attributes, &filename, OBJ_CASE_INSENSITIVE,
			       NULL);
    error = NtCreateFile( &hDevice, 
			  GENERIC_READ |SYNCHRONIZE | FILE_READ_ATTRIBUTES, 
			  &attributes, 
			  &status, 
			  NULL, 
			  0,
			  FILE_SHARE_READ,
			  FILE_OPEN,	
			  FILE_NON_DIRECTORY_FILE 
			  | FILE_SYNCHRONOUS_IO_NONALERT );
    
    if (!NT_SUCCESS(error))
    {
	  return false;
    }
	env->h_device_handle = hDevice;
#else
    sprintf( psz_win32_drive, "\\\\.\\%c:", env->gen.source_name[len-2] );

    env->h_device_handle = CreateFile( psz_win32_drive, 
				       dw_access_flags,
				       FILE_SHARE_READ | FILE_SHARE_WRITE, 
				       NULL, 
				       OPEN_EXISTING,
				       FILE_ATTRIBUTE_NORMAL, 
				       NULL );

    if( env->h_device_handle == INVALID_HANDLE_VALUE )
    {
	  /* No good. try toggle write. */
	  dw_access_flags ^= GENERIC_WRITE;  
	  env->h_device_handle = CreateFile(	psz_win32_drive, 
											dw_access_flags, 
											FILE_SHARE_READ,  
											NULL, 
											OPEN_EXISTING, 
											FILE_ATTRIBUTE_NORMAL, 
											NULL );
	  if (env->h_device_handle == NULL)
		return false;
    }
#endif
    env->b_ioctl_init = true;
    return true;
  }
  return false;
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  via a SCSI MMC READ_TOC (FULTOC).  Return true if successful or
  false if an error.
*/
static bool
read_fulltoc_win32mmc (_img_private_t *p_env) 
{
  scsi_mmc_cdb_t  cdb = {{0, }};
  CDROM_TOC_FULL  cdrom_toc_full;
  int             i_status, i, j;
  int             i_track_format = 0;
  int             i_seen_flag;

  /* Operation code */
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_TOC);

  cdb.field[1] = 0x00;

  /* Format */
  cdb.field[2] = CDIO_MMC_READTOC_FMT_FULTOC;

  memset(&cdrom_toc_full, 0, sizeof(cdrom_toc_full));

  /* Setup to read header, to get length of data */
  CDIO_MMC_SET_READ_LENGTH16(cdb.field, sizeof(cdrom_toc_full));

  i_status = run_scsi_cmd_win32ioctl (p_env, 1000*60*3,
				scsi_mmc_get_cmd_len(cdb.field[0]), 
				&cdb, SCSI_MMC_DATA_READ, 
				sizeof(cdrom_toc_full), &cdrom_toc_full);

  if ( 0 != i_status ) {
    cdio_debug ("SCSI MMC READ_TOC failed\n");  
    return false;
  } 
    
  i_seen_flag=0;
  for( i = 0 ; i <= CDIO_CD_MAX_TRACKS+3; i++ ) {
    
    if ( 0xA0 == cdrom_toc_full.TrackData[i].POINT ) { 
      /* First track number */
      p_env->gen.i_first_track = cdrom_toc_full.TrackData[i].PMIN;
      i_track_format = cdrom_toc_full.TrackData[i].PSEC;
      i_seen_flag|=0x01;
    }
    
    if ( 0xA1 == cdrom_toc_full.TrackData[i].POINT ) { 
      /* Last track number */
      p_env->gen.i_tracks = 
	cdrom_toc_full.TrackData[i].PMIN - p_env->gen.i_first_track + 1;
      i_seen_flag|=0x02;
    }
    
    j = cdrom_toc_full.TrackData[i].POINT;
    if ( 0xA2 ==  j ) { 
      /* Start position of the lead out */
      p_env->tocent[ p_env->gen.i_tracks ].start_lsn = 
	cdio_msf3_to_lba(
			 cdrom_toc_full.TrackData[i].PMIN,
			 cdrom_toc_full.TrackData[i].PSEC,
			 cdrom_toc_full.TrackData[i].PFRAME );
      p_env->tocent[ p_env->gen.i_tracks ].Control 
	= cdrom_toc_full.TrackData[i].Control;
      p_env->tocent[ p_env->gen.i_tracks ].Format  = i_track_format;
      i_seen_flag|=0x04;
    }
    
    if (cdrom_toc_full.TrackData[i].POINT > 0 
	&& cdrom_toc_full.TrackData[i].POINT <= p_env->gen.i_tracks) {
      p_env->tocent[j-1].start_lsn = 
	cdio_msf3_to_lba(
			 cdrom_toc_full.TrackData[i].PMIN,
			 cdrom_toc_full.TrackData[i].PSEC,
			 cdrom_toc_full.TrackData[i].PFRAME );
      p_env->tocent[j-1].Control = 
	cdrom_toc_full.TrackData[i].Control;
      p_env->tocent[j-1].Format  = i_track_format;
      
      set_track_flags(&(p_env->gen.track_flags[j]), 
		      p_env->tocent[j-1].Control);
      
      cdio_debug("p_sectors: %i, %lu", i, 
		 (unsigned long int) (p_env->tocent[i].start_lsn));
      
      if (cdrom_toc_full.TrackData[i].POINT == p_env->gen.i_tracks)
	i_seen_flag|=0x08;
    }
    
    if ( 0x0F == i_seen_flag ) break;
  }
  if ( 0x0F == i_seen_flag ) {
    p_env->gen.toc_init = true; 
    return true;
  }
  return false;
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
bool
read_toc_win32ioctl (_img_private_t *p_env) 
{
  CDROM_TOC    cdrom_toc;
  DWORD        dwBytesReturned;
  unsigned int i, j;

  if ( ! p_env ) return false;

  /* XBOX: Thompson drives do not like mmc3 full toc reading */

  /* Read full TOC, (not supported on DVD media) */

 /* if ( read_fulltoc_win32mmc(p_env) ) return true; */

  /* SCSI-MMC READ_TOC (FULTOC) read failed.  Try reading TOC via
     DeviceIoControl instead */
  if( DeviceIoControl( p_env->h_device_handle,
		       IOCTL_CDROM_READ_TOC,
		       NULL, 0, &cdrom_toc, sizeof(CDROM_TOC),
		       &dwBytesReturned, NULL ) == 0 ) {
    char *psz_msg = NULL;
    long int i_err = GetLastError();
    FORMAT_ERROR(i_err, psz_msg);
    if (psz_msg) {
      cdio_warn("could not read TOC (%ld): %s", i_err, psz_msg);
      LocalFree(psz_msg);
    } else 
      cdio_warn("could not read TOC (%ld)", i_err);
    return false;
  }
  
  p_env->gen.i_first_track = cdrom_toc.FirstTrack;
  p_env->gen.i_tracks  = cdrom_toc.LastTrack - cdrom_toc.FirstTrack + 1;
  
  j = p_env->gen.i_first_track;
  for( i = 0 ; i <= p_env->gen.i_tracks ; i++, j++ ) {
    p_env->tocent[ i ].start_lsn = 
cdio_lba_to_lsn(cdio_msf3_to_lba( cdrom_toc.TrackData[i].Address[1],
					     cdrom_toc.TrackData[i].Address[2],
					     cdrom_toc.TrackData[i].Address[3] ));
    p_env->tocent[ i ].Control   = cdrom_toc.TrackData[i].Control;
    p_env->tocent[ i ].Format    = cdrom_toc.TrackData[i].Format;

    p_env->gen.track_flags[j].preemphasis = 
      p_env->tocent[i].Control & 0x1 
      ? CDIO_TRACK_FLAG_TRUE : CDIO_TRACK_FLAG_FALSE;

    p_env->gen.track_flags[j].copy_permit = 
      p_env->tocent[i].Control & 0x2 
      ? CDIO_TRACK_FLAG_TRUE : CDIO_TRACK_FLAG_FALSE;
    
    p_env->gen.track_flags[j].channels = 
      p_env->tocent[i].Control & 0x8 ? 4 : 2;
    

    cdio_debug("p_sectors: %i, %lu", i, 
	       (unsigned long int) (p_env->tocent[i].start_lsn));
  }
  p_env->gen.toc_init = true;
  return true;
}

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
char *
get_mcn_win32ioctl (const _img_private_t *env) {

  DWORD dwBytesReturned;
  SUB_Q_MEDIA_CATALOG_NUMBER mcn;
  CDROM_SUB_Q_DATA_FORMAT q_data_format;
  
  memset( &mcn, 0, sizeof(mcn) );
  
  q_data_format.Format = CDIO_SUBCHANNEL_MEDIA_CATALOG;

  q_data_format.Track=1;
  
  if( DeviceIoControl( env->h_device_handle,
		       IOCTL_CDROM_READ_Q_CHANNEL,
		       &q_data_format, sizeof(q_data_format), 
		       &mcn, sizeof(mcn),
		       &dwBytesReturned, NULL ) == 0 ) {
    cdio_warn( "could not read Q Channel at track %d", 1);
  } else if (mcn.Mcval) 
    return strdup(mcn.MediaCatalog);
  return NULL;
}

/*!  
  Get the format (XA, DATA, AUDIO) of a track. 
*/
track_format_t
get_track_format_win32ioctl(const _img_private_t *env, track_t i_track) 
{
  /* This is pretty much copied from the "badly broken" cdrom_count_tracks
     in linux/cdrom.c.
  */

  if (env->tocent[i_track - env->gen.i_first_track].Control & 0x04) {
    if (env->tocent[i_track - env->gen.i_first_track].Format == 0x10)
      return TRACK_FORMAT_CDI;
    else if (env->tocent[i_track - env->gen.i_first_track].Format == 0x20) 
      return TRACK_FORMAT_XA;
    else
      return TRACK_FORMAT_DATA;
  } else
    return TRACK_FORMAT_AUDIO;
}

#endif /*HAVE_WIN32_CDROM*/
