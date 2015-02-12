/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include <algorithm>
#include "threads/SystemClock.h"
#include "system.h"
#include "utils/log.h"
#include "threads/SingleLock.h"
#include "utils/TimeUtils.h"
#include "CircularCache.h"

using namespace XFILE;

CCircularCache::CCircularCache(size_t front, size_t back)
 : CCacheStrategy()
 , m_beg(0)
 , m_end(0)
 , m_cur(0)
 , m_buf(NULL)
 , m_size(front + back)
 , m_size_back(back)
#ifdef TARGET_WINDOWS
 , m_handle(INVALID_HANDLE_VALUE)
#endif
{
}

CCircularCache::~CCircularCache()
{
  Close();
}

int CCircularCache::Open()
{
#ifdef TARGET_WINDOWS
  m_handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, m_size, NULL);
  if(m_handle == NULL)
    return CACHE_RC_ERROR;
  m_buf = (uint8_t*)MapViewOfFile(m_handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#else
  m_buf = new uint8_t[m_size];
#endif
  if(m_buf == 0)
    return CACHE_RC_ERROR;
  m_beg = 0;
  m_end = 0;
  m_cur = 0;
  return CACHE_RC_OK;
}

void CCircularCache::Close()
{
#ifdef TARGET_WINDOWS
  UnmapViewOfFile(m_buf);
  CloseHandle(m_handle);
  m_handle = INVALID_HANDLE_VALUE;
#else
  delete[] m_buf;
#endif
  m_buf = NULL;
}

size_t CCircularCache::GetMaxWriteSize(const size_t& iRequestSize)
{
  CSingleLock lock(m_sync);

  size_t back  = (size_t)(m_cur - m_beg); // Backbuffer size
  size_t front = (size_t)(m_end - m_cur); // Frontbuffer size
  size_t limit = m_size - std::min(back, m_size_back) - front;

  // Never return more than limit and size requested by caller
  return std::min(iRequestSize, limit);
}

/**
 * Function will write to m_buf at m_end % m_size location
 * it will write at maximum m_size, but it will only write
 * as much it can without wrapping around in the buffer
 *
 * It will always leave m_size_back of the backbuffer intact
 * but if the back buffer is less than that, that space is
 * usable to write.
 *
 * If back buffer is filled to an larger extent than
 * m_size_back, it will allow it to be overwritten
 * until only m_size_back data remains.
 *
 * The following always apply:
 *  * m_end <= m_cur <= m_end
 *  * m_end - m_beg <= m_size
 *
 * Multiple calls may be needed to fill buffer completely.
 */
int CCircularCache::WriteToCache(const char *buf, size_t len)
{
  CSingleLock lock(m_sync);

  // where are we in the buffer
  size_t pos   = m_end % m_size;
  size_t back  = (size_t)(m_cur - m_beg);
  size_t front = (size_t)(m_end - m_cur);

  size_t limit = m_size - std::min(back, m_size_back) - front;
  size_t wrap  = m_size - pos;

  // limit by max forward size
  if(len > limit)
    len = limit;

  // limit to wrap point
  if(len > wrap)
    len = wrap;

  if(len == 0)
    return 0;

  // write the data
  memcpy(m_buf + pos, buf, len);
  m_end += len;

  // drop history that was overwritten
  if(m_end - m_beg > (int64_t)m_size)
    m_beg = m_end - m_size;

  m_written.Set();

  return len;
}

/**
 * Reads data from cache. Will only read up till
 * the buffer wrap point. So multiple calls
 * may be needed to empty the whole cache
 */
int CCircularCache::ReadFromCache(char *buf, size_t len)
{
  CSingleLock lock(m_sync);

  size_t pos   = m_cur % m_size;
  size_t front = (size_t)(m_end - m_cur);
  size_t avail = std::min(m_size - pos, front);

  if(avail == 0)
  {
    if(IsEndOfInput())
      return 0;
    else
      return CACHE_RC_WOULD_BLOCK;
  }

  if(len > avail)
    len = avail;

  if(len == 0)
    return 0;

  memcpy(buf, m_buf + pos, len);
  m_cur += len;

  m_space.Set();

  return len;
}

/* Wait "millis" milliseconds for "minimum" amount of data to come in.
 * Note that caller needs to make sure there's sufficient space in the forward
 * buffer for "minimum" bytes else we may block the full timeout time
 */
int64_t CCircularCache::WaitForData(unsigned int minimum, unsigned int millis)
{
  CSingleLock lock(m_sync);
  int64_t avail = m_end - m_cur;

  if(millis == 0 || IsEndOfInput())
    return avail;

  if(minimum > m_size - m_size_back)
    minimum = m_size - m_size_back;

  XbmcThreads::EndTime endtime(millis);
  while (!IsEndOfInput() && avail < minimum && !endtime.IsTimePast() )
  {
    lock.Leave();
    m_written.WaitMSec(50); // may miss the deadline. shouldn't be a problem.
    lock.Enter();
    avail = m_end - m_cur;
  }

  return avail;
}

int64_t CCircularCache::Seek(int64_t pos)
{
  CSingleLock lock(m_sync);

  // if seek is a bit over what we have, try to wait a few seconds for the data to be available.
  // we try to avoid a (heavy) seek on the source
  if (pos >= m_end && pos < m_end + 100000)
  {
    /* Make everything in the cache (back & forward) back-cache, to make sure
     * there's sufficient forward space. Increasing it with only 100000 may not be
     * sufficient due to variable filesystem chunksize
     */
    m_cur = m_end;
    lock.Leave();
    WaitForData((size_t)(pos - m_cur), 5000);
    lock.Enter();
  }

  if(pos >= m_beg && pos <= m_end)
  {
    m_cur = pos;
    return pos;
  }

  return CACHE_RC_ERROR;
}

bool CCircularCache::Reset(int64_t pos, bool clearAnyway)
{
  CSingleLock lock(m_sync);
  if (!clearAnyway && IsCachedPosition(pos))
  {
    m_cur = pos;
    return false;
  }
  m_end = pos;
  m_beg = pos;
  m_cur = pos;

  return true;
}

int64_t CCircularCache::CachedDataEndPosIfSeekTo(int64_t iFilePosition)
{
  if (IsCachedPosition(iFilePosition))
    return m_end;
  return iFilePosition;
}

int64_t CCircularCache::CachedDataEndPos()
{
  return m_end;
}

bool CCircularCache::IsCachedPosition(int64_t iFilePosition)
{
  return iFilePosition >= m_beg && iFilePosition <= m_end;
}

CCacheStrategy *CCircularCache::CreateNew()
{
  return new CCircularCache(m_size - m_size_back, m_size_back);
}

