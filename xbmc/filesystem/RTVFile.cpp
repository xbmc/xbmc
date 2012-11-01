/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// FileRTV.cpp: implementation of the CRTVFile class.
//
//////////////////////////////////////////////////////////////////////

#include "RTVFile.h"
#include "URL.h"
#include "utils/log.h"
#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#include "PlatformDefs.h" //for PRIdS
#endif
extern "C"
{
#include "lib/libRTV/interface.h"
}


using namespace XFILE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRTVFile::CRTVFile()
{
  m_filePos = 0;
  m_fileSize = 0;
  m_bOpened = false;
  m_rtvd = NULL;
}

CRTVFile::~CRTVFile()
{
  Close();
}

//*********************************************************************************************
bool CRTVFile::Open(const char* strHostName, const char* strFileName, int iport)
{
  // Close any existing connection
  if (m_bOpened) Close();

  m_bOpened = false;

  // Set up global variables.  Don't set m_filePos to 0 because we use it to SEEK!
  m_fileSize = 0;
  m_rtvd = NULL;
  strcpy(m_hostName, strHostName);
  strcpy(m_fileName, strFileName);
  m_iport = iport;

  // Allow for ReplayTVs on ports other than 80
  CStdString strHostAndPort;
  strHostAndPort = strHostName;
  if (iport)
  {
    char buffer[10];
    sprintf(buffer,"%i",iport);
    strHostAndPort += ':';
    strHostAndPort += buffer;
  }

  // Get the file size of strFileName.  If size is 0 or negative, file doesn't exist so exit.
  u64 size;
  size = rtv_get_filesize(strHostAndPort.c_str(), strFileName);
  if (!size)
  {
    CLog::Log(LOGERROR, "%s - Failed to get filesize of %s on %s", __FUNCTION__, strHostName, strFileName);
    return false;
  }
  m_fileSize = size;

  // Open a connection to strFileName stating at position m_filePos
  // Store the handle to the connection in m_rtvd.  Exit if handle invalid.
  m_rtvd = rtv_open_file(strHostAndPort.c_str(), strFileName, m_filePos);
  if (!m_rtvd)
  {
    CLog::Log(LOGERROR, "%s - Failed to open %s on %s", __FUNCTION__, strHostName, strFileName);
    return false;
  }
  m_bOpened = true;

  CLog::Log(LOGDEBUG, "%s - Opened %s on %s, Size %"PRIu64", Position %"PRIu64"", __FUNCTION__, strHostName, strFileName, m_fileSize, m_filePos);
  return true;
}

bool CRTVFile::Open(const CURL& url)
{
  return Open(url.GetHostName(), url.GetFileName(), url.GetPort());
}

bool CRTVFile::Exists(const CURL& url)
{
  return true;
}

int CRTVFile::Stat(const CURL& url, struct __stat64* buffer)
{
  errno = ENOENT;
  return -1;
}

//*********************************************************************************************
unsigned int CRTVFile::Read(void *lpBuf, int64_t uiBufSize)
{
  size_t lenread;

  // Don't read if no connection is open!
  if (!m_bOpened) return 0;

  // Read uiBufSize bytes from the m_rtvd connection
  lenread = rtv_read_file(m_rtvd, (char *) lpBuf, (size_t) uiBufSize);

  CLog::Log(LOGDEBUG, "%s - Requested %"PRIdS", Recieved %"PRIdS"", __FUNCTION__, (size_t)uiBufSize, lenread);

  // Some extra checking so library behaves
  if(m_filePos + lenread > m_fileSize)
  {
    CLog::Log(LOGWARNING, "%s - RTV library read passed filesize, returning last chunk", __FUNCTION__);
    lenread = (size_t)(m_fileSize - m_filePos);
    m_filePos = m_fileSize;
    return lenread;
  }

  // Increase the file position by the number of bytes we just read
  m_filePos += lenread;

  // Return the number of bytes we just read
  return lenread;
}

//*********************************************************************************************
void CRTVFile::Close()
{
  m_bOpened = false;

  // Only try to close a valid handle!
  if (m_rtvd)
  {
    rtv_close_file(m_rtvd);
  }
  m_rtvd = NULL;
}

//*********************************************************************************************
int64_t CRTVFile::Seek(int64_t iFilePosition, int iWhence)
{
  UINT64 newpos;

  if (!m_bOpened) return 0;
  switch (iWhence)
  {
  case SEEK_SET:
    // cur = pos
    newpos = iFilePosition;
    break;
  case SEEK_CUR:
    // cur += pos
    newpos = m_filePos + iFilePosition;
    break;
  case SEEK_END:
    // end += pos
    newpos = m_fileSize + iFilePosition;
    break;
  default:
    return -1;
  }
  // Return offset from beginning
  if (newpos > m_fileSize) newpos = m_fileSize;

  // NEW CODE
  // If the new file position is different from the old, then we must SEEK there!
  if (m_filePos != newpos)
  {
    m_filePos = newpos;
    Open(m_hostName, m_fileName, m_iport);
  }

  // OLD CODE
  // Below is old code that may be useful again.  For some reason I'm not sure of, XBMC
  // does a few seeks to the beginning and end of the stream before it ever starts playing.
  // When used instead of the code above, this code seeks only after playing begins.
  // Theoretically, this saves some time because it prevents needless re-opening of the
  // connection to the RTV, but in practice it seems to be the same.
  //m_filePos = newpos;

  //if (m_rtvd->firstReadDone && iWhence == SEEK_SET) {
  // Open(NULL, NULL, m_hostName, m_fileName, m_iport, true);
  //}

  // Return the new file position after the seek
  return m_filePos;
}

//*********************************************************************************************
int64_t CRTVFile::GetLength()
{
  if (!m_bOpened) return 0;
  return m_fileSize;
}

//*********************************************************************************************
int64_t CRTVFile::GetPosition()
{
  if (!m_bOpened) return 0;
  return m_filePos;
}
