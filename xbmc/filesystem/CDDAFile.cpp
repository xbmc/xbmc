/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "CDDAFile.h"
#include <sys/stat.h>
#include "URL.h"
#include "storage/MediaManager.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include <algorithm>

using namespace MEDIA_DETECT;
using namespace XFILE;

CFileCDDA::CFileCDDA(void)
{
  m_pCdIo = NULL;
  m_lsnStart = CDIO_INVALID_LSN;
  m_lsnCurrent = CDIO_INVALID_LSN;
  m_lsnEnd = CDIO_INVALID_LSN;
  m_cdio = CLibcdio::GetInstance();
  m_iSectorCount = 52;
  m_TrackBuf = (uint8_t *) malloc(CDIO_CD_FRAMESIZE_RAW);
  p_TrackBuf = 0;
  f_TrackBuf = 0;
}

CFileCDDA::~CFileCDDA(void)
{
  free(m_TrackBuf);
  Close();
}

bool CFileCDDA::Open(const CURL& url)
{
  std::string strURL = url.GetWithoutFilename();

  // Flag TrackBuffer = FALSE, TrackBuffer is empty
  f_TrackBuf = 0;

  if (!g_mediaManager.IsDiscInDrive(strURL) || !IsValidFile(url))
    return false;

  // Open the dvd drive
#ifdef TARGET_POSIX
  m_pCdIo = m_cdio->cdio_open(g_mediaManager.TranslateDevicePath(strURL).c_str(), DRIVER_UNKNOWN);
#elif defined(TARGET_WINDOWS)
  m_pCdIo = m_cdio->cdio_open_win32(g_mediaManager.TranslateDevicePath(strURL, true).c_str());
#endif
  if (!m_pCdIo)
  {
    CLog::Log(LOGERROR, "file cdda: Opening the dvd drive failed");
    return false;
  }

  int iTrack = GetTrackNum(url);

  m_lsnStart = m_cdio->cdio_get_track_lsn(m_pCdIo, iTrack);
  m_lsnEnd = m_cdio->cdio_get_track_last_lsn(m_pCdIo, iTrack);
  m_lsnCurrent = m_lsnStart;

  if (m_lsnStart == CDIO_INVALID_LSN || m_lsnEnd == CDIO_INVALID_LSN)
  {
    m_cdio->cdio_destroy(m_pCdIo);
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

  int iLastTrack = m_cdio->cdio_get_last_track_num(m_pCdIo);
  if (iLastTrack == CDIO_INVALID_TRACK)
    return false;

  return (iTrack > 0 && iTrack <= iLastTrack);
}

int CFileCDDA::Stat(const CURL& url, struct __stat64* buffer)
{
  if (Open(url))
  {
    memset(buffer, 0, sizeof(struct __stat64));
    buffer->st_size = GetLength();
    buffer->st_mode = _S_IFREG;
    Close();
    return 0;
  }
  errno = ENOENT;
  return -1;
}

ssize_t CFileCDDA::Read(void* lpBuf, size_t uiBufSize)
{

  ssize_t returnValue;
  int iSectorCount;
  void *destBuf;


  if (!m_pCdIo || !g_mediaManager.IsDiscInDrive())
  {
    CLog::Log(LOGERROR, "file cdda: Aborted because no disc in drive or no m_pCdIo");
    return -1;
  }

  uiBufSize = std::min( uiBufSize, (size_t)SSIZE_MAX );

  // If we have data in the TrackBuffer, they must be used first
  if (f_TrackBuf)
  {
    // Get at most the remaining data in m_TrackBuf
    uiBufSize = std::min(uiBufSize, CDIO_CD_FRAMESIZE_RAW - p_TrackBuf);
    memcpy(lpBuf, m_TrackBuf + p_TrackBuf, uiBufSize);
    // Update the data offset
    p_TrackBuf += uiBufSize;
    // Is m_TrackBuf empty?
    f_TrackBuf = (CDIO_CD_FRAMESIZE_RAW == p_TrackBuf);
    // All done, return read bytes
    return uiBufSize;
  }

  // No data left in buffer

  // Is this a short read?
  if (uiBufSize < CDIO_CD_FRAMESIZE_RAW)
  {
    // short request, buffer one full sector
    iSectorCount = 1;
    destBuf = m_TrackBuf;
  }
  else // normal request
  {
    // limit number of sectors that fits in buffer by m_iSectorCount
    iSectorCount = std::min((int)uiBufSize / CDIO_CD_FRAMESIZE_RAW, m_iSectorCount);
    destBuf = lpBuf;
  }
  
  // Are there enough sectors left to read?
  iSectorCount = std::min(iSectorCount, m_lsnEnd - m_lsnCurrent);

  // Have we reached EOF?
  if (iSectorCount == 0)
  {
    CLog::Log(LOGNOTICE, "file cdda: Read EoF");
    return 0; // Success, but nothing read
  } // Reached EoF

  // At leat one sector to read
  int retries;
  int iret;
  // Try reading a decresing number of sectors, then 3 times with 1 sector
  for (retries = 3; retries > 0; iSectorCount>1 ? iSectorCount-- : retries--)
  {
    iret = m_cdio->cdio_read_audio_sectors(m_pCdIo, destBuf, m_lsnCurrent, iSectorCount);
    if (iret == DRIVER_OP_SUCCESS)
      break; // Get out from the loop
    else
    {
      CLog::Log(LOGERROR, "file cdda: Read cdio error when reading track ");
    } // Errors when reading file
  }
  // retries == 0 only if failed reading at least one sector
  if (retries == 0)
  {
    CLog::Log(LOGERROR, "file cdda: Reading %d sectors of audio data starting at lsn %d failed with error code %i", iSectorCount, m_lsnCurrent, iret);
    return -1;
  }

  // Update position in file
  m_lsnCurrent += iSectorCount;

  // Was it a short request?
  if (uiBufSize < CDIO_CD_FRAMESIZE_RAW)
  {
    // We copy the amount if requested data into the destination buffer
    memcpy(lpBuf, m_TrackBuf, uiBufSize);
    // and keep track of the first available data
    p_TrackBuf = uiBufSize;
    // Finally, we set the buffer flag as TRUE
    f_TrackBuf = true;
    // We will return uiBufSize
    return uiBufSize;
  }

  // Otherwise, just return the size of read data
  return iSectorCount*CDIO_CD_FRAMESIZE_RAW;
}

int64_t CFileCDDA::Seek(int64_t iFilePosition, int iWhence /*=SEEK_SET*/)
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
  default:
    return -1;
  }

  return ((int64_t)(m_lsnCurrent -m_lsnStart)*CDIO_CD_FRAMESIZE_RAW);
}

