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

#ifdef _LINUX
#include "threads/SystemClock.h"
#include "linux/PlatformDefs.h"
#endif
#include "settings/AdvancedSettings.h"
#include "MemBufferCache.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"

#include <math.h>

using namespace XFILE;

#define SEEK_CHECK_RET(x) if (!(x)) return -1;

MemBufferCache::MemBufferCache()
 : CCacheStrategy()
{
  m_nStartPosition = 0;
  m_buffer.Create(g_advancedSettings.m_cacheMemBufferSize + 1);
  m_HistoryBuffer.Create(g_advancedSettings.m_cacheMemBufferSize + 1);
  m_forwardBuffer.Create(g_advancedSettings.m_cacheMemBufferSize + 1);
}


MemBufferCache::~MemBufferCache()
{
  m_buffer.Destroy();
  m_HistoryBuffer.Destroy();
  m_forwardBuffer.Destroy();
}

int MemBufferCache::Open()
{
  m_nStartPosition = 0;
  m_buffer.Clear();
  m_HistoryBuffer.Clear();
  m_forwardBuffer.Clear();
  return CACHE_RC_OK;
}

void MemBufferCache::Close()
{
  m_buffer.Clear();
  m_HistoryBuffer.Clear();
  m_forwardBuffer.Clear();
}

int MemBufferCache::WriteToCache(const char *pBuffer, size_t iSize)
{
  CSingleLock lock(m_sync);
  unsigned int nToWrite = m_buffer.getMaxWriteSize() ;

  // must also check the forward buffer.
  // if we have leftovers from the previous seek - we need not read anymore until they are utilized
  if (nToWrite == 0 || m_forwardBuffer.getMaxReadSize() > 0)
    return 0;

  if (nToWrite > iSize)
    nToWrite = iSize;

  if (!m_buffer.WriteData((char*)pBuffer, nToWrite))
  {
    CLog::Log(LOGWARNING,"%s, failed to write %d bytes to buffer. max buffer size: %d", __FUNCTION__, nToWrite, m_buffer.getMaxWriteSize());
    nToWrite = 0;
  }

  m_written.Set();

  return nToWrite;
}

int MemBufferCache::ReadFromCache(char *pBuffer, size_t iMaxSize)
{
  CSingleLock lock(m_sync);
  if ( m_buffer.getMaxReadSize() == 0 ) {
    return m_bEndOfInput ? 0 : CACHE_RC_WOULD_BLOCK;
  }

  int nRead = iMaxSize;
  if ((size_t) m_buffer.getMaxReadSize() < iMaxSize)
    nRead = m_buffer.getMaxReadSize();

  if (nRead > 0)
  {
    if (!m_buffer.ReadData(pBuffer, nRead))
    {
      CLog::Log(LOGWARNING, "%s, failed to read %d bytes from buffer. max read size: %d", __FUNCTION__, nRead, m_buffer.getMaxReadSize());
      return 0;
    }

    // copy to history so we can seek back
    if ((int) m_HistoryBuffer.getMaxWriteSize() < nRead)
      m_HistoryBuffer.SkipBytes(nRead);
    m_HistoryBuffer.WriteData(pBuffer, nRead);

    m_nStartPosition += nRead;
  }

  // check forward buffer and copy it when enough space is available
  if (m_forwardBuffer.getMaxReadSize() > 0 && m_buffer.getMaxWriteSize() >= m_forwardBuffer.getMaxReadSize())
  {
    m_buffer.Append(m_forwardBuffer);
    m_forwardBuffer.Clear();
  }

  if (nRead > 0)
    m_space.Set();

  return nRead;
}

int64_t MemBufferCache::WaitForData(unsigned int iMinAvail, unsigned int millis)
{
  if (millis == 0 || IsEndOfInput())
    return m_buffer.getMaxReadSize();

  XbmcThreads::EndTime endTime(millis);
  while (!IsEndOfInput() && (unsigned int) m_buffer.getMaxReadSize() < iMinAvail && !endTime.IsTimePast() )
    m_written.WaitMSec(50); // may miss the deadline. shouldn't be a problem.

  return m_buffer.getMaxReadSize();
}

int64_t MemBufferCache::Seek(int64_t iFilePosition)
{
  CSingleLock lock(m_sync);

  // if seek is a bit over what we have, try to wait a few seconds for the data to be available.
  // we try to avoid a (heavy) seek on the source
  if (iFilePosition > m_nStartPosition + m_buffer.getMaxReadSize() &&
      iFilePosition < m_nStartPosition + m_buffer.getMaxReadSize() + 100000)
  {
    int nRequired = (int)(iFilePosition - (m_nStartPosition + m_buffer.getMaxReadSize()));
    lock.Leave();
    WaitForData(nRequired + 1, 5000);
    lock.Enter();
  }

  // check if seek is inside the current buffer
  if (iFilePosition >= m_nStartPosition && iFilePosition < m_nStartPosition + m_buffer.getMaxReadSize())
  {
    unsigned int nOffset = (unsigned int)(iFilePosition - m_nStartPosition);
    // copy to history so we can seek back
    if (m_HistoryBuffer.getMaxWriteSize() < nOffset)
      m_HistoryBuffer.SkipBytes(nOffset);

    if (!m_buffer.ReadData(m_HistoryBuffer, nOffset))
    {
      CLog::Log(LOGERROR, "%s, failed to copy %d bytes to history", __FUNCTION__, nOffset);
    }

    m_nStartPosition = iFilePosition;
    m_space.Set();
    return m_nStartPosition;
  }

  int64_t iHistoryStart = m_nStartPosition - m_HistoryBuffer.getMaxReadSize();
  if (iFilePosition < m_nStartPosition && iFilePosition >= iHistoryStart)
  {
    CRingBuffer saveHist, saveUnRead;
    int64_t nToSkip = iFilePosition - iHistoryStart;
    SEEK_CHECK_RET(m_HistoryBuffer.ReadData(saveHist, (int)nToSkip));

    SEEK_CHECK_RET(saveUnRead.Copy(m_buffer));

    SEEK_CHECK_RET(m_buffer.Copy(m_HistoryBuffer));
    int nSpace = m_buffer.getMaxWriteSize();
    int nToCopy = saveUnRead.getMaxReadSize();

    if (nToCopy < nSpace)
      nSpace = nToCopy;

    SEEK_CHECK_RET(saveUnRead.ReadData(m_buffer, nSpace));
    nToCopy -= nSpace;
    if (nToCopy > 0)
      m_forwardBuffer.Copy(saveUnRead);

    m_nStartPosition = iFilePosition;
    m_space.Set();
    return m_nStartPosition;
  }

  // seek outside the buffer. return error.
  return CACHE_RC_ERROR;
}

void MemBufferCache::Reset(int64_t iSourcePosition)
{
  CSingleLock lock(m_sync);
  m_nStartPosition = iSourcePosition;
  m_buffer.Clear();
  m_HistoryBuffer.Clear();
  m_forwardBuffer.Clear();
}


