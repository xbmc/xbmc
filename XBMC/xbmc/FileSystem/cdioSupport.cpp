#include "stdafx.h"
#include "cdioSupport.h"
#include "../lib/libcdio/cdio.h"
#include "../lib/libcdio/logging.h"
#include "../lib/libcdio/Util.h"


using namespace MEDIA_DETECT;

/* Some interesting sector numbers stored in the above buffer. */
#define ISO_SUPERBLOCK_SECTOR  16  /* buffer[0] */
#define UFS_SUPERBLOCK_SECTOR   4  /* buffer[2] */
#define BOOT_SECTOR            17  /* buffer[3] */
#define VCD_INFO_SECTOR       150  /* buffer[4] */
#define UDFX_SECTOR          32  /* buffer[4] */
#define UDF_ANCHOR_SECTOR   256  /* buffer[5] */


signature_t CCdIoSupport::sigs[] =
  {
    /*buffer[x] off look for     description */
    {0, 1, "CD001\0", "ISO 9660\0"},
    {0, 1, "CD-I", "CD-I"},
    {0, 8, "CDTV", "CDTV"},
    {0, 8, "CD-RTOS", "CD-RTOS"},
    {0, 9, "CDROM", "HIGH SIERRA"},
    {0, 16, "CD-BRIDGE", "BRIDGE"},
    {0, 1024, "CD-XA001", "XA"},
    {1, 64, "PPPPHHHHOOOOTTTTOOOO____CCCCDDDD", "PHOTO CD"},
    {1, 0x438, "\x53\xef", "EXT2 FS"},
    {2, 1372, "\x54\x19\x01\x0", "UFS"},
    {3, 7, "EL TORITO", "BOOTABLE"},
    {4, 0, "VIDEO_CD", "VIDEO CD"},
    {4, 0, "SUPERVCD", "Chaoji VCD"},
    {0, 0, "MICROSOFT*XBOX*MEDIA", "UDFX CD"},
    {0, 1, "BEA01", "UDF"},
    { 0 }
  };

#undef DEBUG_CDIO

