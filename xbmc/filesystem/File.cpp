/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "File.h"
#include "FileFactory.h"
#include "Application.h"
#include "DirectoryCache.h"
#include "Directory.h"
#include "FileCache.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/BitstreamStats.h"
#include "Util.h"

#ifndef _LINUX
#include "utils/Win32Exception.h"
#endif
#include "URL.h"

using namespace XFILE;
using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifndef __GNUC__
#pragma warning (disable:4244)
#endif

//*********************************************************************************************
CFile::CFile()
{
  m_pFile = NULL;
  m_pBuffer = NULL;
  m_flags = 0;
  m_bitStreamStats = NULL;
}

//*********************************************************************************************
CFile::~CFile()
{
  if (m_pFile)
    SAFE_DELETE(m_pFile);
  if (m_pBuffer)
    SAFE_DELETE(m_pBuffer);
  if (m_bitStreamStats)
    SAFE_DELETE(m_bitStreamStats);
}

//*********************************************************************************************

class CAutoBuffer
{
  char* p;
public:
  explicit CAutoBuffer(size_t s) { p = (char*)malloc(s); }
  ~CAutoBuffer() { free(p); }
  char* get() { return p; }
};

// This *looks* like a copy function, therefor the name "Cache" is misleading
bool CFile::Cache(const CStdString& strFileName, const CStdString& strDest, XFILE::IFileCallback* pCallback, void* pContext)
{
  CFile file;

  if (strFileName.empty() || strDest.empty())
    return false;

  // special case for zips - ignore caching
  CURL url(strFileName);
  if (URIUtils::IsInZIP(strFileName))
    url.SetOptions("?cache=no");
  if (file.Open(url.Get(), READ_TRUNCATED))
  {

    CFile newFile;
    if (URIUtils::IsHD(strDest)) // create possible missing dirs
    {
      vector<CStdString> tokens;
      CStdString strDirectory;
      URIUtils::GetDirectory(strDest,strDirectory);
      URIUtils::RemoveSlashAtEnd(strDirectory);  // for the test below
      if (!(strDirectory.size() == 2 && strDirectory[1] == ':'))
      {
        CURL url(strDirectory);
        CStdString pathsep;
#ifndef _LINUX
        pathsep = "\\";
#else
        pathsep = "/";
#endif
        CUtil::Tokenize(url.GetFileName(),tokens,pathsep.c_str());
        CStdString strCurrPath;
        // Handle special
        if (!url.GetProtocol().IsEmpty()) {
          pathsep = "/";
          strCurrPath += url.GetProtocol() + "://";
        } // If the directory has a / at the beginning, don't forget it
        else if (strDirectory[0] == pathsep[0])
          strCurrPath += pathsep;
        for (vector<CStdString>::iterator iter=tokens.begin();iter!=tokens.end();++iter)
        {
          strCurrPath += *iter+pathsep;
          CDirectory::Create(strCurrPath);
        }
      }
    }
    if (CFile::Exists(strDest))
      CFile::Delete(strDest);
    if (!newFile.OpenForWrite(strDest, true))  // overwrite always
    {
      file.Close();
      return false;
    }

    // 128k is optimal for xbox
    int iBufferSize = 128 * 1024;

    CAutoBuffer buffer(iBufferSize);
    int iRead, iWrite;

    UINT64 llFileSize = file.GetLength();
    UINT64 llPos = 0;

    CStopWatch timer;
    timer.StartZero();
    float start = 0.0f;
    while (true)
    {
      g_application.ResetScreenSaver();

      iRead = file.Read(buffer.get(), iBufferSize);
      if (iRead == 0) break;
      else if (iRead < 0)
      {
        CLog::Log(LOGERROR, "%s - Failed read from file %s", __FUNCTION__, strFileName.c_str());
        llFileSize = (uint64_t)-1;
        break;
      }

      /* write data and make sure we managed to write it all */
      iWrite = 0;
      while(iWrite < iRead)
      {
        int iWrite2 = newFile.Write(buffer.get()+iWrite, iRead-iWrite);
        if(iWrite2 <=0)
          break;
        iWrite+=iWrite2;
      }

      if (iWrite != iRead)
      {
        CLog::Log(LOGERROR, "%s - Failed write to file %s", __FUNCTION__, strDest.c_str());
        llFileSize = (uint64_t)-1;
        break;
      }

      llPos += iRead;

      // calculate the current and average speeds
      float end = timer.GetElapsedSeconds();

      if (pCallback && end - start > 0.5 && end)
      {
        start = end;

        float averageSpeed = llPos / end;
        int ipercent = 0;
        if(llFileSize)
          ipercent = 100 * llPos / llFileSize;

        if(!pCallback->OnFileCallback(pContext, ipercent, averageSpeed))
        {
          CLog::Log(LOGERROR, "%s - User aborted copy", __FUNCTION__);
          llFileSize = (uint64_t)-1;
          break;
        }
      }
    }

    /* close both files */
    newFile.Close();
    file.Close();

    /* verify that we managed to completed the file */
    if (llFileSize && llPos != llFileSize)
    {
      CFile::Delete(strDest);
      return false;
    }
    return true;
  }
  return false;
}