void CFileCDDA::Close()
{
  // Flag TrackBuffer = FALSE, TrackBuffer is empty
  f_TrackBuf = 0;

  if (m_pCdIo)
  {
    m_cdio->cdio_destroy(m_pCdIo);
    m_pCdIo = NULL;
  }
}

int64_t CFileCDDA::GetPosition()
{
  if (!m_pCdIo)
    return 0;

  return ((int64_t)(m_lsnCurrent -m_lsnStart)*CDIO_CD_FRAMESIZE_RAW);
}

int64_t CFileCDDA::GetLength()
{
  if (!m_pCdIo)
    return 0;

  return ((int64_t)(m_lsnEnd -m_lsnStart)*CDIO_CD_FRAMESIZE_RAW);
}

bool CFileCDDA::IsValidFile(const CURL& url)
{
  // Only .cdda files are supported
  return URIUtils::HasExtension(url.Get(), ".cdda");
}

int CFileCDDA::GetTrackNum(const CURL& url)
{
  std::string strFileName = url.Get();

  // get track number from "cdda://local/01.cdda"
  return atoi(strFileName.substr(13, strFileName.size() - 13 - 5).c_str());
}

#define SECTOR_COUNT 52 // max. sectors that can be read at once
int CFileCDDA::GetChunkSize()
{
  return SECTOR_COUNT*CDIO_CD_FRAMESIZE_RAW;
}
