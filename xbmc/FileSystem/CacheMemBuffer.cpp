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
#include "stdafx.h"
#ifdef _LINUX
#include "../linux/PlatformDefs.h"
#endif
#include "CacheMemBuffer.h"
#include "utils/log.h"
#include "utils/SingleLock.h"

#include <math.h>

#define CACHE_BUFFER_SIZE (1048576 * 5)

using namespace XFILE;

#define SEEK_CHECK_RET(x) if (!(x)) return -1;

CacheMemBuffer::CacheMemBuffer()
 : CCacheStrategy()
{
  m_nStartPosition = 0;
  m_buffer.Create(CACHE_BUFFER_SIZE + 1);
  m_HistoryBuffer.Create(CACHE_BUFFER_SIZE + 1);
  m_forwardBuffer.Create(CACHE_BUFFER_SIZE + 1);
}


CacheMemBuffer::~CacheMemBuffer()
{
  m_buffer.Destroy();
  m_HistoryBuffer.Destroy();
  m_forwardBuffer.Destroy();
}

int CacheMemBuffer::Open()
{
  m_nStartPosition = 0;
  m_buffer.Clear();
  m_HistoryBuffer.Clear();
  m_forwardBuffer.Clear();
  return CACHE_RC_OK;
}

int CacheMemBuffer::Close()
{
  m_buffer.Clear();
  m_HistoryBuffer.Clear();
  m_forwardBuffer.Clear();
  return CACHE_RC_OK;
}

int CacheMemBuffer::WriteToCache(const char *pBuffer, size_t iSize)
{
  CSingleLock lock(m_sync);
  unsigned int nToWrite = m_buffer.GetMaxWriteSize() ;

  // must also check the forward buffer.
  // if we have leftovers from the previous seek - we need not read anymore until they are utilized
  if (nToWrite == 0 || m_forwardBuffer.GetMaxReadSize() > 0)
    return 0;

  if (nToWrite > iSize)
    nToWrite = iSize;

  if (!m_buffer.WriteBinary(pBuffer, nToWrite))
  {
    CLog::Log(LOGWARNING,"%s, failed to write %d bytes to buffer. max buffer size: %d", __FUNCTION__, nToWrite, m_buffer.GetMaxWriteSize());
    nToWrite = 0;
  }

  return nToWrite;
}

int CacheMemBuffer::ReadFromCache(char *pBuffer, size_t iMaxSize)
{
  CSingleLock lock(m_sync);
  if ( m_buffer.GetMaxReadSize() == 0 ) {
    return m_bEndOfInput?CACHE_RC_EOF : CACHE_RC_WOULD_BLOCK;
  }

  int nRead = iMaxSize;
  if ((size_t) m_buffer.GetMaxReadSize() < iMaxSize)
    nRead = m_buffer.GetMaxReadSize();

  if (nRead > 0)
  {
    if (!m_buffer.ReadBinary(pBuffer, nRead))
    {
      CLog::Log(LOGWARNING, "%s, failed to read %d bytes from buffer. max read size: %d", __FUNCTION__, nRead, m_buffer.GetMaxReadSize());
      return 0;
    }

    // copy to history so we can seek back
    if ((int) m_HistoryBuffer.GetMaxWriteSize() < nRead)
      m_HistoryBuffer.SkipBytes(nRead);
    m_HistoryBuffer.WriteBinary(pBuffer, nRead);

    m_nStartPosition += nRead;
  }

  // check forward buffer and copy it when enough space is available
  if (m_forwardBuffer.GetMaxReadSize() > 0 && m_buffer.GetMaxWriteSize() >= m_forwardBuffer.GetMaxReadSize())
  {
    m_buffer.Append(m_forwardBuffer);
    m_forwardBuffer.Clear();
  }

  return nRead;
}

__int64 CacheMemBuffer::WaitForData(unsigned int iMinAvail, unsigned int iMillis)
{
  if (iMillis == 0 || IsEndOfInput())
    return m_buffer.GetMaxReadSize();

  DWORD dwTime = GetTickCount() + iMillis;
  while (!IsEndOfInput() && (unsigned int) m_buffer.GetMaxReadSize() < iMinAvail && GetTickCount() < dwTime )
    Sleep(50); // may miss the deadline. shouldn't be a problem.

  return m_buffer.GetMaxReadSize();
}

__int64 CacheMemBuffer::Seek(__int64 iFilePosition, int iWhence)
{
  if (iWhence != SEEK_SET)
  {
    // sanity. we should always get here with SEEK_SET
    CLog::Log(LOGERROR, "%s, only SEEK_SET supported.", __FUNCTION__);
    return CACHE_RC_ERROR;
  }

  CSingleLock lock(m_sync);

  // if seek is a bit over what we have, try to wait a few seconds for the data to be available.
  // we try to avoid a (heavy) seek on the source
  if (iFilePosition > m_nStartPosition + m_buffer.GetMaxReadSize() &&
      iFilePosition < m_nStartPosition + m_buffer.GetMaxReadSize() + 100000)
  {
    int nRequired = (int)(iFilePosition - (m_nStartPosition + m_buffer.GetMaxReadSize()));
    lock.Leave();
    WaitForData(nRequired + 1, 5000);
    lock.Enter();
  }

  // check if seek is inside the current buffer
  if (iFilePosition >= m_nStartPosition && iFilePosition < m_nStartPosition + m_buffer.GetMaxReadSize())
  {
    int nOffset = (int)(iFilePosition - m_nStartPosition);
    // copy to history so we can seek back
    if (m_HistoryBuffer.GetMaxWriteSize() < nOffset)
      m_HistoryBuffer.SkipBytes(nOffset);

    if (!m_buffer.ReadBinary(m_HistoryBuffer, nOffset))
    {
      CLog::Log(LOGERROR, "%s, failed to copy %d bytes to history", __FUNCTION__, nOffset);
    }

    m_nStartPosition = iFilePosition;
    return m_nStartPosition;
  }

  __int64 iHistoryStart = m_nStartPosition - m_HistoryBuffer.GetMaxReadSize();
  if (iFilePosition < m_nStartPosition && iFilePosition > iHistoryStart)
  {
    CRingBuffer saveHist, saveUnRead;
    __int64 nToSkip = iFilePosition - iHistoryStart;
    SEEK_CHECK_RET(m_HistoryBuffer.ReadBinary(saveHist, (int)nToSkip));

    SEEK_CHECK_RET(saveUnRead.Copy(m_buffer));

    SEEK_CHECK_RET(m_buffer.Copy(m_HistoryBuffer));
    int nSpace = m_buffer.GetMaxWriteSize();
    int nToCopy = saveUnRead.GetMaxReadSize();

    if (nToCopy < nSpace)
      nSpace = nToCopy;

    SEEK_CHECK_RET(saveUnRead.ReadBinary(m_buffer, nSpace));
    nToCopy -= nSpace;
    if (nToCopy > 0)
      m_forwardBuffer.Copy(saveUnRead);

    SEEK_CHECK_RET(m_HistoryBuffer.Copy(saveHist));
    m_HistoryBuffer.Clear();

    m_nStartPosition = iFilePosition;
    return m_nStartPosition;
  }

  // seek outside the buffer. return error.
  return CACHE_RC_ERROR;
}

void CacheMemBuffer::Reset(__int64 iSourcePosition)
{
  CSingleLock lock(m_sync);
  m_nStartPosition = iSourcePosition;
  m_buffer.Clear();
  m_HistoryBuffer.Clear();
  m_forwardBuffer.Clear();
}


