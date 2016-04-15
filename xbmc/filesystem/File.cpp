/*
 *      Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *      Copyright (C) 2002-2013 Team XBMC
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

#include "File.h"
#include "IFile.h"
#include "FileFactory.h"
#include "Application.h"
#include "DirectoryCache.h"
#include "Directory.h"
#include "FileCache.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/BitstreamStats.h"
#include "Util.h"
#include "utils/StringUtils.h"

#include "commons/Exception.h"

using namespace XFILE;

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
  Close();
  if (m_pFile)
    SAFE_DELETE(m_pFile);
  if (m_pBuffer)
    SAFE_DELETE(m_pBuffer);
  if (m_bitStreamStats)
    SAFE_DELETE(m_bitStreamStats);
}

//*********************************************************************************************

bool CFile::Copy(const std::string& strFileName, const std::string& strDest, XFILE::IFileCallback* pCallback, void* pContext)
{
  const CURL pathToUrl(strFileName);
  const CURL pathToUrlDest(strDest);
  return Copy(pathToUrl, pathToUrlDest, pCallback, pContext);
}

bool CFile::Copy(const CURL& url2, const CURL& dest, XFILE::IFileCallback* pCallback, void* pContext)
{
  CFile file;

  const std::string pathToUrl(dest.Get());
  if (pathToUrl.empty())
    return false;

  // special case for zips - ignore caching
  CURL url(url2);
  if (URIUtils::IsInZIP(url.Get()) || URIUtils::IsInAPK(url.Get()))
    url.SetOptions("?cache=no");
  if (file.Open(url.Get(), READ_TRUNCATED | READ_CHUNKED))
  {

    CFile newFile;
    if (URIUtils::IsHD(pathToUrl)) // create possible missing dirs
    {
      std::vector<std::string> tokens;
      std::string strDirectory = URIUtils::GetDirectory(pathToUrl);
      URIUtils::RemoveSlashAtEnd(strDirectory);  // for the test below
      if (!(strDirectory.size() == 2 && strDirectory[1] == ':'))
      {
        CURL url(strDirectory);
        std::string pathsep;
#ifndef TARGET_POSIX
        pathsep = "\\";
#else
        pathsep = "/";
#endif
        StringUtils::Tokenize(url.GetFileName(),tokens,pathsep.c_str());
        std::string strCurrPath;
        // Handle special
        if (!url.GetProtocol().empty()) {
          pathsep = "/";
          strCurrPath += url.GetProtocol() + "://";
        } // If the directory has a / at the beginning, don't forget it
        else if (strDirectory[0] == pathsep[0])
          strCurrPath += pathsep;
        for (std::vector<std::string>::iterator iter=tokens.begin();iter!=tokens.end();++iter)
        {
          strCurrPath += *iter+pathsep;
          CDirectory::Create(strCurrPath);
        }
      }
    }
    if (CFile::Exists(dest))
      CFile::Delete(dest);
    if (!newFile.OpenForWrite(dest, true))  // overwrite always
    {
      file.Close();
      return false;
    }

    int iBufferSize = GetChunkSize(file.GetChunkSize(), 128 * 1024);

    auto_buffer buffer(iBufferSize);
    ssize_t iRead, iWrite;

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
        CLog::Log(LOGERROR, "%s - Failed read from file %s", __FUNCTION__, url.GetRedacted().c_str());
        llFileSize = (uint64_t)-1;
        break;
      }

      /* write data and make sure we managed to write it all */
      iWrite = 0;
      while(iWrite < iRead)
      {
        ssize_t iWrite2 = newFile.Write(buffer.get() + iWrite, iRead - iWrite);
        if(iWrite2 <=0)
          break;
        iWrite+=iWrite2;
      }

      if (iWrite != iRead)
      {
        CLog::Log(LOGERROR, "%s - Failed write to file %s", __FUNCTION__, dest.GetRedacted().c_str());
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
      CFile::Delete(dest);
      return false;
    }
    return true;
  }
  return false;
}

//*********************************************************************************************

bool CFile::CURLCreate(const std::string &url)
{
  m_curl.Parse(url);
  return true;
}

bool CFile::CURLAddOption(XFILE::CURLOPTIONTYPE type, const char* name, const char * value)
{
  switch (type){
  case XFILE::CURL_OPTION_CREDENTIALS:
  {
    m_curl.SetUserName(name);
    m_curl.SetPassword(value);
    break;
  }
  case XFILE::CURL_OPTION_PROTOCOL:
  case XFILE::CURL_OPTION_HEADER:
  {
    m_curl.SetProtocolOption(name, value);
    break;
  }
  case XFILE::CURL_OPTION_OPTION:
  {
    m_curl.SetOption(name, value);
    break;
  }
  default:
    return false;
  }
  return true;
}

