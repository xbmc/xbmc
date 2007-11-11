#include "stdafx.h" 

#include "FileCache.h"
#include "utils/Thread.h"
#include "Util.h"
#include "File.h"

#include "CacheMemBuffer.h"
#include "../utils/SingleLock.h"
#include "../Application.h"

using namespace XFILE;
 
#define READ_CACHE_CHUNK_SIZE (64*1024)

CFileCache::CFileCache()
{
   m_bDeleteCache = false;
   m_nSeekResult = 0;
   m_seekPos = 0;
   m_readPos = 0;
#ifdef _XBOX
   m_pCache = new CSimpleFileCache();
#else
   m_pCache = new CacheMemBuffer();
#endif
}

CFileCache::CFileCache(CCacheStrategy *pCache, bool bDeleteCache)
{
  m_pCache = pCache;
  m_bDeleteCache = bDeleteCache;
  m_seekPos = 0;
  m_readPos = 0; 
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

IFile *CFileCache::GetFileImp() {
  return m_source.GetImplemenation();
}

bool CFileCache::Open(const CURL& url, bool bBinary)
{
  Close();

  CSingleLock lock(m_sync);

  CLog::Log(LOGDEBUG,"CFileCache::Open - opening <%s> using cache", url.GetFileName().c_str());

  if (!m_pCache) {
    CLog::Log(LOGERROR,"CFileCache::Open - no cache strategy defined");
    return false;
  }

  url.GetURL(m_sourcePath);

  // open cache strategy
  if (m_pCache->Open() != CACHE_RC_OK) {
    CLog::Log(LOGERROR,"CFileCache::Open - failed to open cache");
    Close();
    return false;
  }

  // opening the source file.
  if(!m_source.Open(m_sourcePath, true, READ_NO_CACHE | READ_TRUNCATED)) {
    CLog::Log(LOGERROR,"%s - failed to open source <%s>", __FUNCTION__, m_sourcePath.c_str());
    Close();
    return false;
  }

  // check if source can seek
  m_bSeekPossible = m_source.Seek(0, SEEK_POSSIBLE) > 0 ? true : false;

  m_readPos = 0;
  m_seekEvent.Reset();
  m_seekEnded.Reset();

  CThread::Create(false);

  return true;
}

bool CFileCache::Attach(IFile *pFile) {
    CSingleLock lock(m_sync);

  if (!pFile || !m_pCache)
    return false;

  m_source.Attach(pFile);

  if (m_pCache->Open() != CACHE_RC_OK) {
    CLog::Log(LOGERROR,"CFileCache::Attach - failed to open cache");
    Close();
    return false;
  }

  CThread::Create(false);

  return true;
}

void CFileCache::Process() 
{
  if (!m_pCache) {
    CLog::Log(LOGERROR,"CFileCache::Process - sanity failed. no cache strategy");
    return;
  }

  // setup read chunks size
  int chunksize = m_source.GetChunkSize();
  if(chunksize == 0)
    chunksize = READ_CACHE_CHUNK_SIZE;

  // create our read buffer
  auto_aptr<char> buffer(new char[chunksize]);
  if (buffer.get() == NULL)
  {
    CLog::Log(LOGERROR, "%s - failed to allocate read buffer", __FUNCTION__);
    return;
  }
  
  while(!m_bStop)
  {
    // check for seek events
    if (m_seekEvent.WaitMSec(0)) 
    {
      m_seekEvent.Reset();
      CLog::Log(LOGDEBUG,"%s, request seek on source to %lld", __FUNCTION__, m_seekPos);  
      if ((m_nSeekResult = m_source.Seek(m_seekPos, SEEK_SET)) != m_seekPos)
        CLog::Log(LOGERROR,"%s, error %d seeking. seek returned %lld", __FUNCTION__, (int)GetLastError(), m_nSeekResult);
      else
        m_pCache->Reset(m_seekPos);

      m_seekEnded.Set();
    }

    int iRead = m_source.Read(buffer.get(), chunksize);
    if(iRead == 0)
    {
      CLog::Log(LOGINFO, "CFileCache::Process - Hit eof.");
      m_pCache->EndOfInput();
      
      // since there is no more to read - wait either for seek or close 
      // WaitForSingleObject is CThread::WaitForSingleObject that will also listen to the
      // end thread event.
      int nRet = CThread::WaitForSingleObject(m_seekEvent.GetHandle(), INFINITE);
      if (nRet == WAIT_OBJECT_0) 
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
        Sleep(5);

      iTotalWrite += iWrite;
    }

  }

}

void CFileCache::OnExit()
{
  m_bStop = true;

  // make sure cache is set to mark end of file (read may be waiting).
  if(m_pCache)
    m_pCache->EndOfInput();

  // just in case someone's waiting...
  m_seekEnded.Set();
}

bool CFileCache::Exists(const CURL& url)
{
  CStdString strPath;
  url.GetURL(strPath);
  return CFile::Exists(strPath);
}

int CFileCache::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strPath;
  url.GetURL(strPath);
  return CFile::Stat(strPath, buffer);
}

unsigned int CFileCache::Read(void* lpBuf, __int64 uiBufSize)
{
  CSingleLock lock(m_sync);
  if (!m_pCache) {
    CLog::Log(LOGERROR,"%s - sanity failed. no cache strategy!", __FUNCTION__);
    return 0;
  }
  __int64 iRc;

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

  if (iRc == CACHE_RC_EOF || iRc == 0)
    return 0;

  // unknown error code
  CLog::Log(LOGERROR, "%s - cache strategy returned unknown error code %d", __FUNCTION__, (int)iRc);  
  return 0;
}

__int64 CFileCache::Seek(__int64 iFilePosition, int iWhence)
{  
  CSingleLock lock(m_sync);

  if (!m_pCache) {
    CLog::Log(LOGERROR,"%s - sanity failed. no cache strategy!", __FUNCTION__);
    return -1;
  }

  __int64 iCurPos = m_readPos;
  __int64 iTarget = iFilePosition;
  if (iWhence == SEEK_END)
    iTarget = GetLength() + iTarget;
  else if (iWhence == SEEK_CUR)
    iTarget = iCurPos + iTarget;
  else if (iWhence == SEEK_POSSIBLE)
    return m_bSeekPossible;
  else if (iWhence != SEEK_SET)
    return -1;

  if (iTarget == m_readPos)
    return m_readPos;
  
  if ((m_nSeekResult = m_pCache->Seek(iTarget, SEEK_SET)) != iTarget)
  {
    if(!m_bSeekPossible)
      return m_nSeekResult;

    m_seekPos = iTarget;
    m_seekEvent.Set();
    if (WaitForSingleObject(m_seekEnded.GetHandle(),INFINITE) != WAIT_OBJECT_0) 
    {
      CLog::Log(LOGWARNING,"%s - seek to %lld failed.", __FUNCTION__, m_seekPos);
      return -1;
    }
  }

  if (m_nSeekResult >= 0)
    m_readPos = m_nSeekResult;

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

__int64 CFileCache::GetPosition()
{
  return m_readPos;
}

__int64 CFileCache::GetLength()
{
  return m_source.GetLength();
}

ICacheInterface* CFileCache::GetCache()
{
  if(m_pCache)
    return m_pCache->GetInterface();
  return NULL;
}
