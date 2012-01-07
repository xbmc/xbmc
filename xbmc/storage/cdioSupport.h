/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

//  CCdInfo   -  Information about media type of an inserted cd
//  CCdIoSupport -  Wrapper class for libcdio with the interface of CIoSupport
//     and detecting the filesystem on the Disc.
//
// by Bobbin007 in 2003
//  CD-Text support by Mog - Oct 2004
//
//
//

#pragma once

#include "system.h" // for HAS_DVD_DRIVE

#ifdef HAS_DVD_DRIVE

#include "storage/IoSupport.h"

#include <cdio/cdio.h>
#include "threads/CriticalSection.h"
#include "utils/StdString.h"
#include <map>

namespace MEDIA_DETECT
{

#define STRONG "__________________________________\n"
//#define NORMAL ""

#define FS_NO_DATA              0   /* audio only */
#define FS_HIGH_SIERRA   1
#define FS_ISO_9660    2
#define FS_INTERACTIVE   3
#define FS_HFS     4
#define FS_UFS     5
#define FS_EXT2     6
#define FS_ISO_HFS              7  /* both hfs & isofs filesystem */
#define FS_ISO_9660_INTERACTIVE 8  /* both CD-RTOS and isofs filesystem */
#define FS_3DO     9
#define FS_UDFX     10
#define FS_UDF     11
#define FS_ISO_UDF   12
#define FS_UNKNOWN    15
#define FS_MASK     15

#define XA      16
#define MULTISESSION   32
#define PHOTO_CD    64
#define HIDDEN_TRACK   128
#define CDTV     256
#define BOOTABLE    512
#define VIDEOCDI    1024
#define ROCKRIDGE    2048
#define JOLIET     4096
#define CVD      8192   /* Choiji Video CD */

#define IS_ISOFS    0
#define IS_CD_I     1
#define IS_CDTV     2
#define IS_CD_RTOS    3
#define IS_HS     4
#define IS_BRIDGE    5
#define IS_XA     6
#define IS_PHOTO_CD    7
#define IS_EXT2     8
#define IS_UFS     9
#define IS_BOOTABLE    10
#define IS_VIDEO_CD    11 /* Video CD */
#define IS_CVD     12 /* Chinese Video CD - slightly incompatible with SVCD */
#define IS_UDFX     13
#define IS_UDF     14

typedef struct signature
{
  unsigned int buf_num;
  unsigned int offset;
  const char *sig_str;
  const char *description;
}
signature_t;

typedef std::map<cdtext_field_t, CStdString> xbmc_cdtext_t;

typedef struct TRACKINFO
{
  int nfsInfo;  // Information of the Tracks Filesystem
  int nJolietLevel; // Jouliet Level
  int ms_offset;  // Multisession Offset
  int isofs_size;  // Size of the ISO9660 Filesystem
  int nFrames;  // Can be used for cddb query
  int nMins;   // minutes playtime part of Track
  int nSecs;   // seconds playtime part of Track
  xbmc_cdtext_t cdtext; //  CD-Text for this track
}
trackinfo;


class CCdInfo
{
public:
  CCdInfo()
  {
    m_bHasCDDBInfo = true;
    m_nLength = m_nFirstTrack = m_nNumTrack = m_nNumAudio = m_nFirstAudio = m_nNumData = m_nFirstData = 0;
  }

  trackinfo GetTrackInformation( int nTrack ) { return m_ti[nTrack -1]; }
  xbmc_cdtext_t GetDiscCDTextInformation() { return m_cdtext; }

  bool HasDataTracks() { return (m_nNumData > 0); }
  bool HasAudioTracks() { return (m_nNumAudio > 0); }
  int GetFirstTrack() { return m_nFirstTrack; }
  int GetTrackCount() { return m_nNumTrack; }
  int GetFirstAudioTrack() { return m_nFirstAudio; }
  int GetFirstDataTrack() { return m_nFirstData; }
  int GetDataTrackCount() { return m_nNumData; }
  int GetAudioTrackCount() { return m_nNumAudio; }
  ULONG GetCddbDiscId() { return m_ulCddbDiscId; }
  int GetDiscLength() { return m_nLength; }
  CStdString GetDiscLabel(){ return m_strDiscLabel; }

