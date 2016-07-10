/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://kodi.tv/
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#ifdef HAS_DVD_DRIVE

#include "cdioSupport.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/Environment.h"
#include <cdio/cdio.h>
#include <cdio/logging.h>
#include <cdio/util.h>
#include <cdio/mmc.h>
#include <cdio/cd_types.h>

#if defined(TARGET_WINDOWS)
#pragma comment(lib, "libcdio.lib")
#endif

using namespace MEDIA_DETECT;

std::shared_ptr<CLibcdio> CLibcdio::m_pInstance;

/* Some interesting sector numbers stored in the above buffer. */
#define ISO_SUPERBLOCK_SECTOR  16  /* buffer[0] */
#define UFS_SUPERBLOCK_SECTOR   4  /* buffer[2] */
#define BOOT_SECTOR            17  /* buffer[3] */
#define VCD_INFO_SECTOR       150  /* buffer[4] */
#define UDF_ANCHOR_SECTOR     256  /* buffer[5] */


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
    {0, 1, "BEA01", "UDF"},
    { 0 }
  };

#undef DEBUG_CDIO

static void
cdio_log_handler (cdio_log_level_t level, const char message[])
{
#ifdef DEBUG_CDIO
  switch (level)
  {
  case CDIO_LOG_ERROR:
    CLog::Log(LOGDEBUG,"**ERROR: %s", message);
    break;
  case CDIO_LOG_DEBUG:
    CLog::Log(LOGDEBUG,"--DEBUG: %s", message);
    break;
  case CDIO_LOG_WARN:
    CLog::Log(LOGDEBUG,"++ WARN: %s", message);
    break;
  case CDIO_LOG_INFO:
    CLog::Log(LOGDEBUG,"   INFO: %s", message);
    break;
  case CDIO_LOG_ASSERT:
    CLog::Log(LOGDEBUG,"!ASSERT: %s", message);
    break;
  default:
    //cdio_assert_not_reached ();
    break;
  }
#endif
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CLibcdio::CLibcdio(): s_defaultDevice(NULL)
{
  cdio_log_set_handler( cdio_log_handler );
}

CLibcdio::~CLibcdio()
{
  free(s_defaultDevice);
  s_defaultDevice = NULL;
}

void CLibcdio::ReleaseInstance()
{
  m_pInstance.reset();
}

std::shared_ptr<CLibcdio> CLibcdio::GetInstance()
{
  if (!m_pInstance)
  {
    m_pInstance = std::shared_ptr<CLibcdio>(new CLibcdio());
  }
  return m_pInstance;
}

CdIo_t* CLibcdio::cdio_open(const char *psz_source, driver_id_t driver_id)
{
  CSingleLock lock(*this);

  return( ::cdio_open(psz_source, driver_id) );
}

CdIo_t* CLibcdio::cdio_open_win32(const char *psz_source)
{
  CSingleLock lock(*this);

  return( ::cdio_open_win32(psz_source) );
}

void CLibcdio::cdio_destroy(CdIo_t *p_cdio)
{
  CSingleLock lock(*this);

  ::cdio_destroy(p_cdio);
}

discmode_t CLibcdio::cdio_get_discmode(CdIo_t *p_cdio)
{
  CSingleLock lock(*this);

  return( ::cdio_get_discmode(p_cdio) );
}

int CLibcdio::mmc_get_tray_status(const CdIo_t *p_cdio)
{
  CSingleLock lock(*this);

  return( ::mmc_get_tray_status(p_cdio) );
}

int CLibcdio::cdio_eject_media(CdIo_t **p_cdio)
{
  CSingleLock lock(*this);

  return( ::cdio_eject_media(p_cdio) );
}

track_t CLibcdio::cdio_get_last_track_num(const CdIo_t *p_cdio)
{
  CSingleLock lock(*this);

  return( ::cdio_get_last_track_num(p_cdio) );
}

lsn_t CLibcdio::cdio_get_track_lsn(const CdIo_t *p_cdio, track_t i_track)
{
  CSingleLock lock(*this);

  return( ::cdio_get_track_lsn(p_cdio, i_track) );
}

lsn_t CLibcdio::cdio_get_track_last_lsn(const CdIo_t *p_cdio, track_t i_track)
{
  CSingleLock lock(*this);

  return( ::cdio_get_track_last_lsn(p_cdio, i_track) );
}

driver_return_code_t CLibcdio::cdio_read_audio_sectors(
    const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, uint32_t i_blocks)
{
  CSingleLock lock(*this);

  return( ::cdio_read_audio_sectors(p_cdio, p_buf, i_lsn, i_blocks) );
}

char* CLibcdio::GetDeviceFileName()
{
  CSingleLock lock(*this);

  // If We don't have a DVD device initially present (Darwin or a USB DVD drive), 
  // We have to keep checking in case one appears.
  if (s_defaultDevice && strlen(s_defaultDevice) == 0)
  {
    free(s_defaultDevice);
    s_defaultDevice = NULL;
  }

  if (s_defaultDevice == NULL)
  {
    std::string strEnvDvd = CEnvironment::getenv("XBMC_DVD_DEVICE");
    if (!strEnvDvd.empty())
      s_defaultDevice = strdup(strEnvDvd.c_str());
    else
    {
      CdIo_t *p_cdio = ::cdio_open(NULL, DRIVER_UNKNOWN);
      if (p_cdio != NULL)
      {
        s_defaultDevice = strdup(::cdio_get_arg(p_cdio, "source"));
        ::cdio_destroy(p_cdio);
      }
      else
        s_defaultDevice = strdup("");
    }
  }
  return s_defaultDevice;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CCdIoSupport::CCdIoSupport()
: i(0),
  j(0),
  cdio(nullptr),
  m_nNumTracks(CDIO_INVALID_TRACK),
  m_nFirstTrackNum(CDIO_INVALID_TRACK)
{
  m_cdio = CLibcdio::GetInstance();
  m_nFirstData = -1;        /* # of first data track */
  m_nNumData = 0;                /* # of data tracks */
  m_nFirstAudio = -1;      /* # of first audio track */
  m_nNumAudio = 0;              /* # of audio tracks */
  m_nIsofsSize = 0;               /* size of session */
  m_nJolietLevel = 0;
  m_nFs = 0;
  m_nUDFVerMinor = 0;
  m_nUDFVerMajor = 0;
  m_nDataStart = 0;
  m_nMsOffset = 0;
  m_nStartTrack = 0;
}

CCdIoSupport::~CCdIoSupport()
{
}

bool CCdIoSupport::EjectTray()
{
  return false;
}

bool CCdIoSupport::CloseTray()
{
  return false;
}

HANDLE CCdIoSupport::OpenCDROM()
{
  CSingleLock lock(*m_cdio);

  char* source_name = m_cdio->GetDeviceFileName();
  CdIo* cdio = ::cdio_open(source_name, DRIVER_UNKNOWN);

  return (HANDLE) cdio;
}

HANDLE CCdIoSupport::OpenIMAGE( std::string& strFilename )
{
  CSingleLock lock(*m_cdio);

  CdIo* cdio = ::cdio_open(strFilename.c_str(), DRIVER_UNKNOWN);

  return (HANDLE) cdio;
}

INT CCdIoSupport::ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
  CSingleLock lock(*m_cdio);

  CdIo* cdio = (CdIo*) hDevice;
  if ( cdio == NULL )
    return -1;

  if ( ::cdio_read_mode1_sector( cdio, lpczBuffer, dwSector, false ) == 0 )
    return dwSector;

  return -1;
}

INT CCdIoSupport::ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
  CSingleLock lock(*m_cdio);

  CdIo* cdio = (CdIo*) hDevice;
  if ( cdio == NULL )
    return -1;

  if ( ::cdio_read_mode2_sector( cdio, lpczBuffer, dwSector, false ) == 0 )
    return dwSector;

  return -1;
}

