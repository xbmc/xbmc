
#include "stdafx.h"
#include "cdiosupport.h"
#include <xtl.h>
#include "../lib/libcdio/cdio.h"
#include "../lib/libcdio/logging.h"
#include "../xbox/Undocumented.h"
#include "../lib/libcdio/util.h"

using namespace MEDIA_DETECT;

/* Some interesting sector numbers stored in the above buffer. */
#define ISO_SUPERBLOCK_SECTOR  16  /* buffer[0] */
#define UFS_SUPERBLOCK_SECTOR   4  /* buffer[2] */
#define BOOT_SECTOR            17  /* buffer[3] */
#define VCD_INFO_SECTOR       150  /* buffer[4] */
#define UDFX_SECTOR			       32  /* buffer[4] */
#define UDF_ANCHOR_SECTOR			256  /* buffer[5] */


signature_t CCdIoSupport::sigs[] =
  {
/*buffer[x] off look for     description */
    {0,     1, "CD001\0",    "ISO 9660\0"}, 
    {0,     1, "CD-I",       "CD-I"}, 
    {0,     8, "CDTV",       "CDTV"}, 
    {0,     8, "CD-RTOS",    "CD-RTOS"}, 
    {0,     9, "CDROM",      "HIGH SIERRA"}, 
    {0,    16, "CD-BRIDGE",  "BRIDGE"}, 
    {0,  1024, "CD-XA001",   "XA"}, 
    {1,    64, "PPPPHHHHOOOOTTTTOOOO____CCCCDDDD",  "PHOTO CD"}, 
    {1, 0x438, "\x53\xef",   "EXT2 FS"}, 
    {2,  1372, "\x54\x19\x01\x0", "UFS"}, 
    {3,     7, "EL TORITO",  "BOOTABLE"}, 
    {4,     0, "VIDEO_CD",   "VIDEO CD"}, 
    {4,     0, "SUPERVCD",   "Chaoji VCD"}, 
    {0,     0, "MICROSOFT*XBOX*MEDIA", "UDFX CD"},
    {0,     1, "BEA01",      "UDF"}, 
    { 0 }
  };

#undef DEBUG_CDIO

static void
xbox_cdio_log_handler (cdio_log_level_t level, const char message[])
{
#ifdef DEBUG_CDIO
	char buf[1024];
  switch (level)
	{
    case CDIO_LOG_ERROR:
      sprintf (buf, "**ERROR: %s\n", message);
      OutputDebugString( buf );
      break;
    case CDIO_LOG_DEBUG:
      sprintf (buf, "--DEBUG: %s\n", message);
      OutputDebugString( buf );
      break;
    case CDIO_LOG_WARN:
      sprintf (buf, "++ WARN: %s\n", message);
      OutputDebugString( buf );
      break;
    case CDIO_LOG_INFO:
      sprintf (buf, "   INFO: %s\n", message);
      OutputDebugString( buf );
     break;
    case CDIO_LOG_ASSERT:
      sprintf (buf, "!ASSERT: %s\n", message);
      OutputDebugString( buf );
      break;
    default:
      //cdio_assert_not_reached ();
      break;
	}
#endif
}

CCdIoSupport::CCdIoSupport()
{
	cdio_log_set_handler( xbox_cdio_log_handler );
	m_nFirstData = -1;        /* # of first data track */
	m_nNumData = 0;                /* # of data tracks */
	m_nFirstAudio = -1;      /* # of first audio track */
	m_nNumAudio = 0;              /* # of audio tracks */
	m_nIsofsSize = 0;               /* size of session */
	m_nJolietLevel = 0;
	m_nFs=0;

}

CCdIoSupport::~CCdIoSupport()
{

}


HRESULT CCdIoSupport::EjectTray()
{
	return E_FAIL;
}

HRESULT CCdIoSupport::CloseTray()
{
	return E_FAIL;
}

HANDLE CCdIoSupport::OpenCDROM()
{
	char* source_name = "\\\\.\\D:";
    CdIo* cdio = cdio_open (source_name, DRIVER_UNKNOWN);

	return (HANDLE) cdio;
}

HANDLE CCdIoSupport::OpenIMAGE( CStdString& strFilename )
{
    CdIo* cdio = cdio_open (strFilename, DRIVER_UNKNOWN);

	return (HANDLE) cdio;
}