bool CFile::CURLOpen(unsigned int flags)
{
  return Open(m_curl, flags);
}

bool CFile::Open(const std::string& strFileName, const unsigned int flags)
{
  const CURL pathToUrl(strFileName);
  return Open(pathToUrl, flags);
}

bool CFile::Open(const CURL& file, const unsigned int flags)
{
  m_flags = flags;
  try
  {
    bool bPathInCache;

    CURL url(URIUtils::SubstitutePath(file)), url2(url);

    if (url2.IsProtocol("apk") || url2.IsProtocol("zip") )
      url2.SetOptions("");

    if (!g_directoryCache.FileExists(url2.Get(), bPathInCache) )
    {
      if (bPathInCache)
        return false;
    }

    if (!(m_flags & READ_NO_CACHE))
    {
      const std::string pathToUrl(url.Get());
      if (URIUtils::IsInternetStream(url, true) && !CUtil::IsPicture(pathToUrl) )
        m_flags |= READ_CACHED;

      if (m_flags & READ_CACHED)
      {
        // for internet stream, if it contains multiple stream, file cache need handle it specially.
        m_pFile = new CFileCache(m_flags);
        return m_pFile->Open(url);
      }
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
      CLog::Log(LOGDEBUG,"File::Open - redirecting implementation for %s", file.GetRedacted().c_str());
      SAFE_DELETE(m_pFile);
      if (pRedirectEx && pRedirectEx->m_pNewFileImp)
      {
        std::unique_ptr<CURL> pNewUrl(pRedirectEx->m_pNewUrl);
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
      CLog::Log(LOGERROR, "File::Open - unknown exception when opening %s", file.GetRedacted().c_str());
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
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error opening %s", __FUNCTION__, file.GetRedacted().c_str());
  return false;
}

bool CFile::OpenForWrite(const std::string& strFileName, bool bOverWrite)
{
  const CURL pathToUrl(strFileName);
  return OpenForWrite(pathToUrl, bOverWrite);
}

bool CFile::OpenForWrite(const CURL& file, bool bOverWrite)
{
  try
  {
    CURL url = URIUtils::SubstitutePath(file);

    m_pFile = CFileFactory::CreateLoader(url);

    if (m_pFile && m_pFile->OpenForWrite(url, bOverWrite))
    {
      // add this file to our directory cache (if it's stored)
      g_directoryCache.AddFile(url.Get());
      return true;
    }
    return false;
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception opening %s", __FUNCTION__, file.GetRedacted().c_str());
  }
  CLog::Log(LOGERROR, "%s - Error opening %s", __FUNCTION__, file.GetRedacted().c_str());
  return false;
}

bool CFile::Exists(const std::string& strFileName, bool bUseCache /* = true */)
{
  const CURL pathToUrl(strFileName);
  return Exists(pathToUrl, bUseCache);
}

bool CFile::Exists(const CURL& file, bool bUseCache /* = true */)
{
  CURL url(URIUtils::SubstitutePath(file));

  try
  {
    if (bUseCache)
    {
      bool bPathInCache;
      if (g_directoryCache.FileExists(url.Get(), bPathInCache))
        return true;
      if (bPathInCache)
        return false;
    }

    std::unique_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get())
      return false;

    return pFile->Exists(url);
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch (CRedirectException *pRedirectEx)
  {
    // the file implementation decided this item should use a different implementation.
    // the exception will contain the new implementation and optional a redirected URL.
    CLog::Log(LOGDEBUG,"File::Exists - redirecting implementation for %s", file.GetRedacted().c_str());
    if (pRedirectEx && pRedirectEx->m_pNewFileImp)
    {
      std::unique_ptr<IFile> pImp(pRedirectEx->m_pNewFileImp);
      std::unique_ptr<CURL> pNewUrl(pRedirectEx->m_pNewUrl);
      delete pRedirectEx;

      if (pImp.get())
      {
        if (pNewUrl.get())
        {
          if (bUseCache)
          {
            bool bPathInCache;
            if (g_directoryCache.FileExists(pNewUrl->Get(), bPathInCache))
              return true;
            if (bPathInCache)
              return false;
          }
          return pImp->Exists(*pNewUrl);
        }
        else
          return pImp->Exists(url);
      }
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error checking for %s", __FUNCTION__, file.GetRedacted().c_str());
  return false;
}

int CFile::Stat(struct __stat64 *buffer)
{
  if (!buffer)
    return -1;

  if (!m_pFile)
  {
    memset(buffer, 0, sizeof(struct __stat64));
    errno = ENOENT;
    return -1;
  }

  return m_pFile->Stat(buffer);
}

bool CFile::SkipNext()
{
  if (m_pFile)
    return m_pFile->SkipNext();
  return false;
}

int CFile::Stat(const std::string& strFileName, struct __stat64* buffer)
{
  const CURL pathToUrl(strFileName);
  return Stat(pathToUrl, buffer);
}

int CFile::Stat(const CURL& file, struct __stat64* buffer)
{
  if (!buffer)
    return -1;

  CURL url(URIUtils::SubstitutePath(file));

  try
  {
    std::unique_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get())
      return -1;
    return pFile->Stat(url, buffer);
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch (CRedirectException *pRedirectEx)
  {
    // the file implementation decided this item should use a different implementation.
    // the exception will contain the new implementation and optional a redirected URL.
    CLog::Log(LOGDEBUG,"File::Stat - redirecting implementation for %s", file.GetRedacted().c_str());
    if (pRedirectEx && pRedirectEx->m_pNewFileImp)
    {
      std::unique_ptr<IFile> pImp(pRedirectEx->m_pNewFileImp);
      std::unique_ptr<CURL> pNewUrl(pRedirectEx->m_pNewUrl);
      delete pRedirectEx;
        
      if (pNewUrl.get())
      {
        if (pImp.get() && !pImp->Stat(*pNewUrl, buffer))
        {
          return 0;
        }
      }
      else     
      {
        if (pImp.get() && !pImp->Stat(url, buffer))
        {
          return 0;
        }
      }
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error statting %s", __FUNCTION__, file.GetRedacted().c_str());
  return -1;
}

ssize_t CFile::Read(void *lpBuf, size_t uiBufSize)
{
  if (!m_pFile)
    return -1;
  if (lpBuf == NULL && uiBufSize != 0)
    return -1;

  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  if (uiBufSize == 0)
  {
    // "test" read with zero size
    // some VFSs don't handle correctly null buffer pointer
    // provide valid buffer pointer for them
    char dummy;
    return m_pFile->Read(&dummy, 0);
  }

  if(m_pBuffer)
  {
    if(m_flags & READ_TRUNCATED)
    {
      const ssize_t nBytes = m_pBuffer->sgetn(
        (char *)lpBuf, std::min<std::streamsize>((std::streamsize)uiBufSize,
                                                  m_pBuffer->in_avail()));
      if (m_bitStreamStats && nBytes>0)
        m_bitStreamStats->AddSampleBytes(nBytes);
      return nBytes;
    }
    else
    {
      const ssize_t nBytes = m_pBuffer->sgetn((char*)lpBuf, uiBufSize);
      if (m_bitStreamStats && nBytes>0)
        m_bitStreamStats->AddSampleBytes(nBytes);
      return nBytes;
    }
  }

  try
  {
    if(m_flags & READ_TRUNCATED)
    {
      const ssize_t nBytes = m_pFile->Read(lpBuf, uiBufSize);
      if (m_bitStreamStats && nBytes>0)
        m_bitStreamStats->AddSampleBytes(nBytes);
      return nBytes;
    }
    else
    {
      ssize_t done = 0;
      while((uiBufSize-done) > 0)
      {
        const ssize_t curr = m_pFile->Read((char*)lpBuf+done, uiBufSize-done);
        if (curr <= 0)
        {
          if (curr < 0 && done == 0)
            return -1;

          break;
        }
        done+=curr;
      }
      if (m_bitStreamStats && done > 0)
        m_bitStreamStats->AddSampleBytes(done);
      return done;
    }
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
    return -1;
  }
  return 0;
}

//*********************************************************************************************
void CFile::Close()
{
  try
  {
    if (m_pFile)
      m_pFile->Close();

    SAFE_DELETE(m_pBuffer);
    SAFE_DELETE(m_pFile);
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
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
  XBMCCOMMONS_HANDLE_UNCHECKED
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
      return m_pBuffer->pubseekoff(iFilePosition, std::ios_base::cur);
    else if(iWhence == SEEK_END)
      return m_pBuffer->pubseekoff(iFilePosition, std::ios_base::end);
    else if(iWhence == SEEK_SET)
      return m_pBuffer->pubseekoff(iFilePosition, std::ios_base::beg);
  }

  try
  {
    return m_pFile->Seek(iFilePosition, iWhence);
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return -1;
}

//*********************************************************************************************
int CFile::Truncate(int64_t iSize)
{
  if (!m_pFile)
    return -1;
  
  try
  {
    return m_pFile->Truncate(iSize);
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
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
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return 0;
}

//*********************************************************************************************
int64_t CFile::GetPosition() const
{
  if (!m_pFile)
    return -1;

  if (m_pBuffer)
    return m_pBuffer->pubseekoff(0, std::ios_base::cur);

  try
  {
    return m_pFile->GetPosition();
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return -1;
}


//*********************************************************************************************
bool CFile::ReadString(char *szLine, int iLineLength)
{
  if (!m_pFile || !szLine)
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
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return false;
}

ssize_t CFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (!m_pFile)
    return -1;
  if (lpBuf == NULL && uiBufSize != 0)
    return -1;

  try
  {
    if (uiBufSize == 0 && lpBuf == NULL)
    { // "test" write with zero size
      // some VFSs don't handle correctly null buffer pointer
      // provide valid buffer pointer for them
      auto_buffer dummyBuf(255);
      dummyBuf.get()[0] = 0;
      return m_pFile->Write(dummyBuf.get(), 0);
    }

    return m_pFile->Write(lpBuf, uiBufSize);
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  return -1;
}

bool CFile::Delete(const std::string& strFileName)
{
  const CURL pathToUrl(strFileName);
  return Delete(pathToUrl);
}

bool CFile::Delete(const CURL& file)
{
  try
  {
    CURL url(URIUtils::SubstitutePath(file));

    std::unique_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get())
      return false;

    if(pFile->Delete(url))
    {
      g_directoryCache.ClearFile(url.Get());
      return true;
    }
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  if (Exists(file))
    CLog::Log(LOGERROR, "%s - Error deleting file %s", __FUNCTION__, file.GetRedacted().c_str());
  return false;
}

bool CFile::Rename(const std::string& strFileName, const std::string& strNewFileName)
{
  const CURL pathToUrl(strFileName);
  const CURL pathToUrlNew(strNewFileName);
  return Rename(pathToUrl, pathToUrlNew);
}

bool CFile::Rename(const CURL& file, const CURL& newFile)
{
  try
  {
    CURL url(URIUtils::SubstitutePath(file));
    CURL urlnew(URIUtils::SubstitutePath(newFile));

    std::unique_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get())
      return false;

    if(pFile->Rename(url, urlnew))
    {
      g_directoryCache.ClearFile(url.Get());
      g_directoryCache.AddFile(urlnew.Get());
      return true;
    }
  }
  XBMCCOMMONS_HANDLE_UNCHECKED
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception ", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error renaming file %s", __FUNCTION__, file.GetRedacted().c_str());
  return false;
}

bool CFile::SetHidden(const std::string& fileName, bool hidden)
{
  const CURL pathToUrl(fileName);
  return SetHidden(pathToUrl, hidden);
}

bool CFile::SetHidden(const CURL& file, bool hidden)
{
  try
  {
    CURL url(URIUtils::SubstitutePath(file));

    std::unique_ptr<IFile> pFile(CFileFactory::CreateLoader(url));
    if (!pFile.get())
      return false;

    return pFile->SetHidden(url, hidden);
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s(%s) - Unhandled exception", __FUNCTION__, file.GetRedacted().c_str());
  }
  return false;
}

int CFile::IoControl(EIoControl request, void* param)
{
  int result = -1;
  if (m_pFile == NULL)
    return -1;
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

int CFile::GetChunkSize()
{
  if (m_pFile)
    return m_pFile->GetChunkSize();
  return 0;
}

std::string CFile::GetContentMimeType(void)
{
  if (!m_pFile)
    return "";
  return m_pFile->GetContent();
}

std::string CFile::GetContentCharset(void)
{
  if (!m_pFile)
    return "";
  return m_pFile->GetContentCharset();
}

ssize_t CFile::LoadFile(const std::string &filename, auto_buffer& outputBuffer)
{
  const CURL pathToUrl(filename);
  return LoadFile(pathToUrl, outputBuffer);
}

ssize_t CFile::LoadFile(const CURL& file, auto_buffer& outputBuffer)
{
  static const size_t max_file_size = 0x7FFFFFFF;
  static const size_t min_chunk_size = 64 * 1024U;
  static const size_t max_chunk_size = 2048 * 1024U;

  outputBuffer.clear();

  if (!Open(file, READ_TRUNCATED))
    return 0;

  /*
  GetLength() will typically return values that fall into three cases:
  1. The real filesize. This is the typical case.
  2. Zero. This is the case for some http:// streams for example.
  3. Some value smaller than the real filesize. This is the case for an expanding file.

  In order to handle all three cases, we read the file in chunks, relying on Read()
  returning 0 at EOF.  To minimize (re)allocation of the buffer, the chunksize in
  cases 1 and 3 is set to one byte larger than the value returned by GetLength().
  The chunksize in case 2 is set to the lowest value larger than min_chunk_size aligned
  to GetChunkSize().

  We fill the buffer entirely before reallocation.  Thus, reallocation never occurs in case 1
  as the buffer is larger than the file, so we hit EOF before we hit the end of buffer.

  To minimize reallocation, we double the chunksize each read while chunksize is lower
  than max_chunk_size.
  */
  int64_t filesize = GetLength();
  if (filesize > (int64_t)max_file_size)
    return 0; /* file is too large for this function */

  size_t chunksize = (filesize > 0) ? (size_t)(filesize + 1) : (size_t) GetChunkSize(GetChunkSize(), min_chunk_size);
  size_t total_read = 0;
  while (true)
  {
    if (total_read == outputBuffer.size())
    { // (re)alloc
      if (outputBuffer.size() + chunksize > max_file_size)
      {
        outputBuffer.clear();
        return -1;
      }
      outputBuffer.resize(outputBuffer.size() + chunksize);
      if (chunksize < max_chunk_size)
        chunksize *= 2;
    }
    ssize_t read = Read(outputBuffer.get() + total_read, outputBuffer.size() - total_read);
    if (read < 0)
    {
      outputBuffer.clear();
      return -1;
    }
    total_read += read;
    if (!read)
      break;
  }

  outputBuffer.resize(total_read);

  return total_read;
}

double CFile::GetDownloadSpeed()
{
  if (m_pFile)
    return m_pFile->GetDownloadSpeed();
  return 0.0f;
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
  : std::streambuf()
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
    backsize = (size_t)std::min<ptrdiff_t>((ptrdiff_t)m_backsize, egptr()-eback());
    memmove(m_buffer, egptr()-backsize, backsize);
  }

  ssize_t size = m_file->Read(m_buffer+backsize, m_frontsize);

  if (size == 0)
    return traits_type::eof();
  else if (size < 0)
  {
    CLog::LogF(LOGWARNING, "Error reading file - assuming eof");
    return traits_type::eof();
  }

  setg(m_buffer, m_buffer+backsize, m_buffer+backsize+size);
  return traits_type::to_int_type(*gptr());
}

CFileStreamBuffer::pos_type CFileStreamBuffer::seekoff(
  off_type offset,
  std::ios_base::seekdir way,
  std::ios_base::openmode mode)
{
  // calculate relative offset
  off_type aheadbytes  = (egptr() - gptr());
  off_type pos  = m_file->GetPosition() - aheadbytes;
  off_type offset2;
  if(way == std::ios_base::cur)
    offset2 = offset;
  else if(way == std::ios_base::beg)
    offset2 = offset - pos;
  else if(way == std::ios_base::end)
    offset2 = offset + m_file->GetLength() - pos;
  else
    return std::streampos(-1);

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
  if(way == std::ios_base::cur)
    position = m_file->Seek(offset - aheadbytes, SEEK_CUR);
  else if(way == std::ios_base::end)
    position = m_file->Seek(offset, SEEK_END);
  else
    position = m_file->Seek(offset, SEEK_SET);

  if(position<0)
    return std::streampos(-1);

  return position;
}

CFileStreamBuffer::pos_type CFileStreamBuffer::seekpos(
  pos_type pos,
  std::ios_base::openmode mode)
{
  return seekoff(pos, std::ios_base::beg, mode);
}

std::streamsize CFileStreamBuffer::showmanyc()
{
  underflow();
  return egptr() - gptr();
}

CFileStream::CFileStream(int backsize /*= 0*/) :
    std::istream(&m_buffer),
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

  CURL url(URIUtils::SubstitutePath(filename));
  m_file = CFileFactory::CreateLoader(url);
  if(m_file && m_file->Open(url))
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

bool CFileStream::Open(const std::string& filename)
{
  const CURL pathToUrl(filename);
  return Open(pathToUrl);
}
