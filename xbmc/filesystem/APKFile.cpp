/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// Android apk file i/o. Depends on libzip
// Basically the same format as zip.
// We might want to refactor CFileZip someday...
//////////////////////////////////////////////////////////////////////
#include "system.h"

#include "APKFile.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

#include <zip.h>

using namespace XFILE;

CAPKFile::CAPKFile()
{
  m_file_pos    = 0;
  m_file_size   = 0;
  m_zip_index   =-1;
  m_zip_file    = NULL;
  m_zip_archive = NULL;
}

CAPKFile::~CAPKFile()
{
}

bool CAPKFile::Open(const CURL& url)
{
  Close();

  m_url = url;
  CStdString path = url.GetFileName();
  CStdString host = url.GetHostName();
  // host name might be encoded rfc1738.txt, decode it.
  CURL::Decode(host);

  int zip_flags = 0, zip_error = 0;
  m_zip_archive = zip_open(host.c_str(), zip_flags, &zip_error);
  if (!m_zip_archive || zip_error)
  {
    CLog::Log(LOGERROR, "CAPKFile::Open: Unable to open archive : '%s'",
      host.c_str());
    return false;
  }

  m_zip_index = zip_name_locate(m_zip_archive, path.c_str(), zip_flags);
  if (m_zip_index == -1)
  {
    // might not be an error if caller is just testing for presence/absence 
    CLog::Log(LOGDEBUG, "CAPKFile::Open: Unable to locate file : '%s'",
      path.c_str());
    zip_close(m_zip_archive);
    m_zip_archive = NULL;
    return false;
  }

  // cache the file size
  struct zip_stat sb;
  zip_stat_init(&sb);
  int rtn = zip_stat_index(m_zip_archive, m_zip_index, zip_flags, &sb);
  if (rtn == -1)
  {
    CLog::Log(LOGERROR, "CAPKFile::Open: Unable to stat file : '%s'",
      path.c_str());
    zip_close(m_zip_archive);
    m_zip_archive = NULL;
    return false;
  }
  m_file_pos = 0;
  m_file_size = sb.size;

  // finally open the file
  m_zip_file = zip_fopen_index(m_zip_archive, m_zip_index, zip_flags);
  if (!m_zip_file)
  {
    CLog::Log(LOGERROR, "CAPKFile::Open: Unable to open file : '%s'",
      path.c_str());
    zip_close(m_zip_archive);
    m_zip_archive = NULL;
    return false;
  }

  // We've successfully opened the file!
  return true;
}

bool CAPKFile::Exists(const CURL& url)
{
  struct __stat64 buffer;
  return (Stat(url, &buffer) == 0);
}

void CAPKFile::Close()
{
  if (m_zip_archive)
  {
    if (m_zip_file)
      zip_fclose(m_zip_file);
    m_zip_file  = NULL;
  }
  zip_close(m_zip_archive);
  m_zip_archive = NULL;
  m_file_pos    = 0;
  m_file_size   = 0;
  m_zip_index   =-1;
}

int64_t CAPKFile::Seek(int64_t iFilePosition, int iWhence)
{
  // libzip has no seek so we have to fake it with reads
  off64_t file_pos = -1;
  if (m_zip_archive && m_zip_file)
  {
    switch(iWhence)
    {
      default:
      case SEEK_CUR:
        // set file offset to current plus offset
        if (m_file_pos + iFilePosition > m_file_size)
          return -1;
        file_pos = m_file_pos + iFilePosition;
        break;

      case SEEK_SET:
        // set file offset to offset
        if (iFilePosition > m_file_size)
          return -1;
        file_pos = iFilePosition;
        break;

      case SEEK_END:
        // set file offset to EOF minus offset
        if (iFilePosition > m_file_size)
          return -1;
        file_pos = m_file_size - iFilePosition;
        break;
    }
    // if offset is past current file position
    // then we must close, open then seek from zero.
    if (file_pos < m_file_pos)
    {
      zip_fclose(m_zip_file);
      int zip_flags = 0;
      m_zip_file = zip_fopen_index(m_zip_archive, m_zip_index, zip_flags);
    }
    char buffer[1024];
    int read_bytes = 1024 * (file_pos / 1024);
    for (int i = 0; i < read_bytes; i += 1024)
      zip_fread(m_zip_file, buffer, 1024);
    if (file_pos - read_bytes > 0)
      zip_fread(m_zip_file, buffer, file_pos - read_bytes);
    m_file_pos = file_pos;
  }

  return m_file_pos;
}

unsigned int CAPKFile::Read(void *lpBuf, int64_t uiBufSize)
{
  int bytes_read = uiBufSize;
  if (m_zip_archive && m_zip_file)
  {
    // check for a read pas EOF and clamp it to EOF
    if ((m_file_pos + bytes_read) > m_file_size)
      bytes_read = m_file_size - m_file_pos;

    bytes_read = zip_fread(m_zip_file, lpBuf, bytes_read);
    if (bytes_read != -1)
      m_file_pos += bytes_read;
    else
      bytes_read = 0;
  }

  return (unsigned int)bytes_read;
}

int CAPKFile::Stat(struct __stat64* buffer)
{
  return Stat(m_url, buffer);
}

int CAPKFile::Stat(const CURL& url, struct __stat64* buffer)
{
  memset(buffer, 0, sizeof(struct __stat64));

  // do not use interal member vars here,
  //  we might be called without opening
  CStdString path = url.GetFileName();
  CStdString host = url.GetHostName();
  // host name might be encoded rfc1738.txt, decode it.
  CURL::Decode(host);

  struct zip *zip_archive;
  int zip_flags = 0, zip_error = 0;
  zip_archive = zip_open(host.c_str(), zip_flags, &zip_error);
  if (!zip_archive || zip_error)
  {
    CLog::Log(LOGERROR, "CAPKFile::Stat: Unable to open archive : '%s'",
      host.c_str());
    errno = ENOENT;
    return -1;
  }

  // check if file exists
  int zip_index = zip_name_locate(zip_archive, url.GetFileName().c_str(), zip_flags);
  if (zip_index != -1)
  {
    struct zip_stat sb;
    zip_stat_init(&sb);
    int rtn = zip_stat_index(zip_archive, zip_index, zip_flags, &sb);
    if (rtn != -1)
    {
      buffer->st_gid  = 0;
      buffer->st_size = sb.size;
      buffer->st_mode = _S_IFREG;
      buffer->st_atime = sb.mtime;
      buffer->st_ctime = sb.mtime;
      buffer->st_mtime = sb.mtime;
    }
  }

  // check if directory exists
  if (buffer->st_mode != _S_IFREG)
  {
    // zip directories have a '/' at end.
    if (!URIUtils::HasSlashAtEnd(path))
      URIUtils::AddSlashAtEnd(path);

    int numFiles = zip_get_num_files(zip_archive);
    for (int i = 0; i < numFiles; i++)
    {
      CStdString name = zip_get_name(zip_archive, i, zip_flags);
      if (!name.IsEmpty() && name.Left(path.size()).Equals(path))
      {
        buffer->st_gid  = 0;
        buffer->st_mode = _S_IFDIR;
        break;
      }
    }
  }
  zip_close(zip_archive);

  if (buffer->st_mode != 0)
  {
    errno = 0;
    return 0;
  }
  else
  {
    errno = ENOENT;
    return -1;
  }
}

int64_t CAPKFile::GetPosition()
{
  return m_file_pos;
}

int64_t CAPKFile::GetLength()
{
  return m_file_size;
}

int  CAPKFile::GetChunkSize()
{
  return 1;
}
