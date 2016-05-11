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

#include "DVDInputStreamFile.h"
#include "filesystem/File.h"
#include "filesystem/IFile.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CDVDInputStreamFile::CDVDInputStreamFile(const CFileItem& fileitem) : CDVDInputStream(DVDSTREAM_TYPE_FILE, fileitem)
{
  m_pFile = NULL;
  m_eof = true;
}

CDVDInputStreamFile::~CDVDInputStreamFile()
{
  Close();
}

bool CDVDInputStreamFile::IsEOF()
{
  return !m_pFile || m_eof;
}

bool CDVDInputStreamFile::Open()
{
  if (!CDVDInputStream::Open())
    return false;

  m_pFile = new CFile();
  if (!m_pFile)
    return false;

  unsigned int flags = READ_TRUNCATED | READ_BITRATE | READ_CHUNKED;
  
  // If this file is audio and/or video (= not a subtitle) flag to caller
  if (!m_item.IsSubtitle())
    flags |= READ_AUDIO_VIDEO;

  /*
   * There are 5 buffer modes available (configurable in as.xml)
   * 0) Buffer all internet filesystems (like 2 but additionally also ftp, webdav, etc.) (default)
   * 1) Buffer all filesystems (including local)
   * 2) Only buffer true internet filesystems (streams) (http, etc.)
   * 3) No buffer
   * 4) Buffer all non-local (remote) filesystems
   */
  if (!URIUtils::IsOnDVD(m_item.GetPath()) && !URIUtils::IsBluray(m_item.GetPath())) // Never cache these
  {
    if ((g_advancedSettings.m_cacheBufferMode == CACHE_BUFFER_MODE_INTERNET && URIUtils::IsInternetStream(m_item.GetPath(), true))
     || (g_advancedSettings.m_cacheBufferMode == CACHE_BUFFER_MODE_TRUE_INTERNET && URIUtils::IsInternetStream(m_item.GetPath(), false))
     || (g_advancedSettings.m_cacheBufferMode == CACHE_BUFFER_MODE_REMOTE && URIUtils::IsRemote(m_item.GetPath()))
     || (g_advancedSettings.m_cacheBufferMode == CACHE_BUFFER_MODE_ALL))
    {
      flags |= READ_CACHED;
    }
  }

  if (!(flags & READ_CACHED))
    flags |= READ_NO_CACHE; // Make sure CFile honors our no-cache hint

  std::string content = m_item.GetMimeType();

  if (content == "video/mp4" ||
      content == "video/x-msvideo" ||
      content == "video/avi" ||
      content == "video/x-matroska" ||
      content == "video/x-matroska-3d")
    flags |= READ_MULTI_STREAM;

  // open file in binary mode
  if (!m_pFile->Open(m_item.GetPath(), flags))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }

  if (m_pFile->GetImplemenation() && (content.empty() || content == "application/octet-stream"))
    m_content = m_pFile->GetImplemenation()->GetContent();

  m_eof = false;
  return true;
}

// close file and reset everyting
void CDVDInputStreamFile::Close()
{
  if (m_pFile)
  {
    m_pFile->Close();
    delete m_pFile;
  }

  CDVDInputStream::Close();
  m_pFile = NULL;
  m_eof = true;
}

int CDVDInputStreamFile::Read(uint8_t* buf, int buf_size)
{
  if(!m_pFile) return -1;

  ssize_t ret = m_pFile->Read(buf, buf_size);

  if (ret < 0)
    return -1; // player will retry read in case of error until playback is stopped

  /* we currently don't support non completing reads */
  if (ret == 0) 
    m_eof = true;

  return (int)ret;
}

int64_t CDVDInputStreamFile::Seek(int64_t offset, int whence)
{
  if(!m_pFile) return -1;

  if(whence == SEEK_POSSIBLE)
    return m_pFile->IoControl(IOCTRL_SEEK_POSSIBLE, NULL);

  int64_t ret = m_pFile->Seek(offset, whence);

  /* if we succeed, we are not eof anymore */
  if( ret >= 0 ) m_eof = false;

  return ret;
}

int64_t CDVDInputStreamFile::GetLength()
{
  if (m_pFile)
    return m_pFile->GetLength();
  return 0;
}

bool CDVDInputStreamFile::GetCacheStatus(XFILE::SCacheStatus *status)
{
  if(m_pFile && m_pFile->IoControl(IOCTRL_CACHE_STATUS, status) >= 0)
    return true;
  else
    return false;
}

BitstreamStats CDVDInputStreamFile::GetBitstreamStats() const
{
  if (!m_pFile)
    return m_stats; // dummy return. defined in CDVDInputStream

  if(m_pFile->GetBitstreamStats())
    return *m_pFile->GetBitstreamStats();
  else
    return m_stats;
}

int CDVDInputStreamFile::GetBlockSize()
{
  if(m_pFile)
    return m_pFile->GetChunkSize();
  else
    return 0;
}

void CDVDInputStreamFile::SetReadRate(unsigned rate)
{
  // Increase requested rate by 10%:
  unsigned maxrate = (unsigned) (1.1 * rate);

  if(m_pFile->IoControl(IOCTRL_CACHE_SETRATE, &maxrate) >= 0)
    CLog::Log(LOGDEBUG, "CDVDInputStreamFile::SetReadRate - set cache throttle rate to %u bytes per second", maxrate);
}
