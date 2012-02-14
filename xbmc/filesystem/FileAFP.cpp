/*
 *      Copyright (C) 2011 Team XBMC
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

// FileAFP.cpp: implementation of the CFileAFP class.
//
//////////////////////////////////////////////////////////////////////
#ifdef _LINUX
#include "system.h"

#if defined(HAS_FILESYSTEM_AFP)
#include "FileAFP.h"
#include "PasswordManager.h"
#include "AFPDirectory.h"
#include "Util.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "DllLibAfp.h"

using namespace XFILE;

#define AFP_MAX_READ_SIZE 131072

CStdString URLEncode(const CStdString value)
{
  CStdString encoded(value);
  CURL::Encode(encoded);
  return encoded;
}

int AFPLog(const char* format, ...)
{
  va_list args;
  CStdString msg;
  va_start(args, format);
  msg.FormatV(format, args);
  va_end(args);
  CLog::Log(LOGDEBUG, "XAFP: %s", msg.c_str());
  
  return 0; 
}

CFileAFP::CFileAFP()
 : m_fileSize(0)
 , m_fileOffset(0)
 , m_fileHandle(NULL)
 , m_Context(NULL)
{
}

CFileAFP::~CFileAFP()
{
  Close();
}

xafp_context_pool_handle gAFPCtxPool = NULL;



xafp_client_handle CFileAFP::GetClientContext(const CURL& url)
{
  if (!gAFPCtxPool)
  {
    // Initialize Client Library and Context Pool
    xafp_set_log_level(XAFP_LOG_LEVEL_DEBUG);
    xafp_set_log_func(AFPLog);
    
    gAFPCtxPool = xafp_create_context_pool();
  }
  
  // Connect to server and mount desired volume/share
  xafp_client_handle ctx = xafp_get_context(gAFPCtxPool, url.GetHostName().c_str(), url.GetUserName().c_str(), url.GetPassWord().c_str());
  
  return ctx;
}

int64_t CFileAFP::GetPosition()
{
  if (!m_fileHandle) 
    return 0;
  return m_fileOffset;
}

int64_t CFileAFP::GetLength()
{
  if (!m_fileHandle) 
    return 0;
  return m_fileSize;
}

bool CFileAFP::Open(const CURL& url)
{
  Close();
  
  // we can't open files like afp://file.f or afp://server/file.f
  // URLS must contain at least a servername, share, and filr/dir name (e.g. afp://server/share/file.f or afp://server/share/dir/)
  if (!IsValidFile(url.GetFileName()))
  {
    CLog::Log(LOGNOTICE, "FileAfp: Bad URL : '%s'", url.GetFileName().c_str());
    return false;
  }

  m_Context = GetClientContext(url);
  if (!m_Context)
    return false;
  
  // TODO: Looks like we might need to handle URL encoding, etc. here...
  m_fileHandle = xafp_open_file(m_Context, url.GetFileName().c_str(), xafp_open_flag_read);
  if (!m_fileHandle)
  {
    // write error to logfile
    CLog::Log(LOGINFO, "CFileAFP::Open: Unable to open file : '%s'\nunix_err:'%x' error : '%s'", url.GetFileName().c_str(), errno, strerror(errno));
    return false;
  }
  
  CLog::Log(LOGDEBUG,"CFileAFP::Open - opened %s, fd=%d",url.GetFileName().c_str(), m_fileHandle);
  m_url = url;
  
#ifdef _LINUX
  struct __stat64 tmpBuffer;
#else
  struct stat tmpBuffer;
#endif
  
  if(Stat(&tmpBuffer))
  {
    Close();
    return false;
  }

  m_fileSize = tmpBuffer.st_size;
  m_fileOffset = 0;

  return true;
}

bool CFileAFP::Exists(const CURL& url)
{
  return (Stat(url, NULL) == 0);
}

int CFileAFP::Stat(struct __stat64* buffer)
{
  if (m_url.Get().length() > 0) 
    return Stat(m_url, buffer);
  return -1;
}

int CFileAFP::Stat(const CURL& url, struct __stat64* buffer)
{
  CAFPContext ctx(GetClientContext(url));
  
  if (!(xafp_client_handle)ctx)
    return -1;
  
  if (buffer) 
    memset(buffer, 0, sizeof(struct __stat64));
  
  return xafp_stat(ctx, url.GetFileName().c_str(), buffer);
}

unsigned int CFileAFP::Read(void *lpBuf, int64_t uiBufSize)
{
  if (!m_fileHandle)
    return 0;
  
  if (!m_Context)
    return 0;
  
  if (uiBufSize > AFP_MAX_READ_SIZE)
    uiBufSize = AFP_MAX_READ_SIZE;

  int bytesRead = xafp_read_file(m_Context, m_fileHandle, m_fileOffset, lpBuf, uiBufSize);
  if (bytesRead > 0)
    m_fileOffset += bytesRead;

  if (bytesRead < 0)
  {
    CLog::Log(LOGERROR, "%s - Error( %d, %d, %s )", __FUNCTION__, bytesRead, errno, strerror(errno));
    return 0;
  }

  return (unsigned int)bytesRead;
}

int64_t CFileAFP::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_fileHandle) 
    return -1;

  off_t newOffset = m_fileOffset;

  switch(iWhence)
  {
    case SEEK_SET:
      newOffset = iFilePosition;
      break;
    case SEEK_END:
      newOffset = m_fileSize+iFilePosition;
      break;
    case SEEK_CUR:
      newOffset += iFilePosition;
      break;
  }

  if ( newOffset < 0 || newOffset > m_fileSize)
  {
    CLog::Log(LOGERROR, "%s - Error( %"PRId64")", __FUNCTION__, newOffset);
    return -1;
  }

  m_fileOffset = newOffset;
  return (int64_t)m_fileOffset;
}

void CFileAFP::Close()
{
  if (m_fileHandle)
  {
    if (m_Context)
    {
      CLog::Log(LOGDEBUG, "CFileAFP::Close closing fd %d", m_fileHandle);      
      xafp_close_file(m_Context, m_fileHandle);
      xafp_free_context(gAFPCtxPool, m_Context);
      m_Context = NULL;
    }
    m_fileHandle = NULL;
  }
  m_url.Reset();
  m_fileSize = 0;
  m_fileOffset = 0;
}

int CFileAFP::Write(const void* lpBuf, int64_t uiBufSize)
{
  // Read-Only for now...
  return 0;
}

bool CFileAFP::Delete(const CURL& url)
{
  // Read-Only for now...
  return false;
}

bool CFileAFP::Rename(const CURL& url, const CURL& urlnew)
{
  CAFPContext ctx(GetClientContext(url));
  if ((xafp_client_handle)ctx)
  {
    CLog::Log(LOGDEBUG, "CFileAFP::Rename renaming %s -> %s", url.GetFileName().c_str(), urlnew.GetFileName().c_str());
    int err = xafp_rename_file(ctx, url.GetFileName(), urlnew.GetFileName());
    if (err)
    {
      CLog::Log(LOGERROR, "CFileAFP::Rename could not rename %s -> %s (err = %d)", url.GetFileName().c_str(), urlnew.GetFileName().c_str(), err);
      return false;
    }
    return true;
  }

  return false;
}

bool CFileAFP::OpenForWrite(const CURL& url, bool bOverWrite)
{
  // Read-Only for now...
  return false;
  
//  if (bOverWrite)
//  {
//    CLog::Log(LOGWARNING, "FileAFP::OpenForWrite() called with overwriting enabled! - %s", url.GetFileName().c_str());
//  }
//  return true;
}

bool CFileAFP::IsValidFile(const CStdString& strFileName)
{
  if (strFileName.Find('/') == -1   || // doesn't have sharename
      strFileName.Right(2)  == "/." || // not current folder
      strFileName.Right(3)  == "/..")  // not parent folder
  {
    return false;
  }
  return true;
}
#endif // HAS_FILESYSTEM_AFP
#endif // _LINUX