INT CCdIoSupport::ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
  CSingleLock lock(*m_cdio);

  CdIo* cdio = (CdIo*) hDevice;
  if ( cdio == NULL )
    return -1;

  if ( ::cdio_read_audio_sector( cdio, lpczBuffer, dwSector ) == 0 )
    return dwSector;

  return -1;
}

VOID CCdIoSupport::CloseCDROM(HANDLE hDevice)
{
  CSingleLock lock(*m_cdio);

  CdIo* cdio = (CdIo*) hDevice;

  if ( cdio == NULL )
    return ;

  ::cdio_destroy( cdio );
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
    CLog::Log(LOGINFO, "ISO 9660: %i blocks, label %s",
              m_nIsofsSize, m_strDiscLabel.c_str());
    break;
  }

  switch (fs & FS_MASK)
  {
  case FS_UDF:
  case FS_ISO_UDF:
    CLog::Log(LOGINFO, "UDF: version %x.%2.2x\n",
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
#if defined(HAVE_VCDINFO) && defined(DEBUG)
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
  CSingleLock lock(*m_cdio);

  unsigned int track_sec_count = ::cdio_get_track_sec_count(cdio, track_num);
  memset(buffer[bufnum], 0, CDIO_CD_FRAMESIZE);

  if ( track_sec_count < (UINT)superblock)
  {
    ::cdio_debug("reading block %u skipped track %d has only %u sectors\n",
               superblock, track_num, track_sec_count);
    return -1;
  }

  ::cdio_debug("about to read sector %lu\n",
             (long unsigned int) offset + superblock);

  if (::cdio_get_track_green(cdio, track_num))
  {
    if (0 < ::cdio_read_mode2_sector(cdio, buffer[bufnum],
                                   offset + superblock, false))
      return -1;
  }
  else
  {
    if (0 < ::cdio_read_mode1_sector(cdio, buffer[bufnum],
                                   offset + superblock, false))
      return -1;
  }

  return 0;
}

bool CCdIoSupport::IsIt(int num)
{
  signature_t *sigp = &sigs[num];
  int len = strlen(sigp->sig_str);

  //! @todo check that num < largest sig.
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
  CSingleLock lock(*m_cdio);

  int ret = FS_UNKNOWN;
  cdio_iso_analysis_t anal;
  cdio_fs_anal_t fs;
  bool udf = false;

  memset(&anal, 0, sizeof(anal));
  discmode_t mode = ::cdio_get_discmode(cdio);
  if (::cdio_is_discmode_dvd(mode))
  {
    m_strDiscLabel = "";
    m_nIsofsSize = ::cdio_get_disc_last_lsn(cdio);
    m_nJolietLevel = ::cdio_get_joliet_level(cdio);

    return FS_ISO_9660;
  }

  fs = ::cdio_guess_cd_type(cdio, start_session, track_num, &anal);

  switch(CDIO_FSTYPE(fs))
    {
    case CDIO_FS_AUDIO:
      ret = FS_NO_DATA;
      break;

    case CDIO_FS_HIGH_SIERRA:
      ret = FS_HIGH_SIERRA;
      break;

    case CDIO_FS_ISO_9660:
      ret = FS_ISO_9660;
      break;

    case CDIO_FS_INTERACTIVE:
      ret = FS_ISO_9660_INTERACTIVE;
      break;

    case CDIO_FS_HFS:
      ret = FS_HFS;
      break;

    case CDIO_FS_UFS:
      ret = FS_UFS;
      break;

    case CDIO_FS_EXT2:
      ret = FS_EXT2;
      break;

    case CDIO_FS_UDF:
      ret = FS_UDF;
      udf = true;
      break;

    case CDIO_FS_ISO_UDF:
      ret = FS_ISO_UDF;
      udf = true;
      break;

    default:
      break;
    }

  if (udf)
  {
    m_nUDFVerMinor = anal.UDFVerMinor;
    m_nUDFVerMajor = anal.UDFVerMajor;
  }

  m_strDiscLabel = anal.iso_label;
  m_nIsofsSize = anal.isofs_size;
  m_nJolietLevel = anal.joliet_level;

  return ret;
}

void CCdIoSupport::GetCdTextInfo(xbmc_cdtext_t &xcdt, int trackNum)
{
  // cdtext disabled for windows as some setup doesn't like mmc commands
  // and stall for over a minute in cdio_get_cdtext 83
#if !defined(TARGET_WINDOWS)
  CSingleLock lock(*m_cdio);

  // Get the CD-Text , if any
#if defined (LIBCDIO_VERSION_NUM) && (LIBCDIO_VERSION_NUM > 83)
  cdtext_t *pcdtext = static_cast<cdtext_t*>( cdio_get_cdtext(cdio) );
#else
  cdtext_t *pcdtext = (cdtext_t *)::cdio_get_cdtext(cdio, trackNum);
#endif 
  
  if (pcdtext == NULL)
    return ;

#if defined (LIBCDIO_VERSION_NUM) && (LIBCDIO_VERSION_NUM > 83)
  for (int i=0; i < MAX_CDTEXT_FIELDS; i++) 
    if (cdtext_get_const(pcdtext, (cdtext_field_t)i, trackNum))
      xcdt[(cdtext_field_t)i] = cdtext_field2str((cdtext_field_t)i);
#else
  // Same ids used in libcdio and for our structure + the ids are consecutive make this copy loop safe.
  for (int i = 0; i < MAX_CDTEXT_FIELDS; i++)
    if (pcdtext->field[i])
      xcdt[(cdtext_field_t)i] = pcdtext->field[(cdtext_field_t)i];
#endif
#endif // TARGET_WINDOWS
}

CCdInfo* CCdIoSupport::GetCdInfo(char* cDeviceFileName)
{
  CSingleLock lock(*m_cdio);

  char* source_name;
  if(cDeviceFileName == NULL)
    source_name = m_cdio->GetDeviceFileName();
  else
    source_name = cDeviceFileName;

  cdio = ::cdio_open(source_name, DRIVER_UNKNOWN);
  if (cdio == NULL)
  {
    CLog::Log(LOGERROR, "%s: Error in automatically selecting driver with input", __FUNCTION__);
    return NULL;
  }

  bool bIsCDRom = true;

  m_nFirstTrackNum = ::cdio_get_first_track_num(cdio);
  if (m_nFirstTrackNum == CDIO_INVALID_TRACK)
  {
#if !defined(TARGET_DARWIN)
    ::cdio_destroy(cdio);
    return NULL;
#else
    m_nFirstTrackNum = 1;
    bIsCDRom = false;
#endif
  }

  m_nNumTracks = ::cdio_get_num_tracks(cdio);
  if (m_nNumTracks == CDIO_INVALID_TRACK)
  {
#if !defined(TARGET_DARWIN)
    ::cdio_destroy(cdio);
    return NULL;
#else
    m_nNumTracks = 1;
    bIsCDRom = false;
#endif
  }

  CCdInfo* info = new CCdInfo;
  info->SetFirstTrack( m_nFirstTrackNum );
  info->SetTrackCount( m_nNumTracks );

  for (i = m_nFirstTrackNum; i <= CDIO_CDROM_LEADOUT_TRACK; i++)
  {
    msf_t msf;
    if (bIsCDRom && !::cdio_get_track_msf(cdio, i, &msf))
    {
      trackinfo ti;
      ti.nfsInfo = FS_UNKNOWN;
      ti.ms_offset = 0;
      ti.isofs_size = 0;
      ti.nJolietLevel = 0;
      ti.nFrames = 0;
      ti.nMins = 0;
      ti.nSecs = 0;
      info->SetTrackInformation( i, ti );
      CLog::Log(LOGDEBUG, "cdio_track_msf for track %i failed, I give up.", i);
      delete info;
      ::cdio_destroy(cdio);
      return NULL;
    }

    trackinfo ti;
    if (bIsCDRom && TRACK_FORMAT_AUDIO == ::cdio_get_track_format(cdio, i))
    {
      m_nNumAudio++;
      ti.nfsInfo = FS_NO_DATA;
      m_nFs = FS_NO_DATA;
      int temp1 = ::cdio_get_track_lba(cdio, i) - CDIO_PREGAP_SECTORS;
      int temp2 = ::cdio_get_track_lba(cdio, i + 1) - CDIO_PREGAP_SECTORS;
      // The length is the address of the second track minus the address of the first track
      temp2 -= temp1;                  // temp2 now has length of track1 in frames
      ti.nMins = temp2 / (60 * 75);    // calculate the number of minutes
      temp2 %= 60 * 75;                // calculate the left-over frames
      ti.nSecs = temp2 / 75;           // calculate the number of seconds
      if ( -1 == m_nFirstAudio)
        m_nFirstAudio = i;

      // Make sure that we have the Disc related info available
      if (i == 1)
      {
        xbmc_cdtext_t xcdt;
        GetCdTextInfo(xcdt, 0);
        info->SetDiscCDTextInformation( xcdt );
      }

      // Get this tracks info
      GetCdTextInfo(ti.cdtext, i);
    }
    else
    {
      m_nNumData++;
      if ( -1 == m_nFirstData)
        m_nFirstData = i;
    }
    ti.nfsInfo = FS_NO_DATA;
    ti.ms_offset = 0;
    ti.isofs_size = 0;
    ti.nJolietLevel = 0;
    ti.nFrames = ::cdio_get_track_lba(cdio, i);
    ti.nMins = 0;
    ti.nSecs = 0;

    info->SetTrackInformation( i, ti );
    /* skip to leadout? */
    if (i == m_nNumTracks)
      i = CDIO_CDROM_LEADOUT_TRACK;
  }

  info->SetCddbDiscId( CddbDiscId() );
  info->SetDiscLength( ::cdio_get_track_lba(cdio, CDIO_CDROM_LEADOUT_TRACK) / CDIO_CD_FRAMES_PER_SEC );

  info->SetAudioTrackCount( m_nNumAudio );
  info->SetDataTrackCount( m_nNumData );
  info->SetFirstAudioTrack( m_nFirstAudio );
  info->SetFirstDataTrack( m_nFirstData );

  CLog::Log(LOGINFO, "CD Analysis Report");
  CLog::Log(LOGINFO, STRONG);

  /* Try to find out what sort of CD we have */
  if (0 == m_nNumData)
  {
    /* no data track, may be a "real" audio CD or hidden track CD */

    msf_t msf;
    ::cdio_get_track_msf(cdio, 1, &msf);
    m_nStartTrack = ::cdio_msf_to_lsn(&msf);

    /* CD-I/Ready says start_track <= 30*75 then CDDA */
    if (m_nStartTrack > 100 /* 100 is just a guess */)
    {
      m_nFs = GuessFilesystem(0, 1);
      if ((m_nFs & FS_MASK) != FS_UNKNOWN)
        m_nFs |= HIDDEN_TRACK;
      else
      {
        m_nFs &= ~FS_MASK; /* del filesystem info */
        CLog::Log(LOGDEBUG, "Oops: %i unused sectors at start, but hidden track check failed.", m_nStartTrack);
      }
    }
    PrintAnalysis(m_nFs, m_nNumAudio);
  }
  else
  {
    /* We have data track(s) */
    for (j = 2, i = m_nFirstData; i <= m_nNumTracks; i++)
    {
      msf_t msf;
      track_format_t track_format = ::cdio_get_track_format(cdio, i);

      ::cdio_get_track_msf(cdio, i, &msf);

      switch ( track_format )
      {
      case TRACK_FORMAT_AUDIO:
        {
          trackinfo ti;
          ti.nfsInfo = FS_NO_DATA;
          m_nFs = FS_NO_DATA;
          ti.ms_offset = 0;
          ti.isofs_size = 0;
          ti.nJolietLevel = 0;
          ti.nFrames = ::cdio_get_track_lba(cdio, i);
          ti.nMins = 0;
          ti.nSecs = 0;
          info->SetTrackInformation( i + 1, ti );
        }
      case TRACK_FORMAT_ERROR:
        break;
      case TRACK_FORMAT_CDI:
      case TRACK_FORMAT_XA:
      case TRACK_FORMAT_DATA:
      case TRACK_FORMAT_PSX:
        break;
      }

      m_nStartTrack = (i == 1) ? 0 : ::cdio_msf_to_lsn(&msf);

      /* Save the start of the data area */
      if (i == m_nFirstData)
        m_nDataStart = m_nStartTrack;

      /* Skip tracks which belong to the current walked session */
      if (m_nStartTrack < m_nDataStart + m_nIsofsSize)
        continue;

      m_nFs = GuessFilesystem(m_nStartTrack, i);
      trackinfo ti;
      ti.nfsInfo = m_nFs;
      ti.ms_offset = m_nMsOffset;
      ti.isofs_size = m_nIsofsSize;
      ti.nJolietLevel = m_nJolietLevel;
      ti.nFrames = ::cdio_get_track_lba(cdio, i);
      ti.nMins = 0;
      ti.nSecs = 0;
      info->SetDiscLabel(m_strDiscLabel);


      if (i > 1)
      {
        /* Track is beyond last session -> new session found */
        m_nMsOffset = m_nStartTrack;

        CLog::Log(LOGINFO, "Session #%d starts at track %2i, LSN: %6i,"
                  " ISO 9660 blocks: %6i",
                  j++, i, m_nStartTrack, m_nIsofsSize);

        CLog::Log(LOGINFO, "ISO 9660: %i blocks, label %s",
                  m_nIsofsSize, m_strDiscLabel.c_str());
        m_nFs |= MULTISESSION;
        ti.nfsInfo = m_nFs;
      }
      else
      {
        PrintAnalysis(m_nFs, m_nNumAudio);
      }

      info->SetTrackInformation( i, ti );

    }
  }
  ::cdio_destroy( cdio );
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
  CSingleLock lock(*m_cdio);

  return ::cdio_from_bcd8(msf->m)*60 + ::cdio_from_bcd8(msf->s);
}


// Compute the CDDB disk ID for an Audio disk.
// This is a funny checksum consisting of the concatenation of 3 things:
//    The sum of the decimal digits of sizes of all tracks,
//    The total length of the disk, and
//    The number of tracks.

uint32_t CCdIoSupport::CddbDiscId()
{
  CSingleLock lock(*m_cdio);

  int i, t, n = 0;
  msf_t start_msf;
  msf_t msf;

  for (i = 1; i <= m_nNumTracks; i++)
  {
    ::cdio_get_track_msf(cdio, i, &msf);
    n += CddbDecDigitSum(MsfSeconds(&msf));
  }

  ::cdio_get_track_msf(cdio, 1, &start_msf);
  ::cdio_get_track_msf(cdio, CDIO_CDROM_LEADOUT_TRACK, &msf);

  t = MsfSeconds(&msf) - MsfSeconds(&start_msf);

  return ((n % 0xff) << 24 | t << 8 | m_nNumTracks);
}

#endif