  // CD-ROM with ISO 9660 filesystem
  bool IsIso9660( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_ISO_9660); }
  // CD-ROM with joliet extension
  bool IsJoliet( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & JOLIET) ? false : true; }
  // Joliet extension level
  int GetJolietLevel( int nTrack ) { return m_ti[nTrack - 1].nJolietLevel; }
  // ISO filesystem size
  int GetIsoSize( int nTrack ) { return m_ti[nTrack - 1].isofs_size; }
  // CD-ROM with rockridge extensions
  bool IsRockridge( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & ROCKRIDGE) ? false : true; }

  // CD-ROM with CD-RTOS and ISO 9660 filesystem
  bool IsIso9660Interactive( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_ISO_9660_INTERACTIVE); }

  // CD-ROM with High Sierra filesystem
  bool IsHighSierra( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_HIGH_SIERRA); }

  // CD-Interactive, with audiotracks > 0 CD-Interactive/Ready
  bool IsCDInteractive( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_INTERACTIVE); }

  // CD-ROM with Macintosh HFS
  bool IsHFS( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_HFS); }

  // CD-ROM with both Macintosh HFS and ISO 9660 filesystem
  bool IsISOHFS( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_ISO_HFS); }

  // CD-ROM with both UDF and ISO 9660 filesystem
  bool IsISOUDF( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_ISO_UDF); }

  // CD-ROM with Unix UFS
  bool IsUFS( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_UFS); }

  // CD-ROM with Linux second extended filesystem
  bool IsEXT2( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_EXT2); }

  // CD-ROM with Panasonic 3DO filesystem
  bool Is3DO( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_3DO); }

  // CD-ROM with XBOX UDFX filesystem
  bool IsUDFX( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_UDFX); }

  // Mixed Mode CD-ROM
  bool IsMixedMode( int nTrack ) { return (m_nFirstData == 1 && m_nNumAudio > 0); }

  // CD-ROM with XA sectors
  bool IsXA( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & XA) ? false : true; }

  // Multisession CD-ROM
  bool IsMultiSession( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & MULTISESSION) ? false : true; }
  // Gets multisession offset
  int GetMultisessionOffset( int nTrack ) { return m_ti[nTrack - 1].ms_offset; }

  // Hidden Track on Audio CD
  bool IsHiddenTrack( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & HIDDEN_TRACK) ? false : true; }

  // Photo CD, with audiotracks > 0 Portfolio Photo CD
  bool IsPhotoCd( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & PHOTO_CD) ? false : true; }

  // CD-ROM with Commodore CDTV
  bool IsCdTv( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & CDTV) ? false : true; }

  // CD-Plus/Extra
  bool IsCDExtra( int nTrack ) { return (m_nFirstData > 1); }

  // Bootable CD
  bool IsBootable( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & BOOTABLE) ? false : true; }

  // Video CD
  bool IsVideoCd( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & VIDEOCDI && m_nNumAudio == 0); }

  // Chaoji Video CD
  bool IsChaojiVideoCD( int nTrack ) { return (m_ti[nTrack - 1].nfsInfo & CVD) ? false : true; }

  // Audio Track
  bool IsAudio( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_NO_DATA); }

  // UDF filesystem
  bool IsUDF( int nTrack ) { return ((m_ti[nTrack - 1].nfsInfo & FS_MASK) == FS_UDF); }

  // Has the cd a filesystem that is readable by the xbox
  bool IsValidFs() { return (IsISOHFS(1) || IsIso9660(1) || IsIso9660Interactive(1) || IsISOUDF(1) || IsUDF(1) || IsUDFX(1) || IsAudio(1)); }

  void SetFirstTrack( int nTrack ) { m_nFirstTrack = nTrack; }
  void SetTrackCount( int nCount ) { m_nNumTrack = nCount; }
  void SetFirstAudioTrack( int nTrack ) { m_nFirstAudio = nTrack; }
  void SetFirstDataTrack( int nTrack ) { m_nFirstData = nTrack; }
  void SetDataTrackCount( int nCount ) { m_nNumData = nCount; }
  void SetAudioTrackCount( int nCount ) { m_nNumAudio = nCount; }
  void SetTrackInformation( int nTrack, trackinfo nInfo ) { if ( nTrack > 0 && nTrack <= 99 ) m_ti[nTrack - 1] = nInfo; }
  void SetDiscCDTextInformation( xbmc_cdtext_t cdtext ) { m_cdtext = cdtext; }

  void SetCddbDiscId( ULONG ulCddbDiscId ) { m_ulCddbDiscId = ulCddbDiscId; }
  void SetDiscLength( int nLength ) { m_nLength = nLength; }
  bool HasCDDBInfo() { return m_bHasCDDBInfo; }
  void SetNoCDDBInfo() { m_bHasCDDBInfo = false; }

  void SetDiscLabel(const CStdString& strDiscLabel){ m_strDiscLabel = strDiscLabel; }

