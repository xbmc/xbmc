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

#include "threads/SystemClock.h"
#include "utils/AutoPtrHandle.h"
#include "FileCache.h"
#include "threads/Thread.h"
#include "File.h"
#include "URL.h"

#include "CacheCircular.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "settings/AdvancedSettings.h"

using namespace AUTOPTR;
using namespace XFILE;

#define READ_CACHE_CHUNK_SIZE (64*1024)

class CWriteRate
{
public:
  CWriteRate()
  {
    m_stamp = XbmcThreads::SystemClockMillis();
    m_pos   = 0;
    m_pause = 0;
  }

  void Reset(int64_t pos)
  {
    m_stamp = XbmcThreads::SystemClockMillis();
    m_pos   = pos;
  }

  unsigned Rate(int64_t pos, unsigned int time_bias = 0)
  {
    const unsigned ts = XbmcThreads::SystemClockMillis() + time_bias;
    if (ts == m_stamp)
      return 0;
    return (unsigned)(1000 * (pos - m_pos) / (ts - m_stamp));
  }

  void Pause()
  {
    m_pause = XbmcThreads::SystemClockMillis();
  }

  void Resume()
  {
    m_stamp += XbmcThreads::SystemClockMillis() - m_pause;
    m_pause  = 0;
  }

private:
  unsigned m_stamp;
  int64_t  m_pos;
  unsigned m_pause;
};


CFileCache::CFileCache()
{
   m_bDeleteCache = true;
   m_nSeekResult = 0;
   m_seekPos = 0;
   m_readPos = 0;
   m_writePos = 0;
   if (g_advancedSettings.m_cacheMemBufferSize == 0)
     m_pCache = new CSimpleFileCache();
   else
     m_pCache = new CCacheCircular(g_advancedSettings.m_cacheMemBufferSize
                                 , std::max<unsigned int>( g_advancedSettings.m_cacheMemBufferSize / 4, 1024 * 1024));
   m_seekPossible = 0;
   m_cacheFull = false;
}

CFileCache::CFileCache(CCacheStrategy *pCache, bool bDeleteCache)
{
  m_pCache = pCache;
  m_bDeleteCache = bDeleteCache;
  m_seekPos = 0;
  m_readPos = 0;
  m_writePos = 0;
  m_nSeekResult = 0;
}

CFileCache::~CFileCache()
{
  Close();

  if (m_bDeleteCache && m_pCache)
    delete m_pCache;

  m_pCache = NULL;
}

void CFileCache::SetCacheStrategy(CCacheStrategy *pCache, bool bDeleteCache)
{
  if (m_bDeleteCache && m_pCache)
    delete m_pCache;

  m_pCache = pCache;
  m_bDeleteCache = bDeleteCache;
}

IFile *CFileCache::GetFileImp()
{
  return m_source.GetImplemenation();
}

bool CFileCache::Open(const CURL& url)
{
  Close();

  CSingleLock lock(m_sync);

  CLog::Log(LOGDEBUG,"CFileCache::Open - opening <%s> using cache", url.GetFileName().c_str());

  if (!m_pCache)
  {
    CLog::Log(LOGERROR,"CFileCache::Open - no cache strategy defined");
    return false;
  }

  m_sourcePath = url.Get();

  // open cache strategy
  if (m_pCache->Open() != CACHE_RC_OK)
  {
    CLog::Log(LOGERROR,"CFileCache::Open - failed to open cache");
    Close();
    return false;
  }

  // opening the source file.
  if (!m_source.Open(m_sourcePath, READ_NO_CACHE | READ_TRUNCATED | READ_CHUNKED))
  {
    CLog::Log(LOGERROR,"%s - failed to open source <%s>", __FUNCTION__, m_sourcePath.c_str());
    Close();
    return false;
  }

  // check if source can seek
  m_seekPossible = m_source.IoControl(IOCTRL_SEEK_POSSIBLE, NULL);

  m_readPos = 0;
  m_writePos = 0;
  m_writeRate = 1024 * 1024;
  m_writeRateActual = 0;
  m_cacheFull = false;
  m_seekEvent.Reset();
  m_seekEnded.Reset();

  CThread::Create(false);

  return true;
}

