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

#include "DVDInputStreamFile.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CDVDInputStreamFile::CDVDInputStreamFile() : CDVDInputStream(DVDSTREAM_TYPE_FILE)
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

bool CDVDInputStreamFile::Open(const char* strFile, const std::string& content)
{
  if (!CDVDInputStream::Open(strFile, content))
    return false;

  m_pFile = new CFile();
  if (!m_pFile)
    return false;

  // open file in binary mode
  if (!m_pFile->Open(strFile, READ_TRUNCATED | READ_BITRATE | READ_CHUNKED))
  {
    delete m_pFile;
    m_pFile = NULL;
    return false;
  }

  if (m_pFile->GetImplemenation() && (content.empty() || content == "application/octet-stream"))
    m_content = m_pFile->GetImplemenation()->GetContent();

  m_eof = true;
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

int CDVDInputStreamFile::Read(BYTE* buf, int buf_size)
{
  if(!m_pFile) return -1;

  unsigned int ret = m_pFile->Read(buf, buf_size);

  /* we currently don't support non completing reads */
  if( ret <= 0 ) m_eof = true;

  return (int)(ret & 0xFFFFFFFF);
}

__int64 CDVDInputStreamFile::Seek(__int64 offset, int whence)
{
  if(!m_pFile) return -1;

  if(whence == SEEK_POSSIBLE)
    return m_pFile->IoControl(IOCTRL_SEEK_POSSIBLE, NULL);

  __int64 ret = m_pFile->Seek(offset, whence);

  /* if we succeed, we are not eof anymore */
  if( ret >= 0 ) m_eof = false;

  return ret;
}

__int64 CDVDInputStreamFile::GetLength()
{
  if (m_pFile)
    return m_pFile->GetLength();
  return 0;
}

__int64 CDVDInputStreamFile::GetCachedBytes()
{
  SCacheStatus status;
  if(m_pFile && m_pFile->IoControl(IOCTRL_CACHE_STATUS, &status) >= 0)
    return status.forward;
  else
    return -1;
}

unsigned CDVDInputStreamFile::GetReadRate()
{
  SCacheStatus status;
  if(m_pFile && m_pFile->IoControl(IOCTRL_CACHE_STATUS, &status) >= 0)
  {
    if(status.full)
      return (unsigned)-1;
    else
      return status.currate;
  }
  else
    return (unsigned)-1;
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
  unsigned maxrate = rate + 1024 * 1024 / 8;
  if(m_pFile->IoControl(IOCTRL_CACHE_SETRATE, &maxrate) >= 0)
    CLog::Log(LOGDEBUG, "CDVDInputStreamFile::SetReadRate - set cache throttle rate to %u bytes per second", maxrate);
}
