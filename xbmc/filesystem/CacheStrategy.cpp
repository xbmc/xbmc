/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "threads/SystemClock.h"
#include "CacheStrategy.h"
#include "IFile.h"
#ifdef TARGET_POSIX
#include "PlatformDefs.h"
#include "platform/posix/ConvUtils.h"
#endif
#include "Util.h"
#include "utils/log.h"
#include "SpecialProtocol.h"
#include "URL.h"
#if defined(TARGET_POSIX)
#include "platform/posix/filesystem/PosixFile.h"
#define CacheLocalFile CPosixFile
#elif defined(TARGET_WINDOWS)
#include "platform/win32/filesystem/Win32File.h"
#define CacheLocalFile CWin32File
#endif // TARGET_WINDOWS

#include <cassert>
#include <algorithm>

using namespace XFILE;

CCacheStrategy::~CCacheStrategy() = default;

void CCacheStrategy::EndOfInput() {
  m_bEndOfInput = true;
}

bool CCacheStrategy::IsEndOfInput()
{
  return m_bEndOfInput;
}

void CCacheStrategy::ClearEndOfInput()
{
  m_bEndOfInput = false;
}

CSimpleFileCache::CSimpleFileCache()
  : m_cacheFileRead(new CacheLocalFile())
  , m_cacheFileWrite(new CacheLocalFile())
  , m_hDataAvailEvent(NULL)
{
}

CSimpleFileCache::~CSimpleFileCache()
{
  Close();
  delete m_cacheFileRead;
  delete m_cacheFileWrite;
}

int CSimpleFileCache::Open()
{
  Close();

  m_hDataAvailEvent = new CEvent;

  m_filename = CSpecialProtocol::TranslatePath(
      CUtil::GetNextFilename("special://temp/filecache{:03}.cache", 999));
  if (m_filename.empty())
  {
    CLog::Log(LOGERROR, "CSimpleFileCache::{} - Unable to generate a new filename", __FUNCTION__);
    Close();
    return CACHE_RC_ERROR;
  }

  CURL fileURL(m_filename);

  if (!m_cacheFileWrite->OpenForWrite(fileURL, false))
  {
    CLog::Log(LOGERROR, "CSimpleFileCache::{} - Failed to create file \"{}\" for writing",
              __FUNCTION__, m_filename);
    Close();
    return CACHE_RC_ERROR;
  }

  if (!m_cacheFileRead->Open(fileURL))
  {
    CLog::Log(LOGERROR, "CSimpleFileCache::{} - Failed to open file \"{}\" for reading",
              __FUNCTION__, m_filename);
    Close();
    return CACHE_RC_ERROR;
  }

  return CACHE_RC_OK;
}

void CSimpleFileCache::Close()
{
  if (m_hDataAvailEvent)
    delete m_hDataAvailEvent;

  m_hDataAvailEvent = NULL;

  m_cacheFileWrite->Close();
  m_cacheFileRead->Close();

  if (!m_filename.empty() && !m_cacheFileRead->Delete(CURL(m_filename)))
    CLog::Log(LOGWARNING, "SimpleFileCache::{} - Failed to delete cache file \"{}\"", __FUNCTION__,
              m_filename);

  m_filename.clear();
}

size_t CSimpleFileCache::GetMaxWriteSize(const size_t& iRequestSize)
{
  return iRequestSize; // Can always write since it's on disk
}

int CSimpleFileCache::WriteToCache(const char *pBuffer, size_t iSize)
{
  size_t written = 0;
  while (iSize > 0)
  {
    const ssize_t lastWritten =
        m_cacheFileWrite->Write(pBuffer, std::min(iSize, static_cast<size_t>(SSIZE_MAX)));
    if (lastWritten <= 0)
    {
      CLog::Log(LOGERROR, "SimpleFileCache::{} - <{}> Failed to write to cache", __FUNCTION__,
                m_filename);
      return CACHE_RC_ERROR;
    }
    m_nWritePosition += lastWritten;
    iSize -= lastWritten;
    written += lastWritten;
  }

  // when reader waits for data it will wait on the event.
  m_hDataAvailEvent->Set();

  return written;
}

int64_t CSimpleFileCache::GetAvailableRead()
{
  return m_nWritePosition - m_nReadPosition;
}

