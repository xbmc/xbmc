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

#include "FileXBMSP.h"
#include "utils/URIUtils.h"
#include "Directory.h"
#include "SectionLoader.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"

#include <sys/stat.h>

using namespace XFILE;

namespace XFILE
{

static UINT64 strtouint64(const char *s)
{
  UINT64 r = 0;

  while ((*s != 0) && (isspace(*s)))
    s++;
  if (*s == '+')
    s++;
  while ((*s != 0) && (isdigit(*s)))
  {
    r = r * ((UINT64)10);
    r += ((UINT64)(*s)) - ((UINT64)'0');
    s++;
  }
  return r;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileXBMSP::CFileXBMSP()
{
  CSectionLoader::Load("LIBXBMS");
  m_fileSize = 0;
  m_bOpened = false;
}

CFileXBMSP::~CFileXBMSP()
{
  Close();
  CSectionLoader::Unload("LIBXBMS");
}

//*********************************************************************************************
bool CFileXBMSP::Open(const CURL& urlUtf8)
{
  CStdString strURL = urlUtf8.Get();
  g_charsetConverter.utf8ToStringCharset(strURL);

  CURL url(strURL);
  const char* strUserName = url.GetUserName().c_str();
  const char* strPassword = url.GetPassWord().c_str();
  const char* strHostName = url.GetHostName().c_str();
  const char* strFileName = url.GetFileName().c_str();
  int iport = url.GetPort();

  char *tmp1, *tmp2, *info;

  if (m_bOpened) Close();

  m_bOpened = false;
  m_fileSize = 0;
  m_filePos = 0;

  if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG_SAMBA)
    CLog::Log(LOGDEBUG,"xbms:open: %s",strFileName);

  if (cc_xstream_client_connect(strHostName,
                                (iport > 0) ? iport : 1400,
                                &m_connection) != CC_XSTREAM_CLIENT_OK)
  {
    CLog::Log(LOGDEBUG, "xbms:unable to connect");
    return false;
  }
  if (cc_xstream_client_version_handshake(m_connection) != CC_XSTREAM_CLIENT_OK)
  {
    CLog::Log(LOGDEBUG, "xbms:unable handshake");
    cc_xstream_client_disconnect(m_connection);

    return false;
  }

  // Authenticate here!
  if ((strPassword != NULL) && (strlen(strPassword) > 0))
  {
    // We don't check the return value here.  If authentication
    // step fails, let's try if server lets us log in without
    // authentication.
    cc_xstream_client_password_authenticate(m_connection,
                                            (strUserName != NULL) ? strUserName : "",
                                            strPassword);
  }

  CStdString strFile = URIUtils::GetFileName(strFileName);

  char szPath[1024];
  strcpy(szPath, "");
  if (strFile.size() != strlen(strFileName) )
  {
    strncpy(szPath, strFileName, strlen(strFileName) - (strFile.size() + 1) );
    szPath[ strlen(strFileName) - (strFile.size() + 1)] = 0;
  }


  CStdString strDir;
  strDir = "";

  if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG_SAMBA)
    CLog::Log(LOGDEBUG,"xbms:setdir:/");

  if (cc_xstream_client_setcwd(m_connection, "/") == CC_XSTREAM_CLIENT_OK)
  {
    CStdString strPath = szPath;
    for (int i = 0; i < (int)strPath.size(); ++i)
    {
      if (strPath[i] == '/' || strPath[i] == '\\')
      {
        if (strDir != "")
        {
          if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG_SAMBA)
            CLog::Log(LOGDEBUG,"xbms:setdir: %s",strDir.c_str());

          if (cc_xstream_client_setcwd(m_connection, strDir.c_str()) != CC_XSTREAM_CLIENT_OK)
          {
            CLog::Log(LOGDEBUG, "xbms:unable set dir");
            if (m_connection != 0) cc_xstream_client_disconnect(m_connection);
            return false;
          }
        }
        strDir = "";
      }
      else
      {
        strDir += strPath[i];
      }
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "xbms:unable set dir");
    if (m_connection != 0) cc_xstream_client_disconnect(m_connection);
    return false;
  }
  if (strDir.size() > 0)
  {
    if (g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG_SAMBA)
      CLog::Log(LOGDEBUG,"xbms:setdir: %s",strDir.c_str());

    if (cc_xstream_client_setcwd(m_connection, strDir.c_str()) != CC_XSTREAM_CLIENT_OK)
    {
      CLog::Log(LOGDEBUG, "xbms:unable set dir");
      if (m_connection != 0) cc_xstream_client_disconnect(m_connection);
      return false;
    }
  }

  if (cc_xstream_client_file_info(m_connection, strFile.c_str(), &info) != CC_XSTREAM_CLIENT_OK)
  {
    CLog::Log(LOGDEBUG, "xbms:unable to get info for file: %s", strFile.c_str());
    cc_xstream_client_disconnect(m_connection);
    return false;
  }

  if (strstr(info, "<ATTRIB>file</ATTRIB>") != NULL)
  {
    tmp1 = strstr(info, "<SIZE>");
    tmp2 = strstr(info, "</SIZE>");
    if ((tmp1 != NULL) && (tmp2 != NULL) && (tmp2 > tmp1) && ((tmp2 - tmp1) < 22))
    {
      m_fileSize = strtouint64(tmp1 + 6);
    }
    else
    {
      m_fileSize = 4000000000U;
    }
  }
  else
  {
    m_fileSize = 4000000000U;
  }
  free(info);

  if (cc_xstream_client_file_open(m_connection, strFile.c_str(), &m_handle) != CC_XSTREAM_CLIENT_OK)
  {
    CLog::Log(LOGDEBUG, "xbms:unable to open file: %s", strFile.c_str());
    cc_xstream_client_disconnect(m_connection);

    return false;
  }
  m_bOpened = true;

  return true;
}