INT CCdIoSupport::ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
	CdIo* cdio = (CdIo*) hDevice;
	if ( cdio == NULL )
		return -1;

	if ( cdio_read_mode2_sector( cdio, lpczBuffer, dwSector, false ) == 0 )
		return dwSector;

	return -1;
}

INT CCdIoSupport::ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
	CdIo* cdio = (CdIo*) hDevice;
	if ( cdio == NULL )
		return -1;

	if ( cdio_read_mode2_sector( cdio, lpczBuffer, dwSector, false ) == 0 )
		return dwSector;

	return -1;
}

INT CCdIoSupport::ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
	CdIo* cdio = (CdIo*) hDevice;
	if ( cdio == NULL )
		return -1;

	if ( cdio_read_audio_sector( cdio, lpczBuffer, dwSector ) == 0 )
		return dwSector;

	return -1;
}

VOID CCdIoSupport::CloseCDROM(HANDLE hDevice)
{
	CdIo* cdio = (CdIo*) hDevice;

	if ( cdio == NULL )
		return;

	cdio_destroy( cdio );
}

#ifdef _DEBUG

void CCdIoSupport::PrintAnalysis(int fs, int num_audio)
{
  char buf[1024];
  
  switch(fs & FS_MASK) 
	{
  case FS_UDF:
    sprintf(buf, "CD-ROM with UDF filesystem");
		OutputDebugString( buf );
    break;
  case FS_NO_DATA:
    sprintf(buf, "CDDA Disk");
		OutputDebugString( buf );
    break;
  case FS_ISO_9660:
    sprintf(buf, "CD-ROM with ISO 9660 filesystem");
	  OutputDebugString( buf );
    if (fs & JOLIET) 
		{
      sprintf(buf, " with joliet extension level %d", m_nJolietLevel);
 			OutputDebugString( buf );
		}
		if (fs & ROCKRIDGE) 
		{
			sprintf(buf, " and rockridge extensions");
 			OutputDebugString( buf );
		}
    sprintf(buf, "\n");
 		OutputDebugString( buf );
    break;
  case FS_ISO_9660_INTERACTIVE:
    sprintf(buf, "CD-ROM with CD-RTOS and ISO 9660 filesystem\n");
 		OutputDebugString( buf );
    break;
  case FS_HIGH_SIERRA:
    sprintf(buf, "CD-ROM with High Sierra filesystem\n");
 		OutputDebugString( buf );
    break;
  case FS_INTERACTIVE:
    sprintf(buf, "CD-Interactive%s\n", num_audio > 0 ? "/Ready" : "");
 		OutputDebugString( buf );
    break;
  case FS_HFS:
    sprintf(buf, "CD-ROM with Macintosh HFS\n");
 		OutputDebugString( buf );
    break;
  case FS_ISO_HFS:
    sprintf(buf, "CD-ROM with both Macintosh HFS and ISO 9660 filesystem\n");
 		OutputDebugString( buf );
    break;
  case FS_UFS:
    sprintf(buf, "CD-ROM with Unix UFS\n");
 		OutputDebugString( buf );
    break;
  case FS_EXT2:
    sprintf(buf, "CD-ROM with Linux second extended filesystem\n");
 		OutputDebugString( buf );
	  break;
  case FS_3DO:
    sprintf(buf, "CD-ROM with Panasonic 3DO filesystem\n");
 		OutputDebugString( buf );
    break;
  case FS_UDFX:
    sprintf(buf, "CD-ROM with UDFX filesystem\n");
 		OutputDebugString( buf );
    break;
  case FS_UNKNOWN:
    sprintf(buf, "CD-ROM with unknown filesystem\n");
 		OutputDebugString( buf );
    break;
  }

  switch(fs & FS_MASK) 
	{
  case FS_ISO_9660:
  case FS_ISO_9660_INTERACTIVE:
  case FS_ISO_HFS:
    sprintf(buf, "ISO 9660: %i blocks, label `%.32s'\n",
	  m_nIsofsSize, buffer[0]+40);
		OutputDebugString( buf );
    break;
  }

  if (m_nFirstData == 1 && num_audio > 0) 
	{
    sprintf(buf, "mixed mode CD   ");
		OutputDebugString( buf );
  }
  if (fs & XA) 
	{
    sprintf(buf, "XA sectors   ");
		OutputDebugString( buf );
  }
  if (fs & MULTISESSION) 
	{
    sprintf(buf, "Multisession, offset = %i   ",m_nMsOffset);
		OutputDebugString( buf );
  }
  if (fs & HIDDEN_TRACK) 
	{
    sprintf(buf, "Hidden Track   ");
		OutputDebugString( buf );
  }
  if (fs & PHOTO_CD) 
	{
    sprintf(buf, "%sPhoto CD   ", num_audio > 0 ? " Portfolio " : "");
		OutputDebugString( buf );
  }
  if (fs & CDTV) 
	{
    sprintf(buf, "Commodore CDTV   ");
		OutputDebugString( buf );
  }
  if (m_nFirstData > 1) 
	{
    sprintf(buf, "CD-Plus/Extra   ");
		OutputDebugString( buf );
  }
  if (fs & BOOTABLE) 
	{
    sprintf(buf, "bootable CD   ");
		OutputDebugString( buf );
  }
  if (fs & VIDEOCDI && num_audio == 0) 
	{
    sprintf(buf, "Video CD   ");
		OutputDebugString( buf );
#ifdef HAVE_VCDINFO
    if (!opts.no_vcd) 
		{
      printf("\n");
      print_vcd_info();
    }
#endif    
  }
  if (fs & CVD) 
	{
    sprintf(buf, "Chaoji Video CD");
		OutputDebugString( buf );
  }
  OutputDebugString( "\n" );
}
#endif