int CSimpleFileCache::ReadFromCache(char *pBuffer, size_t iMaxSize)
{
  int64_t iAvailable = GetAvailableRead();
  if ( iAvailable <= 0 )
    return m_bEndOfInput ? 0 : CACHE_RC_WOULD_BLOCK;

  size_t toRead = std::min(iMaxSize, static_cast<size_t>(iAvailable));

  size_t readBytes = 0;
  while (toRead > 0)
  {
    const ssize_t lastRead =
      m_cacheFileRead->Read(pBuffer, std::min(toRead, static_cast<size_t>(SSIZE_MAX)));

    if (lastRead == 0)
      break;
    if (lastRead < 0)
    {
      CLog::Log(LOGERROR, "CSimpleFileCache::{} - <{}> Failed to read from cache", __FUNCTION__,
                m_filename);
      return CACHE_RC_ERROR;
    }
    m_nReadPosition += lastRead;
    toRead -= lastRead;
    readBytes += lastRead;
  }

  if (readBytes > 0)
    m_space.Set();

  return readBytes;
}

int64_t CSimpleFileCache::WaitForData(unsigned int iMinAvail, unsigned int iMillis)
{
  if( iMillis == 0 || IsEndOfInput() )
    return GetAvailableRead();

  XbmcThreads::EndTime endTime(iMillis);
  while (!IsEndOfInput())
  {
    int64_t iAvail = GetAvailableRead();
    if (iAvail >= iMinAvail)
      return iAvail;

    if (!m_hDataAvailEvent->Wait(std::chrono::milliseconds(endTime.MillisLeft())))
      return CACHE_RC_TIMEOUT;
  }
  return GetAvailableRead();
}

int64_t CSimpleFileCache::Seek(int64_t iFilePosition)
{
  int64_t iTarget = iFilePosition - m_nStartPosition;

  if (iTarget < 0)
  {
    CLog::Log(LOGDEBUG, "CSimpleFileCache::{} - <{}> Request seek to {} before start of cache",
              __FUNCTION__, iFilePosition, m_filename);
    return CACHE_RC_ERROR;
  }

  int64_t nDiff = iTarget - m_nWritePosition;
  if (nDiff > 500000)
  {
    CLog::Log(LOGDEBUG,
              "CSimpleFileCache::{} - <{}> Requested position {} is beyond cached data ({})",
              __FUNCTION__, m_filename, iFilePosition, m_nWritePosition);
    return CACHE_RC_ERROR;
  }

  if (nDiff > 0 &&
      WaitForData(static_cast<unsigned int>(iTarget - m_nReadPosition), 5000) == CACHE_RC_TIMEOUT)
  {
    CLog::Log(LOGDEBUG, "CSimpleFileCache::{} - <{}> Wait for position {} failed. Ended up at {}",
              __FUNCTION__, m_filename, iFilePosition, m_nWritePosition);
    return CACHE_RC_ERROR;
  }

  m_nReadPosition = m_cacheFileRead->Seek(iTarget, SEEK_SET);
  if (m_nReadPosition != iTarget)
  {
    CLog::Log(LOGERROR, "CSimpleFileCache::{} - <{}> Can't seek cache file for position {}",
              __FUNCTION__, iFilePosition, m_filename);
    return CACHE_RC_ERROR;
  }

  m_space.Set();

  return iFilePosition;
}

bool CSimpleFileCache::Reset(int64_t iSourcePosition)
{
  if (IsCachedPosition(iSourcePosition))
  {
    m_nReadPosition = m_cacheFileRead->Seek(iSourcePosition - m_nStartPosition, SEEK_SET);
    return false;
  }

  m_nStartPosition = iSourcePosition;
  m_nWritePosition = m_cacheFileWrite->Seek(0, SEEK_SET);
  m_nReadPosition = m_cacheFileRead->Seek(0, SEEK_SET);
  return true;
}

void CSimpleFileCache::EndOfInput()
{
  CCacheStrategy::EndOfInput();
  m_hDataAvailEvent->Set();
}

int64_t CSimpleFileCache::CachedDataEndPosIfSeekTo(int64_t iFilePosition)
{
  if (iFilePosition >= m_nStartPosition && iFilePosition <= m_nStartPosition + m_nWritePosition)
    return m_nStartPosition + m_nWritePosition;
  return iFilePosition;
}

int64_t CSimpleFileCache::CachedDataStartPos()
{
  return m_nStartPosition;
}

int64_t CSimpleFileCache::CachedDataEndPos()
{
  return m_nStartPosition + m_nWritePosition;
}

bool CSimpleFileCache::IsCachedPosition(int64_t iFilePosition)
{
  return iFilePosition >= m_nStartPosition && iFilePosition <= m_nStartPosition + m_nWritePosition;
}

CCacheStrategy *CSimpleFileCache::CreateNew()
{
  return new CSimpleFileCache();
}


CDoubleCache::CDoubleCache(CCacheStrategy *impl)
{
  assert(NULL != impl);
  m_pCache = impl;
  m_pCacheOld = NULL;
}

CDoubleCache::~CDoubleCache()
{
  delete m_pCache;
  delete m_pCacheOld;
}