bool CFileXBMSP::Exists(const CURL& url)
{
  bool exist(true);
  exist = CFileXBMSP::Open(url);
  Close();
  return exist;
}

int CFileXBMSP::Stat(const CURL& url, struct __stat64* buffer)
{
  memset(buffer, 0, sizeof(struct __stat64));
  if (Open(url))
  {
    buffer->st_size = this->m_fileSize;
    buffer->st_mode = _S_IFREG;
    Close();
    errno = 0;
    return 0;
  }

  int dot = url.GetFileName().rfind('.');
  int slash = url.GetFileName().rfind('/');
  if (dot <= slash)
    if (CDirectory::Exists(url.Get()))
    {
      buffer->st_mode = _S_IFDIR;
      return 0;
    }

  errno = ENOENT;
  return -1;
}

//*********************************************************************************************
unsigned int CFileXBMSP::Read(void *lpBuf, int64_t uiBufSize)
{
  unsigned char *buf = NULL;
  size_t buflen = 0;
  size_t readsize;

  if (!m_bOpened) return 0;

  /* ccx has a max read size of 120k */
  if(uiBufSize > 120*1024)
    readsize = 120*1024;
  else
    readsize = (size_t)uiBufSize;

  if (cc_xstream_client_file_read(m_connection, m_handle, readsize, &buf, &buflen) !=
      CC_XSTREAM_CLIENT_OK)
  {
    CLog::Log(LOGERROR, "xbms:cc_xstream_client_file_read reported error on read");
    free(buf);
    return 0;
  }
  memcpy(lpBuf, buf, buflen);
  m_filePos += buflen;

  free(buf);

  return buflen;
}

//*********************************************************************************************
void CFileXBMSP::Close()
{

  if (m_bOpened)
  {
    cc_xstream_client_close(m_connection, m_handle);
    cc_xstream_client_disconnect(m_connection);
  }
  m_bOpened = false;
  m_fileSize = 0;
}

//*********************************************************************************************
int64_t CFileXBMSP::Seek(int64_t iFilePosition, int iWhence)
{
  UINT64 newpos;

  if (!m_bOpened) return -1;

  switch (iWhence)
  {
  case SEEK_SET:
    // cur = pos
    newpos = iFilePosition;
    break;
  case SEEK_CUR:
    // cur += pos
    newpos = m_filePos + iFilePosition;
    break;
  case SEEK_END:
    // end += pos
    newpos = m_fileSize + iFilePosition;
    break;
  default:
    return -1;
  }

  // We can't seek beyond EOF
  if (newpos > m_fileSize) return -1;

  if (newpos == m_filePos) return m_filePos;

  if ( newpos == 0 )
  {
    // goto beginning
    if (cc_xstream_client_file_rewind(m_connection, m_handle) == CC_XSTREAM_CLIENT_OK)
    {
      m_filePos = newpos;
    }
    else
    {
      return -1;
    }
  }
  else if ( newpos == m_fileSize )
  {
    // goto end
    if (cc_xstream_client_file_end(m_connection, m_handle) == CC_XSTREAM_CLIENT_OK)
    {
      m_filePos = newpos;
    }
    else
    {
      return -1;
    }
  }
  else if (newpos > m_filePos)
  {
    //Fix for broken seeking when we are at position 0 when using -dvd-device
    if (m_filePos == 0)
    {
      char cBuf[1];
      Read(cBuf, 1);
    }
    if (newpos == m_filePos) return m_filePos;
    if (cc_xstream_client_file_forward(m_connection, m_handle, (size_t)(newpos - m_filePos), 0) == CC_XSTREAM_CLIENT_OK)
    {
      m_filePos = newpos;
    }
    else
    {
      return -1;
    }
  }
  else if (newpos < m_filePos)
  {
    if (cc_xstream_client_file_backwards(m_connection, m_handle, (size_t)(m_filePos - newpos), 0) == CC_XSTREAM_CLIENT_OK)
    {
      m_filePos = newpos;
    }
    else
    {
      return -1;
    }
  }
  return m_filePos;
}

//*********************************************************************************************
int64_t CFileXBMSP::GetLength()
{
  if (!m_bOpened) return 0;
  return m_fileSize;
}

//*********************************************************************************************
int64_t CFileXBMSP::GetPosition()
{
  if (!m_bOpened) return 0;
  return m_filePos;
}

}