int CCdIoSupport::ReadBlock(int superblock, uint32_t offset, uint8_t bufnum, track_t track_num)
{
  unsigned int track_sec_count = cdio_get_track_sec_count(cdio, track_num);
  memset(buffer[bufnum], 0, CDIO_CD_FRAMESIZE);

  if ( track_sec_count < (UINT)superblock) 
	{
    return -1;
  }
  
  if (cdio_get_track_green(cdio,  track_num)) 
	{
    if (0 > cdio_read_mode2_sector(cdio, buffer[bufnum], 
				   offset+superblock, false))
      return -1;
  } 
	else 
	{
    if (0 > cdio_read_yellow_sector(cdio, buffer[bufnum], 
				    offset+superblock, false))
      return -1;
  }

  return 0;
}

bool CCdIoSupport::IsIt(int num) 
{
  signature_t *sigp=&sigs[num];
  int len=strlen(sigp->sig_str);

  /* TODO: check that num < largest sig. */
  return 0 == memcmp(&buffer[sigp->buf_num][sigp->offset], sigp->sig_str, len);
}

int CCdIoSupport::IsHFS(void)
{
  return (0 == memcmp(&buffer[1][512],"PM",2)) ||
    (0 == memcmp(&buffer[1][512],"TS",2)) ||
    (0 == memcmp(&buffer[1][1024], "BD",2));
}

int CCdIoSupport::Is3DO(void)
{
  return (0 == memcmp(&buffer[1][0],"\x01\x5a\x5a\x5a\x5a\x5a\x01", 7)) &&
    (0 == memcmp(&buffer[1][40], "CD-ROM", 6));
}

int CCdIoSupport::IsJoliet(void)
{
  return 2 == buffer[3][0] && buffer[3][88] == 0x25 && buffer[3][89] == 0x2f;
}

int CCdIoSupport::IsUDF(void)
{
	return 2 == ((uint16_t)buffer[5][0] | ((uint16_t)buffer[5][1] << 8));
}

/* ISO 9660 volume space in M2F1_SECTOR_SIZE byte units */
int CCdIoSupport::GetSize(void)
{
  return ((buffer[0][80] & 0xff) |
	  ((buffer[0][81] & 0xff) << 8) |
	  ((buffer[0][82] & 0xff) << 16) |
	  ((buffer[0][83] & 0xff) << 24));
}

int CCdIoSupport::GetJolietLevel( void )
{
  switch (buffer[3][90]) 
	{
  case 0x40: 
		return 1;
  case 0x43: 
		return 2;
  case 0x45: 
		return 3;
  }
  return 0;
}

#define is_it_dbg(sig) /*\
    if (is_it(sig)) printf("%s, ", sigs[sig].description)*/