void CFileCache::Process()
{
  if (!m_pCache)
  {
    CLog::Log(LOGERROR,"CFileCache::Process - sanity failed. no cache strategy");
    return;
  }

  // setup read chunks size
  int chunksize = CFile::GetChunkSize(m_source.GetChunkSize(), READ_CACHE_CHUNK_SIZE);

  // create our read buffer
  auto_aptr<char> buffer(new char[chunksize]);
  if (buffer.get() == NULL)
  {
    CLog::Log(LOGERROR, "%s - failed to allocate read buffer", __FUNCTION__);
    return;
  }

  CWriteRate limiter;
  CWriteRate average;

  while (!m_bStop)
  {
    // check for seek events
    if (m_seekEvent.WaitMSec(0))
    {
      m_seekEvent.Reset();
      CLog::Log(LOGDEBUG,"%s, request seek on source to %"PRId64, __FUNCTION__, m_seekPos);
      m_nSeekResult = m_source.Seek(m_seekPos, SEEK_SET);
      if (m_nSeekResult != m_seekPos)
      {
        CLog::Log(LOGERROR,"%s, error %d seeking. seek returned %"PRId64, __FUNCTION__, (int)GetLastError(), m_nSeekResult);
        m_seekPossible = m_source.IoControl(IOCTRL_SEEK_POSSIBLE, NULL);
      }
      else
      {
        m_pCache->Reset(m_seekPos);
        average.Reset(m_seekPos);
        limiter.Reset(m_seekPos);
        m_writePos = m_seekPos;
        m_readPos = m_seekPos;
        m_cacheFull = false;
      }

      m_seekEnded.Set();
    }

    while (m_writeRate)
    {
      if (m_writePos - m_readPos < m_writeRate)
      {
        limiter.Reset(m_writePos);
        break;
      }

      if (limiter.Rate(m_writePos) < m_writeRate)
        break;

      if (m_seekEvent.WaitMSec(100))
      {
        m_seekEvent.Set();
        break;
      }
    }

    int iRead = m_source.Read(buffer.get(), chunksize);
    if (iRead == 0)
    {
      CLog::Log(LOGINFO, "CFileCache::Process - Hit eof.");
      m_pCache->EndOfInput();

      // The thread event will now also cause the wait of an event to return a false.
      if (AbortableWait(m_seekEvent) == WAIT_SIGNALED)
      {
        m_pCache->ClearEndOfInput();
        m_seekEvent.Set(); // hack so that later we realize seek is needed
      }
      else
        break;
    }
    else if (iRead < 0)
      m_bStop = true;

    int iTotalWrite=0;
    while (!m_bStop && (iTotalWrite < iRead))
    {
      int iWrite = 0;
      iWrite = m_pCache->WriteToCache(buffer.get()+iTotalWrite, iRead - iTotalWrite);

      // write should always work. all handling of buffering and errors should be
      // done inside the cache strategy. only if unrecoverable error happened, WriteToCache would return error and we break.
      if (iWrite < 0)
      {
        CLog::Log(LOGERROR,"CFileCache::Process - error writing to cache");
        m_bStop = true;
        break;
      }
      else if (iWrite == 0)
      {
        m_cacheFull = true;
        average.Pause();
        m_pCache->m_space.WaitMSec(5);
        average.Resume();
      }
      else
        m_cacheFull = false;

      iTotalWrite += iWrite;

      // check if seek was asked. otherwise if cache is full we'll freeze.
      if (m_seekEvent.WaitMSec(0))
      {
        m_seekEvent.Set(); // make sure we get the seek event later.
        break;
      }
    }

    m_writePos += iTotalWrite;

    // under estimate write rate by a second, to
    // avoid uncertainty at start of caching
    m_writeRateActual = average.Rate(m_writePos, 1000);
  }
}

void CFileCache::OnExit()
{
  m_bStop = true;

  // make sure cache is set to mark end of file (read may be waiting).
  if (m_pCache)
    m_pCache->EndOfInput();

  // just in case someone's waiting...
  m_seekEnded.Set();
}