private:
  int m_nFirstData;        /* # of first data track */
  int m_nNumData;               /* # of data tracks */
  int m_nFirstAudio;      /* # of first audio track */
  int m_nNumAudio;             /* # of audio tracks */
  int m_nNumTrack;
  int m_nFirstTrack;
  trackinfo m_ti[100];
  ULONG m_ulCddbDiscId;
  int m_nLength;   // Disclength can be used for cddb query, also see trackinfo.nFrames
  bool m_bHasCDDBInfo;
  CStdString m_strDiscLabel;
  xbmc_cdtext_t m_cdtext;  //  CD-Text for this disc
};

class CLibcdio : public CCriticalSection
{
private:
  CLibcdio();
public:
  virtual ~CLibcdio();

  static void RemoveInstance();
  static CLibcdio* GetInstance();

  // libcdio is not thread safe so these are wrappers to libcdio routines
  CdIo_t* cdio_open(const char *psz_source, driver_id_t driver_id);
  CdIo_t* cdio_open_win32(const char *psz_source);
  void cdio_destroy(CdIo_t *p_cdio);
  discmode_t cdio_get_discmode(CdIo_t *p_cdio);
  int mmc_get_tray_status(const CdIo_t *p_cdio);
  int cdio_eject_media(CdIo_t **p_cdio);
  track_t cdio_get_last_track_num(const CdIo_t *p_cdio);
  lsn_t cdio_get_track_lsn(const CdIo_t *p_cdio, track_t i_track);
  lsn_t cdio_get_track_last_lsn(const CdIo_t *p_cdio, track_t i_track);
  driver_return_code_t cdio_read_audio_sectors(const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, uint32_t i_blocks);

  char* GetDeviceFileName();

private:
  static char* s_defaultDevice;
  CCriticalSection m_critSection;
  static CLibcdio* m_pInstance;
};

class CCdIoSupport
{
public:
  CCdIoSupport();
  virtual ~CCdIoSupport();

  HRESULT EjectTray();
  HRESULT CloseTray();

  HANDLE OpenCDROM();
  HANDLE OpenIMAGE( CStdString& strFilename );
  INT ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
  INT ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
  INT ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
  VOID CloseCDROM(HANDLE hDevice);

  void PrintAnalysis(int fs, int num_audio);

  CCdInfo* GetCdInfo(char* cDeviceFileName=NULL);
  void GetCdTextInfo(xbmc_cdtext_t &xcdt, int trackNum);

protected:
  int ReadBlock(int superblock, uint32_t offset, uint8_t bufnum, track_t track_num);
  bool IsIt(int num);
  int IsHFS(void);
  int Is3DO(void);
  int IsJoliet(void);
  int IsUDF(void);
  int GetSize(void);
  int GetJolietLevel( void );
  int GuessFilesystem(int start_session, track_t track_num);

  ULONG CddbDiscId();
  int CddbDecDigitSum(int n);
  UINT MsfSeconds(msf_t *msf);

private:
  char buffer[7][CDIO_CD_FRAMESIZE_RAW];  /* for CD-Data */
  static signature_t sigs[17];
  int i, j;                                                           /* index */
  int m_nStartTrack;                                   /* first sector of track */
  int m_nIsofsSize;                                      /* size of session */
  int m_nJolietLevel;
  int m_nMsOffset;                /* multisession offset found by track-walking */
  int m_nDataStart;                                       /* start of data area */
  int m_nFs;
  int m_nUDFVerMinor;
  int m_nUDFVerMajor;

  CdIo* cdio;
  track_t m_nNumTracks;
  track_t m_nFirstTrackNum;

  CStdString m_strDiscLabel;

  int m_nFirstData;        /* # of first data track */
  int m_nNumData;                /* # of data tracks */
  int m_nFirstAudio;      /* # of first audio track */
  int m_nNumAudio;              /* # of audio tracks */

  CLibcdio* m_cdio;
};

}

#endif