int CCdIoSupport::GuessFilesystem(int start_session, track_t track_num)
{
  int ret = 0;
  
  if (ReadBlock(UDFX_SECTOR, start_session, 0, track_num) < 0)
    return FS_UNKNOWN;

  if ( IsIt(IS_UDFX) )
	  return FS_UDFX;

  if (ReadBlock(ISO_SUPERBLOCK_SECTOR, start_session, 0, track_num) < 0)
    return FS_UNKNOWN;

	if (IsIt(IS_UDF))
	{
		if (ReadBlock(32, start_session, 0, track_num) < 0)
			return FS_UDF;
		m_strDiscLabel=buffer[0]+25;
		return FS_UDF;
	}
	/* filesystem */
  if (IsIt(IS_CD_I) && IsIt(IS_CD_RTOS) && !IsIt(IS_BRIDGE) && !IsIt(IS_XA)) 
	{
    return FS_INTERACTIVE;
  } 
	else 
	{	/* read sector 0 ONLY, when NO greenbook CD-I !!!! */

    if (ReadBlock(0, start_session, 1, track_num) < 0)
      return ret;
        
    if (IsIt(IS_HS))
      ret |= FS_HIGH_SIERRA;
    else if (IsIt(IS_ISOFS)) 
		{
			if (IsIt(IS_CD_RTOS) && IsIt(IS_BRIDGE))
				ret = FS_ISO_9660_INTERACTIVE;
      else if (IsHFS())
				ret = FS_ISO_HFS;
      else
				ret = FS_ISO_9660;

      m_nIsofsSize = GetSize();
			m_strDiscLabel=buffer[0]+40;
			if (ReadBlock(UDF_ANCHOR_SECTOR, start_session, 5, track_num) < 0)
				return ret;

			//	Maybe there is an UDF anchor in iso session
			//	so its ISO/UDF session and we prefere UDF
			if ( IsUDF() )
			{
				if (ReadBlock(32, start_session, 0, track_num) < 0)
					return FS_UDF;
				m_strDiscLabel=buffer[0]+25;
				return FS_UDF;
			}
#if 0
      if (IsRockridge())
				ret |= ROCKRIDGE;
#endif

      if (ReadBlock(BOOT_SECTOR, start_session, 3, track_num) < 0)
				return ret;
            
      if (IsJoliet()) 
			{
				m_nJolietLevel = GetJolietLevel();
				ret |= JOLIET;
      }
      if (IsIt(IS_BOOTABLE))
				ret |= BOOTABLE;
      
      if (IsIt(IS_XA) && IsIt(IS_ISOFS) && !IsIt(IS_PHOTO_CD)) 
			{
        if (ReadBlock(VCD_INFO_SECTOR, start_session, 4, track_num) < 0)
					return ret;
	
				if (IsIt(IS_BRIDGE) && IsIt(IS_CD_RTOS)) 
				{	  
					if (IsIt(IS_VIDEO_CD)) 
						ret |= VIDEOCDI;
				} 
				else 
				{	  
					if (IsIt(IS_CVD)) 
						ret |= CVD;
				}
      }
    } 
    else if (IsHFS())       
			ret |= FS_HFS;
    else if (IsIt(IS_EXT2)) 
			ret |= FS_EXT2;
    else if (Is3DO())       
			ret |= FS_3DO;
    else 
		{
      if (ReadBlock(UFS_SUPERBLOCK_SECTOR, start_session, 2, track_num) < 0)
				return ret;
            
      if (IsIt(IS_UFS)) 
				ret |= FS_UFS;
      else
				ret |= FS_UNKNOWN;
    }
  }
  
  /* other checks */
  if (IsIt(IS_XA))       ret |= XA;
  if (IsIt(IS_PHOTO_CD)) ret |= PHOTO_CD;
  if (IsIt(IS_CDTV))     ret |= CDTV;

  return ret;
}
CCdInfo* CCdIoSupport::GetCdInfo()
{
	char* source_name = "\\\\.\\D:";
	cdio = cdio_open (source_name, DRIVER_UNKNOWN);
	if (cdio==NULL) 
	{
		char buf[1024];
		sprintf(buf, "%s: Error in automatically selecting driver with input\n", 
			NULL);
		OutputDebugString( buf );
		return NULL;
	} 

	CCdInfo* info = new CCdInfo;
	m_nFirstTrackNum = cdio_get_first_track_num(cdio);
	m_nNumTracks      = cdio_get_num_tracks(cdio);
	info->SetFirstTrack( m_nFirstTrackNum );
	info->SetTrackCount( m_nNumTracks );

	for (i = m_nFirstTrackNum; i <= CDIO_CDROM_LEADOUT_TRACK; i++) 
	{
		msf_t msf;
		if (!cdio_get_track_msf(cdio, i, &msf)) 
		{
			char buf[1024];
			trackinfo ti;
			ti.nfsInfo = FS_UNKNOWN;
			ti.ms_offset = 0;
			ti.isofs_size = 0;
			ti.nJolietLevel = 0;
			ti.nFrames = 0;
			info->SetTrackInformation( i, ti );
			sprintf( buf, "cdio_track_msf for track %i failed, I give up.\n", i);
			OutputDebugString( buf );
			return NULL;
		}

		trackinfo ti;
		if (TRACK_FORMAT_AUDIO == cdio_get_track_format(cdio, i)) 
		{
			m_nNumAudio++;
			ti.nfsInfo = FS_NO_DATA;
			int temp1 = cdio_get_track_lba(cdio, i) - CDIO_PREGAP_SECTORS;
			int temp2 = cdio_get_track_lba(cdio, i+1) - CDIO_PREGAP_SECTORS;
			// the length is the address of the second track minus the address of the first track
			temp2 -= temp1;    // temp2 now has length of track1 in frames
			ti.nMins = temp2 / (60 * 75);    // calculate the number of minutes
			temp2 %= 60 * 75;    // calculate the left-over frames
			ti.nSecs = temp2 / 75;    // calculate the number of seconds
			if (-1 == m_nFirstAudio)
				m_nFirstAudio = i;
		} 
		else 
		{
			m_nNumData++;
			if (-1 == m_nFirstData)
				m_nFirstData = i;
		}
		ti.ms_offset = 0;
		ti.isofs_size = 0;
		ti.nJolietLevel = 0;
		ti.nFrames = cdio_get_track_lba(cdio, i);
		info->SetTrackInformation( i, ti );
		/* skip to leadout? */
		if (i == m_nNumTracks) 
			i = CDIO_CDROM_LEADOUT_TRACK-1;
	}

	info->SetCddbDiscId( CddbDiscId() );
	info->SetDiscLength( cdio_get_track_lba(cdio, CDIO_CDROM_LEADOUT_TRACK) / CDIO_CD_FRAMES_PER_SEC );
	
	info->SetAudioTrackCount( m_nNumAudio );
	info->SetDataTrackCount( m_nNumData-1 ); //	Do not count leadout track
	info->SetFirstAudioTrack( m_nFirstAudio );
	info->SetFirstDataTrack( m_nFirstData );

	char buf[1024];
	sprintf(buf, STRONG "CD Analysis Report\n" NORMAL);
	OutputDebugString( buf );
    
    /* try to find out what sort of CD we have */
	if (0 == m_nNumData) 
	{
		/* no data track, may be a "real" audio CD or hidden track CD */

		msf_t msf;
		cdio_get_track_msf(cdio, 1, &msf);
		m_nStartTrack = cdio_msf_to_lsn(&msf);

		/* CD-I/Ready says start_track <= 30*75 then CDDA */
		if (m_nStartTrack > 100 /* 100 is just a guess */) 
		{
			m_nFs = GuessFilesystem(0, 1);
			if ((m_nFs & FS_MASK) != FS_UNKNOWN)
				m_nFs |= HIDDEN_TRACK;
			else 
			{
				m_nFs &= ~FS_MASK; /* del filesystem info */
				sprintf(buf, "Oops: %i unused sectors at start, "
					"but hidden track check failed.\n",
					m_nStartTrack);
				OutputDebugString( buf );
			}
		}
	} 
	else 
	{
	/* we have data track(s) */
		for (j = 2, i = m_nFirstData; i <= m_nNumTracks; i++) 
		{
			msf_t msf;
			track_format_t track_format = cdio_get_track_format(cdio, i);

			cdio_get_track_msf(cdio, i, &msf);

			switch ( track_format ) 
			{
				case TRACK_FORMAT_AUDIO:
					trackinfo ti;
					ti.nfsInfo = FS_NO_DATA;
					ti.ms_offset = 0;
					ti.isofs_size = 0;
					ti.nJolietLevel = 0;
					ti.nFrames = cdio_get_track_lba(cdio, i);
					info->SetTrackInformation( i + 1, ti );
				case TRACK_FORMAT_ERROR:
					break;
				case TRACK_FORMAT_CDI:
				case TRACK_FORMAT_XA:
				case TRACK_FORMAT_DATA: 
				case TRACK_FORMAT_PSX: 
					break;
			}
		
			m_nStartTrack = (i == 1) ? 0 : cdio_msf_to_lsn(&msf);
			
			/* save the start of the data area */
			if (i == m_nFirstData) 
				m_nDataStart = m_nStartTrack;

			/* skip tracks which belong to the current walked session */
			if (m_nStartTrack < m_nDataStart + m_nIsofsSize)
				continue;
			
			m_nFs = GuessFilesystem(m_nStartTrack, i);
			trackinfo ti;
			ti.nfsInfo = m_nFs;
			ti.ms_offset = m_nMsOffset;
			ti.isofs_size = m_nIsofsSize;
			ti.nJolietLevel = m_nJolietLevel;
			ti.nFrames = cdio_get_track_lba(cdio, i);
			info->SetDiscLabel(m_strDiscLabel);


			if (i > 1) 
			{
				/* track is beyond last session -> new session found */
				m_nMsOffset = m_nStartTrack;

				sprintf(buf, "session #%d starts at track %2i, LSN: %6i,"
							" ISO 9660 blocks: %6i\n",
							j++, i, m_nStartTrack, m_nIsofsSize);
				OutputDebugString( buf );

				sprintf(buf, "ISO 9660: %i blocks, label `%.32s'\n",
							m_nIsofsSize, buffer[0]+40);
				OutputDebugString( buf );
				m_nFs |= MULTISESSION;
				ti.nfsInfo = m_nFs;
			} 
#ifdef _DEBUG
			else 
			{
				PrintAnalysis(m_nFs, m_nNumAudio);
			}
#endif

			info->SetTrackInformation( i, ti );
			
			//	xbox does not support multisession cd's
			if (!(((m_nFs & FS_MASK) == FS_ISO_9660 ||
				(m_nFs & FS_MASK) == FS_ISO_HFS ||
				/* (fs & FS_MASK) == FS_ISO_9660_INTERACTIVE) && (fs & XA))) */
				(m_nFs & FS_MASK) == FS_ISO_9660_INTERACTIVE)))
				break;	/* no method for non-iso9660 multisessions */
		}
	}
	cdio_destroy( cdio );
	return info;
}

 
//	Returns the sum of the decimal digits in a number. Eg. 1955 = 20
int CCdIoSupport::CddbDecDigitSum(int n)
{
  int ret=0;
  
  for (;;) 
	{
    ret += n%10;
    n    = n/10;
    if (!n)
      return ret;
  }
}

//	Return the number of seconds (discarding frame portion) of an MSF
UINT CCdIoSupport::MsfSeconds(msf_t *msf) 
{
  return from_bcd8(msf->m)*60 + from_bcd8(msf->s);
}


//	Compute the CDDB disk ID for an Audio disk.  This is a funny checksum
//	consisting of the concatenation of 3 things:
//	   the sum of the decimal digits of sizes of all tracks, 
//	   the total length of the disk, and 
//	   the number of tracks.

ULONG CCdIoSupport::CddbDiscId()
{
  int i,t,n=0;
  msf_t start_msf;
  msf_t msf;
  
  for (i = 1; i <= m_nNumTracks; i++) 
	{
    cdio_get_track_msf(cdio, i, &msf);
    n += CddbDecDigitSum(MsfSeconds(&msf));
  }

  cdio_get_track_msf(cdio, 1, &start_msf);
  cdio_get_track_msf(cdio, CDIO_CDROM_LEADOUT_TRACK, &msf);
  
  t = MsfSeconds(&msf) - MsfSeconds(&start_msf);
  
  return ((n % 0xff) << 24 | t << 8 | m_nNumTracks);
}
