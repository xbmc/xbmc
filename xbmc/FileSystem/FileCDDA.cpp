#include "stdafx.h"
#include "FileCDDA.h"
#include <sys/stat.h>
#include "../Util.h"
#ifndef _LINUX
#include "../lib/libcdio/Util.h"
#else
#include <cdio/util.h>
#endif
#include "../DetectDVDType.h"


using namespace MEDIA_DETECT;
using namespace XFILE;

CFileCDDA::CFileCDDA(void)
{
  m_pCdIo = NULL;
  m_lsnStart = CDIO_INVALID_LSN;
  m_lsnCurrent = CDIO_INVALID_LSN;
  m_lsnEnd = CDIO_INVALID_LSN;
}

CFileCDDA::~CFileCDDA(void)
{
  Close();
}

bool CFileCDDA::Open(const CURL& url, bool bBinary /*=true*/)
{
  if (!CDetectDVDMedia::IsDiscInDrive() || !IsValidFile(url))
    return false;

  // Open the dvd drive
#ifdef _LINUX
  m_pCdIo = cdio_open(CCdIoSupport::GetDeviceFileName(), DRIVER_UNKNOWN);
#else
  m_pCdIo = cdio_open_win32("D:");
#endif
  if (!m_pCdIo)
  {
    CLog::Log(LOGERROR, "file cdda: Opening the dvd drive failed");
    return false;
  }

  int iTrack = GetTrackNum(url);

  m_lsnStart = cdio_get_track_lsn(m_pCdIo, iTrack);
  m_lsnEnd = cdio_get_track_last_lsn(m_pCdIo, iTrack);
  m_lsnCurrent = m_lsnStart;

  if (m_lsnStart == CDIO_INVALID_LSN || m_lsnEnd == CDIO_INVALID_LSN)
  {
    cdio_destroy(m_pCdIo);
    m_pCdIo = NULL;
    return false;
  }

  return true;
}

bool CFileCDDA::Exists(const CURL& url)
{
  if (!IsValidFile(url))
    return false;

  int iTrack = GetTrackNum(url);

  if (!Open(url))
    return false;

  int iLastTrack = cdio_get_last_track_num(m_pCdIo);
  if (iLastTrack == CDIO_INVALID_TRACK)
    return false;

  return (iTrack > 0 && iTrack <= iLastTrack);
}

int CFileCDDA::Stat(const CURL& url, struct __stat64* buffer)
{
  if (Open(url))
  {
    buffer->st_size = GetLength();
    buffer->st_mode = _S_IFREG;
    Close();
    return 0;
  }
  errno = ENOENT;
  return -1;
}

unsigned int CFileCDDA::Read(void* lpBuf, __int64 uiBufSize)
{
  if (!m_pCdIo || !CDetectDVDMedia::IsDiscInDrive())
    return 0;

  int iSectorCount = (int)uiBufSize / CDIO_CD_FRAMESIZE_RAW;

  if (iSectorCount <= 0)
    return 0;

  // Are there enough sectors left to read
  if (m_lsnCurrent + iSectorCount > m_lsnEnd)
    iSectorCount = m_lsnEnd - m_lsnCurrent;

  if (cdio_read_audio_sectors(m_pCdIo, lpBuf, m_lsnCurrent, iSectorCount) != DRIVER_OP_SUCCESS)
  {
    CLog::Log(LOGERROR, "file cdda: Reading %d sectors of audio data starting at lsn %d failed", iSectorCount, m_lsnCurrent);
    return 0;
  }

  m_lsnCurrent += iSectorCount;

  return iSectorCount*CDIO_CD_FRAMESIZE_RAW;
}

__int64 CFileCDDA::Seek(__int64 iFilePosition, int iWhence /*=SEEK_SET*/)
{
  if (!m_pCdIo)
    return -1;

  lsn_t lsnPosition = (int)iFilePosition / CDIO_CD_FRAMESIZE_RAW;

  switch (iWhence)
  {
  case SEEK_SET:
    // cur = pos
    m_lsnCurrent = m_lsnStart + lsnPosition;
    break;
  case SEEK_CUR:
    // cur += pos
    m_lsnCurrent += lsnPosition;
    break;
  case SEEK_END:
    // end += pos
    m_lsnCurrent = m_lsnEnd + lsnPosition;
    break;
  }

  return ((m_lsnCurrent -m_lsnStart)*CDIO_CD_FRAMESIZE_RAW);
}

void CFileCDDA::Close()
{
  if (m_pCdIo)
  {
    cdio_destroy(m_pCdIo);
    m_pCdIo = NULL;
  }
}

__int64 CFileCDDA::GetPosition()
{
  if (!m_pCdIo)
    return 0;

  return ((m_lsnCurrent -m_lsnStart)*CDIO_CD_FRAMESIZE_RAW);
}

__int64 CFileCDDA::GetLength()
{
  if (!m_pCdIo)
    return 0;

  return ((m_lsnEnd -m_lsnStart)*CDIO_CD_FRAMESIZE_RAW);
}

bool CFileCDDA::IsValidFile(const CURL& url)
{
  CStdString strFileName;
  url.GetURL(strFileName);

  // Only .cdda files are supported
  CStdString strExtension;
  CUtil::GetExtension(strFileName, strExtension);
  strExtension.MakeLower();

  return (strExtension == ".cdda");
}

int CFileCDDA::GetTrackNum(const CURL& url)
{
  CStdString strFileName;
  url.GetURL(strFileName);

  // get track number from "cdda://local/01.cdda"
  return atoi(strFileName.substr(13, strFileName.size() - 13 - 5).c_str());
}

#define SECTOR_COUNT 52 // max. sectors that can be read at once
int CFileCDDA::GetChunkSize()
{
  return SECTOR_COUNT*CDIO_CD_FRAMESIZE_RAW;
}