bool CFileCache::Exists(const CURL& url)
{
  return CFile::Exists(url.Get());
}

int CFileCache::Stat(const CURL& url, struct __stat64* buffer)
{
  return CFile::Stat(url.Get(), buffer);
}

unsigned int CFileCache::Read(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_sync);
  if (!m_pCache)
  {
    CLog::Log(LOGERROR,"%s - sanity failed. no cache strategy!", __FUNCTION__);
    return 0;
  }
  int64_t iRc;

retry:
  // attempt to read
  iRc = m_pCache->ReadFromCache((char *)lpBuf, (size_t)uiBufSize);
  if (iRc > 0)
  {
    m_readPos += iRc;
    return (int)iRc;
  }

  if (iRc == CACHE_RC_WOULD_BLOCK)
  {
    // just wait for some data to show up
    iRc = m_pCache->WaitForData(1, 10000);
    if (iRc > 0)
      goto retry;
  }

  if (iRc == CACHE_RC_TIMEOUT)
  {
    CLog::Log(LOGWARNING, "%s - timeout waiting for data", __FUNCTION__);
    return 0;
  }

  if (iRc == 0)
    return 0;

  // unknown error code
  CLog::Log(LOGERROR, "%s - cache strategy returned unknown error code %d", __FUNCTION__, (int)iRc);
  return 0;
}

int64_t CFileCache::Seek(int64_t iFilePosition, int iWhence)
{
  CSingleLock lock(m_sync);

  if (!m_pCache)
  {
    CLog::Log(LOGERROR,"%s - sanity failed. no cache strategy!", __FUNCTION__);
    return -1;
  }

  int64_t iCurPos = m_readPos;
  int64_t iTarget = iFilePosition;
  if (iWhence == SEEK_END)
    iTarget = GetLength() + iTarget;
  else if (iWhence == SEEK_CUR)
    iTarget = iCurPos + iTarget;
  else if (iWhence != SEEK_SET)
    return -1;

  if (iTarget == m_readPos)
    return m_readPos;

  if ((m_nSeekResult = m_pCache->Seek(iTarget)) != iTarget)
  {
    if (m_seekPossible == 0)
      return m_nSeekResult;

    m_seekPos = iTarget;
    m_seekEvent.Set();
    if (!m_seekEnded.Wait())
    {
      CLog::Log(LOGWARNING,"%s - seek to %"PRId64" failed.", __FUNCTION__, m_seekPos);
      return -1;
    }
    m_seekEvent.Reset();
  }
  else
    m_readPos = iTarget;

  return m_nSeekResult;
}

void CFileCache::Close()
{
  StopThread();

  CSingleLock lock(m_sync);
  if (m_pCache)
    m_pCache->Close();

  m_source.Close();
}

int64_t CFileCache::GetPosition()
{
  return m_readPos;
}

int64_t CFileCache::GetLength()
{
  return m_source.GetLength();
}

void CFileCache::StopThread(bool bWait /*= true*/)
{
  m_bStop = true;
  //Process could be waiting for seekEvent
  m_seekEvent.Set();
  CThread::StopThread(bWait);
}

CStdString CFileCache::GetContent()
{
  if (!m_source.GetImplemenation())
    return IFile::GetContent();

  return m_source.GetImplemenation()->GetContent();
}

int CFileCache::IoControl(EIoControl request, void* param)
{
  if (request == IOCTRL_CACHE_STATUS)
  {
    SCacheStatus* status = (SCacheStatus*)param;
    status->forward = m_pCache->WaitForData(0, 0);
    status->maxrate = m_writeRate;
    status->currate = m_writeRateActual;
    status->full    = m_cacheFull;
    return 0;
  }

  if (request == IOCTRL_CACHE_SETRATE)
  {
    m_writeRate = *(unsigned*)param;
    return 0;
  }

  if (request == IOCTRL_SEEK_POSSIBLE)
    return m_seekPossible;

  return -1;
}
