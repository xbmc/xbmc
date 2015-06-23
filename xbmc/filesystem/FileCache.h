#pragma once
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

#include "IFile.h"
#include "CacheStrategy.h"
#include "threads/CriticalSection.h"
#include "File.h"
#include "threads/Thread.h"
#include <atomic>

namespace XFILE
{

  class CFileCache : public IFile, public CThread
  {
  public:
    CFileCache(bool useDoubleCache=false);
    CFileCache(CCacheStrategy *pCache, bool bDeleteCache=true);
    virtual ~CFileCache();

    void SetCacheStrategy(CCacheStrategy *pCache, bool bDeleteCache=true);

    // CThread methods
    virtual void Process();
    virtual void OnExit();
    virtual void StopThread(bool bWait = true);

    // IFIle methods
    virtual bool          Open(const CURL& url);
    virtual void          Close();
    virtual bool          Exists(const CURL& url);
    virtual int           Stat(const CURL& url, struct __stat64* buffer);

    virtual ssize_t       Read(void* lpBuf, size_t uiBufSize);

    virtual int64_t       Seek(int64_t iFilePosition, int iWhence);
    virtual int64_t       GetPosition();
    virtual int64_t       GetLength();

    virtual int           IoControl(EIoControl request, void* param);

    IFile *GetFileImp();

    virtual std::string GetContent();
    virtual std::string GetContentCharset(void);

  private:
    CCacheStrategy *m_pCache;
    bool      m_bDeleteCache;
    int        m_seekPossible;
    CFile      m_source;
    std::string    m_sourcePath;
    CEvent      m_seekEvent;
    CEvent      m_seekEnded;
    int64_t      m_nSeekResult;
    int64_t      m_seekPos;
    int64_t      m_readPos;
    int64_t      m_writePos;
    unsigned     m_chunkSize;
    unsigned     m_writeRate;
    unsigned     m_writeRateActual;
    bool         m_cacheFull;
    std::atomic<int64_t> m_fileSize;
    CCriticalSection m_sync;
  };

}