int CDoubleCache::Open()
{
  return m_pCache->Open();
}

void CDoubleCache::Close()
{
  m_pCache->Close();
  if (m_pCacheOld)
  {
    delete m_pCacheOld;
    m_pCacheOld = NULL;
  }
}

size_t CDoubleCache::GetMaxWriteSize(const size_t& iRequestSize)
{
  return m_pCache->GetMaxWriteSize(iRequestSize); // NOTE: Check the active cache only
}

int CDoubleCache::WriteToCache(const char *pBuffer, size_t iSize)
{
  return m_pCache->WriteToCache(pBuffer, iSize);
}

int CDoubleCache::ReadFromCache(char *pBuffer, size_t iMaxSize)
{
  return m_pCache->ReadFromCache(pBuffer, iMaxSize);
}

int64_t CDoubleCache::WaitForData(unsigned int iMinAvail, unsigned int iMillis)
{
  return m_pCache->WaitForData(iMinAvail, iMillis);
}

int64_t CDoubleCache::Seek(int64_t iFilePosition)
{
  /* Check whether position is NOT in our current cache but IS in our old cache.
   * This is faster/more efficient than having to possibly wait for data in the
   * Seek() call below
   */
  if (!m_pCache->IsCachedPosition(iFilePosition) &&
       m_pCacheOld && m_pCacheOld->IsCachedPosition(iFilePosition))
  {
    // Return error to trigger a seek event which will swap the caches:
    return CACHE_RC_ERROR;
  }

  return m_pCache->Seek(iFilePosition); // Normal seek
}

bool CDoubleCache::Reset(int64_t iSourcePosition)
{
  /* Check if we should (not) swap the caches. Note that when both caches have the
   * requested position, we prefer the cache that has the most forward data
   */
  if (m_pCache->IsCachedPosition(iSourcePosition) &&
      (!m_pCacheOld || !m_pCacheOld->IsCachedPosition(iSourcePosition) ||
       m_pCache->CachedDataEndPos() >= m_pCacheOld->CachedDataEndPos()))
  {
    // No swap: Just use current cache
    return m_pCache->Reset(iSourcePosition);
  }

  // Need to swap caches
  CCacheStrategy* pCacheTmp;
  if (!m_pCacheOld)
  {
    pCacheTmp = m_pCache->CreateNew();
    if (pCacheTmp->Open() != CACHE_RC_OK)
    {
      delete pCacheTmp;
      return m_pCache->Reset(iSourcePosition);
    }
  }
  else
  {
    pCacheTmp = m_pCacheOld;
  }

  // Perform actual swap:
  m_pCacheOld = m_pCache;
  m_pCache = pCacheTmp;

  // If new active cache still doesn't have this position, log it
  if (!m_pCache->IsCachedPosition(iSourcePosition))
  {
    CLog::Log(LOGDEBUG, "CDoubleCache::{} - ({}) Cache miss for {} with new={}-{} and old={}-{}",
              __FUNCTION__, fmt::ptr(this), iSourcePosition, m_pCache->CachedDataStartPos(),
              m_pCache->CachedDataEndPos(), m_pCacheOld->CachedDataStartPos(),
              m_pCacheOld->CachedDataEndPos());
  }

  return m_pCache->Reset(iSourcePosition);
}

void CDoubleCache::EndOfInput()
{
  m_pCache->EndOfInput();
}

bool CDoubleCache::IsEndOfInput()
{
  return m_pCache->IsEndOfInput();
}

void CDoubleCache::ClearEndOfInput()
{
  m_pCache->ClearEndOfInput();
}

int64_t CDoubleCache::CachedDataStartPos()
{
  return m_pCache->CachedDataStartPos();
}

int64_t CDoubleCache::CachedDataEndPos()
{
  return m_pCache->CachedDataEndPos();
}

int64_t CDoubleCache::CachedDataEndPosIfSeekTo(int64_t iFilePosition)
{
  /* Return the position on source we would end up after a cache-seek(/reset)
   * Note that we select the cache that has the most forward data already cached
   * for this position
   */
  int64_t ret = m_pCache->CachedDataEndPosIfSeekTo(iFilePosition);
  if (m_pCacheOld)
    return std::max(ret, m_pCacheOld->CachedDataEndPosIfSeekTo(iFilePosition));
  return ret;
}

bool CDoubleCache::IsCachedPosition(int64_t iFilePosition)
{
  return m_pCache->IsCachedPosition(iFilePosition) || (m_pCacheOld && m_pCacheOld->IsCachedPosition(iFilePosition));
}

CCacheStrategy *CDoubleCache::CreateNew()
{
  return new CDoubleCache(m_pCache->CreateNew());
}

