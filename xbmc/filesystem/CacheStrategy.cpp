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

#include "threads/SystemClock.h"
#include "CacheStrategy.h"
#include "IFile.h"
#include "Util.h"
#include "utils/log.h"
#include "SpecialProtocol.h"
#include "PlatformDefs.h" //for PRIdS, PRId64
#include "URL.h"
#if defined(TARGET_POSIX)
#include "posix/PosixFile.h"
#define CacheLocalFile CPosixFile
#elif defined(TARGET_WINDOWS)
#include "win32/Win32File.h"
#define CacheLocalFile CWin32File
#endif // TARGET_WINDOWS

#include <cassert>
#include <algorithm>

using namespace XFILE;

CCacheStrategy::CCacheStrategy() : m_bEndOfInput(false)
{
}


CCacheStrategy::~CCacheStrategy()
{
}

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
  , m_nStartPosition(0)
  , m_nWritePosition(0)
  , m_nReadPosition(0) {
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

  m_filename = CSpecialProtocol::TranslatePath(CUtil::GetNextFilename("special://temp/filecache%03d.cache", 999));
  if (m_filename.empty())
  {
    CLog::Log(LOGERROR, "%s - Unable to generate a new filename", __FUNCTION__);
    Close();
    return CACHE_RC_ERROR;
  }

  CURL fileURL(m_filename);

  if (!m_cacheFileWrite->OpenForWrite(fileURL, false))
  {
    CLog::LogF(LOGERROR, "failed to create file \"%s\" for writing", m_filename.c_str());
    Close();
    return CACHE_RC_ERROR;
  }

  if (!m_cacheFileRead->Open(fileURL))
  {
    CLog::LogF(LOGERROR, "failed to open file \"%s\" for reading", m_filename.c_str());
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
    CLog::LogF(LOGWARNING, "failed to delete temporary file \"%s\"", m_filename.c_str());

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
    const ssize_t lastWritten = m_cacheFileWrite->Write(pBuffer, (iSize > SSIZE_MAX) ? SSIZE_MAX : iSize);
    if (lastWritten <= 0)
    {
      CLog::LogF(LOGERROR, "failed to write to file");
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
    return m_bEndOfInput? 0 : CACHE_RC_WOULD_BLOCK;

  size_t toRead = ((int64_t)iMaxSize > iAvailable) ? (size_t)iAvailable : iMaxSize;

  size_t readBytes = 0;
  while (toRead > 0)
  {
    const ssize_t lastRead = m_cacheFileRead->Read(pBuffer, (toRead > SSIZE_MAX) ? SSIZE_MAX : toRead);
    if (lastRead == 0)
      break;
    if (lastRead < 0)
    {
      CLog::LogF(LOGERROR, "failed to read from file");
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

    if (!m_hDataAvailEvent->WaitMSec(endTime.MillisLeft()))
      return CACHE_RC_TIMEOUT;
  }
  return GetAvailableRead();
}

int64_t CSimpleFileCache::Seek(int64_t iFilePosition)
{
  int64_t iTarget = iFilePosition - m_nStartPosition;

  if (iTarget < 0)
  {
    CLog::Log(LOGDEBUG,"CSimpleFileCache::Seek, request seek before start of cache.");
    return CACHE_RC_ERROR;
  }

  int64_t nDiff = iTarget - m_nWritePosition;
  if (nDiff > 500000 || (nDiff > 0 && WaitForData((unsigned int)(iTarget - m_nReadPosition), 5000) == CACHE_RC_TIMEOUT))
  {
    CLog::Log(LOGDEBUG,"CSimpleFileCache::Seek - Attempt to seek past read data");
    return CACHE_RC_ERROR;
  }

  m_nReadPosition = m_cacheFileRead->Seek(iTarget, SEEK_SET);
  if (m_nReadPosition != iTarget)
  {
    CLog::LogF(LOGERROR, "can't seek file");
    return CACHE_RC_ERROR;
  }

  m_space.Set();

  return iFilePosition;
}

bool CSimpleFileCache::Reset(int64_t iSourcePosition, bool clearAnyway)
{
  if (!clearAnyway && IsCachedPosition(iSourcePosition))
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
    return CACHE_RC_ERROR; // Request seek event, so caches are swapped
  }

  return m_pCache->Seek(iFilePosition); // Normal seek
}

bool CDoubleCache::Reset(int64_t iSourcePosition, bool clearAnyway)
{
  if (!clearAnyway && m_pCache->IsCachedPosition(iSourcePosition)
      && (!m_pCacheOld || !m_pCacheOld->IsCachedPosition(iSourcePosition)
          || m_pCache->CachedDataEndPos() >= m_pCacheOld->CachedDataEndPos()))
  {
    return m_pCache->Reset(iSourcePosition, clearAnyway);
  }
  if (!m_pCacheOld)
  {
    CCacheStrategy *pCacheNew = m_pCache->CreateNew();
    if (pCacheNew->Open() != CACHE_RC_OK)
    {
      delete pCacheNew;
      return m_pCache->Reset(iSourcePosition, clearAnyway);
    }
    bool bRes = pCacheNew->Reset(iSourcePosition, clearAnyway);
    m_pCacheOld = m_pCache;
    m_pCache = pCacheNew;
    return bRes;
  }
  bool bRes = m_pCacheOld->Reset(iSourcePosition, clearAnyway);
  CCacheStrategy *tmp = m_pCacheOld;
  m_pCacheOld = m_pCache;
  m_pCache = tmp;
  return bRes;
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

int64_t CDoubleCache::CachedDataEndPos()
{
  return m_pCache->CachedDataEndPos();
}

int64_t CDoubleCache::CachedDataEndPosIfSeekTo(int64_t iFilePosition)
{
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