static void
xbox_cdio_log_handler (cdio_log_level_t level, const char message[])
{
#ifdef DEBUG_CDIO
  switch (level)
  {
  case CDIO_LOG_ERROR:
    CLog::DebugLog("**ERROR: %s", message);
    break;
  case CDIO_LOG_DEBUG:
    CLog::DebugLog("--DEBUG: %s", message);
    break;
  case CDIO_LOG_WARN:
    CLog::DebugLog("++ WARN: %s", message);
    break;
  case CDIO_LOG_INFO:
    CLog::DebugLog("   INFO: %s", message);
    break;
  case CDIO_LOG_ASSERT:
    CLog::DebugLog("!ASSERT: %s", message);
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
  m_nFs = 0;
  m_nUDFVerMinor = 0;
  m_nUDFVerMajor = 0;

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

  if ( cdio_read_mode1_sector( cdio, lpczBuffer, dwSector, false ) == 0 )
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
    return ;

  cdio_destroy( cdio );
}

void CCdIoSupport::PrintAnalysis(int fs, int num_audio)
{
  switch (fs & FS_MASK)
  {
  case FS_UDF:
    CLog::Log(LOGINFO, "CD-ROM with UDF filesystem");
    break;
  case FS_NO_DATA:
    CLog::Log(LOGINFO, "CD-ROM with audio tracks");
    break;
  case FS_ISO_9660:
    CLog::Log(LOGINFO, "CD-ROM with ISO 9660 filesystem");
    if (fs & JOLIET)
    {
      CLog::Log(LOGINFO, " with joliet extension level %d", m_nJolietLevel);
    }
    if (fs & ROCKRIDGE)
    {
      CLog::Log(LOGINFO, " and rockridge extensions");
    }
    break;
  case FS_ISO_9660_INTERACTIVE:
    CLog::Log(LOGINFO, "CD-ROM with CD-RTOS and ISO 9660 filesystem");
    break;
  case FS_HIGH_SIERRA:
    CLog::Log(LOGINFO, "CD-ROM with High Sierra filesystem");
    break;
  case FS_INTERACTIVE:
    CLog::Log(LOGINFO, "CD-Interactive%s", num_audio > 0 ? "/Ready" : "");
    break;
  case FS_HFS:
    CLog::Log(LOGINFO, "CD-ROM with Macintosh HFS");
    break;
  case FS_ISO_HFS:
    CLog::Log(LOGINFO, "CD-ROM with both Macintosh HFS and ISO 9660 filesystem");
    break;
  case FS_ISO_UDF:
    CLog::Log(LOGINFO, "CD-ROM with both UDF and ISO 9660 filesystem");
    break;
  case FS_UFS:
    CLog::Log(LOGINFO, "CD-ROM with Unix UFS");
    break;
  case FS_EXT2:
    CLog::Log(LOGINFO, "CD-ROM with Linux second extended filesystem");
    break;
  case FS_3DO:
    CLog::Log(LOGINFO, "CD-ROM with Panasonic 3DO filesystem");
    break;
  case FS_UDFX:
    CLog::Log(LOGINFO, "CD-ROM with UDFX filesystem");
    break;
  case FS_UNKNOWN:
    CLog::Log(LOGINFO, "CD-ROM with unknown filesystem");
    break;
  }

  switch (fs & FS_MASK)
  {
  case FS_ISO_9660:
  case FS_ISO_9660_INTERACTIVE:
  case FS_ISO_HFS:
  case FS_ISO_UDF:
    CLog::Log(LOGINFO, "ISO 9660: %i blocks, label `%.32s'\n",
              m_nIsofsSize, buffer[0] + 40);
    break;
  }

  switch (fs & FS_MASK)
  {
  case FS_UDF:
  case FS_ISO_UDF:
    CLog::Log(LOGINFO, "UDF: version %x.%02.2x\n",
              m_nUDFVerMajor, m_nUDFVerMinor);
    break;
  }

  if (m_nFirstData == 1 && num_audio > 0)
  {
    CLog::Log(LOGINFO, "mixed mode CD   ");
  }
  if (fs & XA)
  {
    CLog::Log(LOGINFO, "XA sectors   ");
  }
  if (fs & MULTISESSION)
  {
    CLog::Log(LOGINFO, "Multisession, offset = %i   ", m_nMsOffset);
  }
  if (fs & HIDDEN_TRACK)
  {
    CLog::Log(LOGINFO, "Hidden Track   ");
  }
  if (fs & PHOTO_CD)
  {
    CLog::Log(LOGINFO, "%sPhoto CD   ", num_audio > 0 ? " Portfolio " : "");
  }
  if (fs & CDTV)
  {
    CLog::Log(LOGINFO, "Commodore CDTV   ");
  }
  if (m_nFirstData > 1)
  {
    CLog::Log(LOGINFO, "CD-Plus/Extra   ");
  }
  if (fs & BOOTABLE)
  {
    CLog::Log(LOGINFO, "bootable CD   ");
  }
  if (fs & VIDEOCDI && num_audio == 0)
  {
    CLog::Log(LOGINFO, "Video CD   ");
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
    CLog::Log(LOGINFO, "Chaoji Video CD");
  }
}

int CCdIoSupport::ReadBlock(int superblock, uint32_t offset, uint8_t bufnum, track_t track_num)
{
  unsigned int track_sec_count = cdio_get_track_sec_count(cdio, track_num);
  memset(buffer[bufnum], 0, CDIO_CD_FRAMESIZE);

  if ( track_sec_count < (UINT)superblock)
  {
    cdio_debug("reading block %u skipped track %d has only %u sectors\n",
               superblock, track_num, track_sec_count);
    return -1;
  }

  cdio_debug("about to read sector %lu\n",
             (long unsigned int) offset + superblock);

  if (cdio_get_track_green(cdio, track_num))
  {
    if (0 < cdio_read_mode2_sector(cdio, buffer[bufnum],
                                   offset + superblock, false))
      return -1;
  }
  else
  {
    if (0 < cdio_read_mode1_sector(cdio, buffer[bufnum],
                                   offset + superblock, false))
      return -1;
  }

  return 0;
}

bool CCdIoSupport::IsIt(int num)
{
  signature_t *sigp = &sigs[num];
  int len = strlen(sigp->sig_str);

  /* TODO: check that num < largest sig. */
  return 0 == memcmp(&buffer[sigp->buf_num][sigp->offset], sigp->sig_str, len);
}

int CCdIoSupport::IsHFS(void)
{
  return (0 == memcmp(&buffer[1][512], "PM", 2)) ||
         (0 == memcmp(&buffer[1][512], "TS", 2)) ||
         (0 == memcmp(&buffer[1][1024], "BD", 2));
}

int CCdIoSupport::Is3DO(void)
{
  return (0 == memcmp(&buffer[1][0], "\x01\x5a\x5a\x5a\x5a\x5a\x01", 7)) &&
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
  int ret = FS_UNKNOWN;

  if (ReadBlock(UDFX_SECTOR, start_session, 0, track_num) < 0)
    return ret;

  if ( IsIt(IS_UDFX) )
  {
    return FS_UDFX;
  }

  if (ReadBlock(ISO_SUPERBLOCK_SECTOR, start_session, 0, track_num) < 0)
    return ret;

  if (IsIt(IS_UDF))
  {
    // Detect UDF version
    // Test if we have a valid version of UDF the xbox can read nativly
    if (ReadBlock(35, start_session, 5, track_num) < 0)
      return FS_UNKNOWN;

    m_nUDFVerMinor = (int)buffer[5][240];
    m_nUDFVerMajor = (int)buffer[5][241];
    // Read disc label
    if (ReadBlock(32, start_session, 5, track_num) < 0)
      return FS_UDF;
    m_strDiscLabel = buffer[5] + 25;
    return FS_UDF;
  }
  /* filesystem */
  if (IsIt(IS_CD_I) && IsIt(IS_CD_RTOS) && !IsIt(IS_BRIDGE) && !IsIt(IS_XA))
  {
    return FS_INTERACTIVE;
  }
  else
  { /* read sector 0 ONLY, when NO greenbook CD-I !!!! */

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
      m_strDiscLabel = buffer[0] + 40;
      if (ReadBlock(UDF_ANCHOR_SECTOR, start_session, 5, track_num) < 0)
        return ret;

      // Maybe there is an UDF anchor in iso session
      // so its ISO/UDF session and we prefere UDF
      if ( IsUDF() )
      {
        // Detect UDF version
        // Test if we have a valid version of UDF the xbox can read nativly
        if (ReadBlock(35, start_session, 5, track_num) < 0)
          return ret;

        m_nUDFVerMinor = (int)buffer[5][240];
        m_nUDFVerMajor = (int)buffer[5][241];
        // Read disc label
        if (ReadBlock(32, start_session, 5, track_num) < 0)
          return ret;
        // we are using ISO/UDF cd's as iso,
        // no need to get UDF disc label
        //m_strDiscLabel=buffer[5]+25;
        ret = FS_ISO_UDF;
      }
      /*
            if (IsRockridge())
          ret |= ROCKRIDGE;
      */

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
  if (IsIt(IS_XA)) ret |= XA;
  if (IsIt(IS_PHOTO_CD)) ret |= PHOTO_CD;
  if (IsIt(IS_CDTV)) ret |= CDTV;

  return ret;
}

void CCdIoSupport::GetCdTextInfo(trackinfo *pti, int trackNum)
{
  // Get the CD-Text , if any
  cdtext_t *pcdtext = (cdtext_t *)cdio_get_cdtext (cdio, trackNum);

  cdtext_init(&pti->cdtext);

  if (pcdtext == NULL)
    return ;

  for (int i = 0; i < MAX_CDTEXT_FIELDS; i++)
  {
    if (pcdtext->field[i])
      pti->cdtext.field[i] = strdup(pcdtext->field[i]);
  }
}

CCdInfo* CCdIoSupport::GetCdInfo()
{
  char* source_name = "\\\\.\\D:";
  cdio = cdio_open (source_name, DRIVER_UNKNOWN);
  if (cdio == NULL)
  {
    char buf[1024];
    sprintf(buf, "%s: Error in automatically selecting driver with input\n",
            NULL);
    OutputDebugString( buf );
    return NULL;
  }

  m_nFirstTrackNum = cdio_get_first_track_num(cdio);
  if (m_nFirstTrackNum == CDIO_INVALID_TRACK)
  {
    cdio_destroy(cdio);
    return NULL;
  }

  m_nNumTracks = cdio_get_num_tracks(cdio);
  if (m_nNumTracks == CDIO_INVALID_TRACK)
  {
    cdio_destroy(cdio);
    return NULL;
  }

  CCdInfo* info = new CCdInfo;
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
      cdtext_init(&ti.cdtext);
      info->SetTrackInformation( i, ti );
      sprintf( buf, "cdio_track_msf for track %i failed, I give up.\n", i);
      OutputDebugString( buf );
      delete info;
      cdio_destroy(cdio);
      return NULL;
    }

    trackinfo ti_0, ti;
    cdtext_init(&ti_0.cdtext);
    cdtext_init(&ti.cdtext);
    if (TRACK_FORMAT_AUDIO == cdio_get_track_format(cdio, i))
    {
      m_nNumAudio++;
      ti.nfsInfo = FS_NO_DATA;
      m_nFs = FS_NO_DATA;
      int temp1 = cdio_get_track_lba(cdio, i) - CDIO_PREGAP_SECTORS;
      int temp2 = cdio_get_track_lba(cdio, i + 1) - CDIO_PREGAP_SECTORS;
      // the length is the address of the second track minus the address of the first track
      temp2 -= temp1;    // temp2 now has length of track1 in frames
      ti.nMins = temp2 / (60 * 75);    // calculate the number of minutes
      temp2 %= 60 * 75;    // calculate the left-over frames
      ti.nSecs = temp2 / 75;    // calculate the number of seconds
      if ( -1 == m_nFirstAudio)
        m_nFirstAudio = i;

      // Make sure that we have the Disc related info available
      if (i == 1)
      {
        GetCdTextInfo(&ti_0, 0);
        info->SetDiscCDTextInformation( ti_0.cdtext );
      }

      // Get this tracks info
      GetCdTextInfo(&ti, i);
    }
    else
    {
      m_nNumData++;
      if ( -1 == m_nFirstData)
        m_nFirstData = i;
    }
    ti.ms_offset = 0;
    ti.isofs_size = 0;
    ti.nJolietLevel = 0;
    ti.nFrames = cdio_get_track_lba(cdio, i);
    info->SetTrackInformation( i, ti );
    /* skip to leadout? */
    if (i == m_nNumTracks)
      i = CDIO_CDROM_LEADOUT_TRACK;
  }

  info->SetCddbDiscId( CddbDiscId() );
  info->SetDiscLength( cdio_get_track_lba(cdio, CDIO_CDROM_LEADOUT_TRACK) / CDIO_CD_FRAMES_PER_SEC );

  info->SetAudioTrackCount( m_nNumAudio );
  info->SetDataTrackCount( m_nNumData );
  info->SetFirstAudioTrack( m_nFirstAudio );
  info->SetFirstDataTrack( m_nFirstData );

  char buf[1024];
  CLog::Log(LOGINFO, "CD Analysis Report");
  CLog::Log(LOGINFO, STRONG);

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
    PrintAnalysis(m_nFs, m_nNumAudio);
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
        m_nFs = FS_NO_DATA;
        ti.ms_offset = 0;
        ti.isofs_size = 0;
        ti.nJolietLevel = 0;
        ti.nFrames = cdio_get_track_lba(cdio, i);
        cdtext_init(&ti.cdtext);
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
      cdtext_init(&ti.cdtext);
      ti.nfsInfo = m_nFs;
      // valid UDF version for xbox
      if ((m_nFs & FS_MASK) == FS_UDF)
      {
        // Is UDF 1.02
        if (m_nUDFVerMajor > 0x1)
        {
          ti.nfsInfo = FS_UNKNOWN;
          m_strDiscLabel.Empty();
        }
        else if (m_nUDFVerMinor > 0x2)
        {
          ti.nfsInfo = FS_UNKNOWN;
          m_strDiscLabel.Empty();
        }
      }

      if ((m_nFs & FS_MASK) == FS_ISO_UDF)
      {
        // fallback to iso9660 if not udf 1.02
        if (m_nUDFVerMajor > 0x1)
          ti.nfsInfo = FS_ISO_9660;
        else if (m_nUDFVerMinor > 0x2)
          ti.nfsInfo = FS_ISO_9660;
      }

      ti.ms_offset = m_nMsOffset;
      ti.isofs_size = m_nIsofsSize;
      ti.nJolietLevel = m_nJolietLevel;
      ti.nFrames = cdio_get_track_lba(cdio, i);
      info->SetDiscLabel(m_strDiscLabel);


      if (i > 1)
      {
        /* track is beyond last session -> new session found */
        m_nMsOffset = m_nStartTrack;

        CLog::Log(LOGINFO, "Session #%d starts at track %2i, LSN: %6i,"
                  " ISO 9660 blocks: %6i",
                  j++, i, m_nStartTrack, m_nIsofsSize);

        CLog::Log(LOGINFO, "ISO 9660: %i blocks, label '%.32s'\n",
                  m_nIsofsSize, buffer[0] + 40);
        m_nFs |= MULTISESSION;
        ti.nfsInfo = m_nFs;
      }
      else
      {
        PrintAnalysis(m_nFs, m_nNumAudio);
      }

      info->SetTrackInformation( i, ti );

      // xbox does not support multisession cd's
      if (!(((m_nFs & FS_MASK) == FS_ISO_9660 ||
             (m_nFs & FS_MASK) == FS_ISO_HFS ||
             /* (fs & FS_MASK) == FS_ISO_9660_INTERACTIVE) && (fs & XA))) */
             (m_nFs & FS_MASK) == FS_ISO_9660_INTERACTIVE)))
        break; /* no method for non-iso9660 multisessions */
    }
  }
  cdio_destroy( cdio );
  return info;
}


// Returns the sum of the decimal digits in a number. Eg. 1955 = 20
int CCdIoSupport::CddbDecDigitSum(int n)
{
  int ret = 0;

  for (;;)
  {
    ret += n % 10;
    n = n / 10;
    if (!n)
      return ret;
  }
}

// Return the number of seconds (discarding frame portion) of an MSF
UINT CCdIoSupport::MsfSeconds(msf_t *msf)
{
  return from_bcd8(msf->m)*60 + from_bcd8(msf->s);
}


// Compute the CDDB disk ID for an Audio disk.  This is a funny checksum
// consisting of the concatenation of 3 things:
//    the sum of the decimal digits of sizes of all tracks,
//    the total length of the disk, and
//    the number of tracks.

ULONG CCdIoSupport::CddbDiscId()
{
  int i, t, n = 0;
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
