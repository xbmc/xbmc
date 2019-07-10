/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDInputStreamFile.h"

#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "filesystem/IFile.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

using namespace XFILE;

CDVDInputStreamFile::CDVDInputStreamFile(const CFileItem& fileitem, unsigned int flags)
  : CDVDInputStream(DVDSTREAM_TYPE_FILE, fileitem), m_flags(flags)
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

  unsigned int flags = m_flags;

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
  if (!URIUtils::IsOnDVD(m_item.GetDynPath()) && !URIUtils::IsBluray(m_item.GetDynPath())) // Never cache these
  {
    unsigned int iCacheBufferMode = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_cacheBufferMode;
    if ((iCacheBufferMode == CACHE_BUFFER_MODE_INTERNET && URIUtils::IsInternetStream(m_item.GetDynPath(), true))
     || (iCacheBufferMode == CACHE_BUFFER_MODE_TRUE_INTERNET && URIUtils::IsInternetStream(m_item.GetDynPath(), false))
     || (iCacheBufferMode == CACHE_BUFFER_MODE_REMOTE && URIUtils::IsRemote(m_item.GetDynPath()))
     || (iCacheBufferMode == CACHE_BUFFER_MODE_ALL))
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
  if (!m_pFile->Open(m_item.GetDynPath(), flags))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }

  if (m_pFile->GetImplementation() && (content.empty() || content == "application/octet-stream"))
    m_content = m_pFile->GetImplementation()->GetProperty(XFILE::FILE_PROPERTY_CONTENT_TYPE);

  m_eof = false;
  return true;
}

// close file and reset everything
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
