#pragma once
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

#include "IFile.h"
#include "CacheStrategy.h"
#include "utils/CriticalSection.h"
#include "FileSystem/File.h"
#include "utils/Thread.h"

namespace XFILE
{

  class CFileCache : public IFile, public CThread
  {
  public:
    CFileCache();
    CFileCache(CCacheStrategy *pCache, bool bDeleteCache=true);
    virtual ~CFileCache();

    void SetCacheStrategy(CCacheStrategy *pCache, bool bDeleteCache=true);

    // CThread methods
    virtual void Process();
    virtual void OnExit();
    virtual void StopThread();

    // IFIle methods
    virtual bool          Open(const CURL& url);
    virtual bool          Attach(IFile *pFile);
    virtual void          Close();
    virtual bool          Exists(const CURL& url);
    virtual int           Stat(const CURL& url, struct __stat64* buffer);

    virtual unsigned int  Read(void* lpBuf, __int64 uiBufSize);

    virtual __int64       Seek(__int64 iFilePosition, int iWhence);
    virtual __int64       GetPosition();
    virtual __int64       GetLength();

    virtual ICacheInterface* GetCache();
    IFile *GetFileImp();

    virtual int  GetChunkSize() {return m_source.GetChunkSize();}

    virtual CStdString GetContent();

  private:
    CCacheStrategy *m_pCache;
    bool      m_bDeleteCache;
    bool      m_bSeekPossible;
    CFile      m_source;
    CStdString    m_sourcePath;
    CEvent      m_seekEvent;
    CEvent      m_seekEnded;
    int        m_nBytesToBuffer;
    time_t      m_tmLastBuffering;
    __int64      m_nSeekResult;
    __int64      m_seekPos;
    __int64      m_readPos;
    CCriticalSection m_sync;
  };

}