//*********************************************************************************************
bool CFile::Open(const CStdString& strFileName, unsigned int flags)
{
  m_flags = flags;
  try
  {
    bool bPathInCache;
    CURL url2(strFileName);
    if (url2.GetProtocol() == "zip")
      url2.SetOptions("");
    if (!g_directoryCache.FileExists(url2.Get(), bPathInCache) )
    {
      if (bPathInCache)
        return false;
    }

    CURL url(URIUtils::SubstitutePath(strFileName));
    if ( (flags & READ_NO_CACHE) == 0 && URIUtils::IsInternetStream(url) && !CUtil::IsPicture(strFileName) )
      m_flags |= READ_CACHED;

    if (m_flags & READ_CACHED)
    {
      m_pFile = new CFileCache();
      return m_pFile->Open(url);
    }

    m_pFile = CFileFactory::CreateLoader(url);
    if (!m_pFile)
      return false;

    try
    {
      if (!m_pFile->Open(url))
      {
        SAFE_DELETE(m_pFile);
        return false;
      }
    }
    catch (CRedirectException *pRedirectEx)
    {
      // the file implementation decided this item should use a different implementation.
      // the exception will contain the new implementation.
      CLog::Log(LOGDEBUG,"File::Open - redirecting implementation for %s", strFileName.c_str());
      SAFE_DELETE(m_pFile);
      if (pRedirectEx && pRedirectEx->m_pNewFileImp)
      {
        auto_ptr<CURL> pNewUrl(pRedirectEx->m_pNewUrl);
        m_pFile = pRedirectEx->m_pNewFileImp;
        delete pRedirectEx;
        
        if (pNewUrl.get())
        {
          if (!m_pFile->Open(*pNewUrl))
          {
            SAFE_DELETE(m_pFile);
            return false;
          }
        }
        else
        {        
          if (!m_pFile->Open(url))
          {
            SAFE_DELETE(m_pFile);
            return false;
          }
        }
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "File::Open - unknown exception when opening %s", strFileName.c_str());
      SAFE_DELETE(m_pFile);
      return false;
    }

    if (m_pFile->GetChunkSize() && !(m_flags & READ_CHUNKED))
    {
      m_pBuffer = new CFileStreamBuffer(0);
      m_pBuffer->Attach(m_pFile);
    }

    if (m_flags & READ_BITRATE)
    {
      m_bitStreamStats = new BitstreamStats();
      m_bitStreamStats->Start();
    }

    return true;
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error opening %s", __FUNCTION__, strFileName.c_str());
  return false;
}

bool CFile::OpenForWrite(const CStdString& strFileName, bool bOverWrite)
{
  try
  {
    CURL url(URIUtils::SubstitutePath(strFileName));

    m_pFile = CFileFactory::CreateLoader(url);
    if (m_pFile && m_pFile->OpenForWrite(url, bOverWrite))
    {
      // add this file to our directory cache (if it's stored)
      g_directoryCache.AddFile(strFileName);
      return true;
    }
    return false;
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception opening %s", __FUNCTION__, strFileName.c_str());
  }
  CLog::Log(LOGERROR, "%s - Error opening %s", __FUNCTION__, strFileName.c_str());
  return false;
}

bool CFile::Exists(const CStdString& strFileName, bool bUseCache /* = true */)
{
  CURL url;
  
  try
  {
    if (strFileName.IsEmpty())
      return false;

    if (bUseCache)
    {
      bool bPathInCache;
      if (g_directoryCache.FileExists(strFileName, bPathInCache) )
        return true;
      if (bPathInCache)
        return false;
    }

    url = URIUtils::SubstitutePath(strFileName);

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get())
      return false;

    return pFile->Exists(url);
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch (CRedirectException *pRedirectEx)
  {
    // the file implementation decided this item should use a different implementation.
    // the exception will contain the new implementation and optional a redirected URL.
    CLog::Log(LOGDEBUG,"File::Exists - redirecting implementation for %s", strFileName.c_str());
    if (pRedirectEx && pRedirectEx->m_pNewFileImp)
    {
      auto_ptr<IFile> pImp(pRedirectEx->m_pNewFileImp);
      auto_ptr<CURL> pNewUrl(pRedirectEx->m_pNewUrl);
      delete pRedirectEx;
        
      if (pNewUrl.get())
      {
        if (pImp.get() && !pImp->Exists(*pNewUrl))
        {
          return false;
        }
      }
      else     
      {
        if (pImp.get() && !pImp->Exists(url))
        {
          return false;
        }
      }
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error checking for %s", __FUNCTION__, strFileName.c_str());
  return false;
}

int CFile::Stat(struct __stat64 *buffer)
{
  return m_pFile->Stat(buffer);
}

int CFile::Stat(const CStdString& strFileName, struct __stat64* buffer)
{
  CURL url;
  
  try
  {
    url = URIUtils::SubstitutePath(strFileName);
    
    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get())
      return false;
    return pFile->Stat(url, buffer);
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch (CRedirectException *pRedirectEx)
  {
    // the file implementation decided this item should use a different implementation.
    // the exception will contain the new implementation and optional a redirected URL.
    CLog::Log(LOGDEBUG,"File::Stat - redirecting implementation for %s", strFileName.c_str());
    if (pRedirectEx && pRedirectEx->m_pNewFileImp)
    {
      auto_ptr<IFile> pImp(pRedirectEx->m_pNewFileImp);
      auto_ptr<CURL> pNewUrl(pRedirectEx->m_pNewUrl);
      delete pRedirectEx;
        
      if (pNewUrl.get())
      {
        if (pImp.get() && !pImp->Stat(*pNewUrl, buffer))
        {
          return false;
        }
      }
      else     
      {
        if (pImp.get() && !pImp->Stat(url, buffer))
        {
          return false;
        }
      }
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error statting %s", __FUNCTION__, strFileName.c_str());
  return -1;
}

unsigned int CFile::Read(void *lpBuf, int64_t uiBufSize)
{
  if (!m_pFile)
    return 0;

  if(m_pBuffer)
  {
    if(m_flags & READ_TRUNCATED)
    {
      unsigned int nBytes = m_pBuffer->sgetn(
        (char *)lpBuf, min<streamsize>((streamsize)uiBufSize,
                                                  m_pBuffer->in_avail()));
      if (m_bitStreamStats && nBytes>0)
        m_bitStreamStats->AddSampleBytes(nBytes);
      return nBytes;
    }
    else
    {
      unsigned int nBytes = m_pBuffer->sgetn((char*)lpBuf, uiBufSize);
      if (m_bitStreamStats && nBytes>0)
        m_bitStreamStats->AddSampleBytes(nBytes);
      return nBytes;
    }
  }

  try
  {
    if(m_flags & READ_TRUNCATED)
    {
      unsigned int nBytes = m_pFile->Read(lpBuf, uiBufSize);
      if (m_bitStreamStats && nBytes>0)
        m_bitStreamStats->AddSampleBytes(nBytes);
      return nBytes;
    }
    else
    {
      unsigned int done = 0;
      while((uiBufSize-done) > 0)
      {
        int curr = m_pFile->Read((char*)lpBuf+done, uiBufSize-done);
        if(curr<=0)
          break;

        done+=curr;
      }
      if (m_bitStreamStats && done > 0)
        m_bitStreamStats->AddSampleBytes(done);
      return done;
    }
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return 0;
}

//*********************************************************************************************
void CFile::Close()
{
  try
  {
    SAFE_DELETE(m_pBuffer);
    SAFE_DELETE(m_pFile);
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return;
}

void CFile::Flush()
{
  try
  {
    if (m_pFile)
      m_pFile->Flush();
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return;
}

//*********************************************************************************************
int64_t CFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_pFile)
    return -1;

  if (m_pBuffer)
  {
    if(iWhence == SEEK_CUR)
      return m_pBuffer->pubseekoff(iFilePosition,ios_base::cur);
    else if(iWhence == SEEK_END)
      return m_pBuffer->pubseekoff(iFilePosition,ios_base::end);
    else if(iWhence == SEEK_SET)
      return m_pBuffer->pubseekoff(iFilePosition,ios_base::beg);
  }

  try
  {
    return m_pFile->Seek(iFilePosition, iWhence);
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return -1;
}

//*********************************************************************************************
int64_t CFile::GetLength()
{
  try
  {
    if (m_pFile)
      return m_pFile->GetLength();
    return 0;
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return 0;
}

//*********************************************************************************************
int64_t CFile::GetPosition()
{
  if (!m_pFile)
    return -1;

  if (m_pBuffer)
    return m_pBuffer->pubseekoff(0, ios_base::cur);

  try
  {
    return m_pFile->GetPosition();
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return -1;
}


//*********************************************************************************************
bool CFile::ReadString(char *szLine, int iLineLength)
{
  if (!m_pFile)
    return false;

  if (m_pBuffer)
  {
    typedef CFileStreamBuffer::traits_type traits;
    CFileStreamBuffer::int_type aByte = m_pBuffer->sgetc();

    if(aByte == traits::eof())
      return false;

    while(iLineLength>0)
    {
      aByte = m_pBuffer->sbumpc();

      if(aByte == traits::eof())
        break;

      if(aByte == traits::to_int_type('\n'))
      {
        if(m_pBuffer->sgetc() == traits::to_int_type('\r'))
          m_pBuffer->sbumpc();
        break;
      }

      if(aByte == traits::to_int_type('\r'))
      {
        if(m_pBuffer->sgetc() == traits::to_int_type('\n'))
          m_pBuffer->sbumpc();
        break;
      }

      *szLine = traits::to_char_type(aByte);
      szLine++;
      iLineLength--;
    }

    // if we have no space for terminating character we failed
    if(iLineLength==0)
      return false;

    *szLine = 0;

    return true;
  }

  try
  {
    return m_pFile->ReadString(szLine, iLineLength);
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return false;
}

int CFile::Write(const void* lpBuf, int64_t uiBufSize)
{
  try
  {
    return m_pFile->Write(lpBuf, uiBufSize);
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return -1;
}

bool CFile::Delete(const CStdString& strFileName)
{
  try
  {
    CURL url(URIUtils::SubstitutePath(strFileName));

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get())
      return false;

    if(pFile->Delete(url))
    {
      g_directoryCache.ClearFile(strFileName);
      return true;
    }
  }
#ifndef _LINUX
  catch (const access_violation &e)
  {
    e.writelog(__FUNCTION__);
  }
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  if (Exists(strFileName))
    CLog::Log(LOGERROR, "%s - Error deleting file %s", __FUNCTION__, strFileName.c_str());
  return false;
}

bool CFile::Rename(const CStdString& strFileName, const CStdString& strNewFileName)
{
  try
  {
    CURL url(URIUtils::SubstitutePath(strFileName));
    CURL urlnew(URIUtils::SubstitutePath(strNewFileName));

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get())
      return false;

    if(pFile->Rename(url, urlnew))
    {
      g_directoryCache.ClearFile(strFileName);
      g_directoryCache.ClearFile(strNewFileName);
      return true;
    }
  }
#ifndef _LINUX
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception ", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error renaming file %s", __FUNCTION__, strFileName.c_str());
  return false;
}

bool CFile::SetHidden(const CStdString& fileName, bool hidden)
{
  try
  {
    CURL url(URIUtils::SubstitutePath(fileName));

    auto_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get())
      return false;

    return pFile->SetHidden(url, hidden);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s(%s) - Unhandled exception", __FUNCTION__, fileName.c_str());
  }
  return false;
}

int CFile::IoControl(EIoControl request, void* param)
{
  int result = -1;
  if (m_pFile)
    result = m_pFile->IoControl(request, param);

  if(result == -1 && request == IOCTRL_SEEK_POSSIBLE)
  {
    if(m_pFile->GetLength() >= 0 && m_pFile->Seek(0, SEEK_CUR) >= 0)
      return 1;
    else
      return 0;
  }

  return result;
}
//*********************************************************************************************
//*************** Stream IO for CFile objects *************************************************
//*********************************************************************************************
CFileStreamBuffer::~CFileStreamBuffer()
{
  sync();
  Detach();
}

CFileStreamBuffer::CFileStreamBuffer(int backsize)
  : streambuf()
  , m_file(NULL)
  , m_buffer(NULL)
  , m_backsize(backsize)
  , m_frontsize(0)
{
}

void CFileStreamBuffer::Attach(IFile *file)
{
  m_file = file;

  m_frontsize = CFile::GetChunkSize(m_file->GetChunkSize(), 64*1024);

  m_buffer = new char[m_frontsize+m_backsize];
  setg(0,0,0);
  setp(0,0);
}

void CFileStreamBuffer::Detach()
{
  setg(0,0,0);
  setp(0,0);
  delete[] m_buffer;
  m_buffer = NULL;
}

CFileStreamBuffer::int_type CFileStreamBuffer::underflow()
{
  if(gptr() < egptr())
    return traits_type::to_int_type(*gptr());

  if(!m_file)
    return traits_type::eof();

  size_t backsize = 0;
  if(m_backsize)
  {
    backsize = (size_t)min<ptrdiff_t>((ptrdiff_t)m_backsize, egptr()-eback());
    memmove(m_buffer, egptr()-backsize, backsize);
  }

  unsigned int size = 0;
#ifndef _LINUX
  try
  {
#endif
    size = m_file->Read(m_buffer+backsize, m_frontsize);
#ifndef _LINUX
  }
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
  }
#endif

  if(size == 0)
    return traits_type::eof();

  setg(m_buffer, m_buffer+backsize, m_buffer+backsize+size);
  return traits_type::to_int_type(*gptr());
}

CFileStreamBuffer::pos_type CFileStreamBuffer::seekoff(
  off_type offset,
  ios_base::seekdir way,
  ios_base::openmode mode)
{
  // calculate relative offset
  off_type pos  = m_file->GetPosition() - (egptr() - gptr());
  off_type offset2;
  if(way == ios_base::cur)
    offset2 = offset;
  else if(way == ios_base::beg)
    offset2 = offset - pos;
  else if(way == ios_base::end)
    offset2 = offset + m_file->GetLength() - pos;
  else
    return streampos(-1);

  // a non seek shouldn't modify our buffer
  if(offset2 == 0)
    return pos;

  // try to seek within buffer
  if(gptr()+offset2 >= eback() && gptr()+offset2 < egptr())
  {
    gbump(offset2);
    return pos + offset2;
  }

  // reset our buffer pointer, will
  // start buffering on next read
  setg(0,0,0);
  setp(0,0);

  int64_t position = -1;
#ifndef _LINUX
  try
  {
#endif
    if(way == ios_base::cur)
      position = m_file->Seek(offset, SEEK_CUR);
    else if(way == ios_base::end)
      position = m_file->Seek(offset, SEEK_END);
    else
      position = m_file->Seek(offset, SEEK_SET);
#ifndef _LINUX
  }
  catch (const win32_exception &e)
  {
    e.writelog(__FUNCTION__);
    return streampos(-1);
  }
#endif

  if(position<0)
    return streampos(-1);

  return position;
}

CFileStreamBuffer::pos_type CFileStreamBuffer::seekpos(
  pos_type pos,
  ios_base::openmode mode)
{
  return seekoff(pos, ios_base::beg, mode);
}

streamsize CFileStreamBuffer::showmanyc()
{
  underflow();
  return egptr() - gptr();
}

CFileStream::CFileStream(int backsize /*= 0*/) :
    istream(&m_buffer),
    m_buffer(backsize),
    m_file(NULL)
{
}

CFileStream::~CFileStream()
{
  Close();
}


bool CFileStream::Open(const CURL& filename)
{
  Close();

  // NOTE: This is currently not translated - reason is that all entry points into CFileStream::Open currently
  //       go from the CStdString version below.  We may have to change this in future, but I prefer not decoding
  //       the URL and re-encoding, or applying the translation twice.
  m_file = CFileFactory::CreateLoader(filename);
  if(m_file && m_file->Open(filename))
  {
    m_buffer.Attach(m_file);
    return true;
  }

  setstate(failbit);
  return false;
}

int64_t CFileStream::GetLength()
{
  return m_file->GetLength();
}

void CFileStream::Close()
{
  if(!m_file)
    return;

  m_buffer.Detach();
  SAFE_DELETE(m_file);
}

bool CFileStream::Open(const CStdString& filename)
{
  return Open(CURL(URIUtils::SubstitutePath(filename)));
}

#ifdef _ARMEL
char* CFileStream::ReadFile()
{
  if(!m_file)
    return NULL;

  int64_t length = m_file->GetLength();
  char *str = new char[length+1];
  m_file->Read(str, length);
  str[length] = '\0';
  return str;
}
#endif
