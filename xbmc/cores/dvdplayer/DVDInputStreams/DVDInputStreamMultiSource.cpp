/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "DVDInputStreamMultiSource.h"
#include "DVDFactoryInputStream.h"
#include "filesystem/File.h"
#include "filesystem/IFile.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include<map>

using namespace XFILE;

CDVDInputStreamMultiSource::CDVDInputStreamMultiSource(IDVDPlayer* pPlayer, const std::vector<std::string>& filenames) : CDVDInputStream(DVDSTREAM_TYPE_MULTIFILES),
  m_pPlayer(pPlayer),
  m_filenames(filenames)
{
}

CDVDInputStreamMultiSource::~CDVDInputStreamMultiSource()
{
  Close();
}

void CDVDInputStreamMultiSource::Abort()
{
  for (auto iter : m_pInputStreams)
    iter->Abort();
}

void CDVDInputStreamMultiSource::Close()
{
  m_pInputStreams.clear();
  CDVDInputStream::Close();
}

BitstreamStats CDVDInputStreamMultiSource::GetBitstreamStats() const
{
  return m_stats;
}

int CDVDInputStreamMultiSource::GetBlockSize()
{
  return 0;
}

bool CDVDInputStreamMultiSource::GetCacheStatus(XFILE::SCacheStatus *status)
{
  return false;
}

int64_t CDVDInputStreamMultiSource::GetLength()
{
  int64_t length = 0;
  for (auto iter : m_pInputStreams)
  {
    length = std::max(length, iter->GetLength());
  }

  return length;
}

bool CDVDInputStreamMultiSource::IsEOF()
{
  if (m_pInputStreams.empty())
    return true;

  for (auto iter : m_pInputStreams)
  {
    if (!(iter->IsEOF()))
      return false;
  }

  return true;
}

bool CDVDInputStreamMultiSource::Open(const char* strFile, const std::string& content)
{
  if (!m_pPlayer || m_filenames.empty())
    return false;

  for (unsigned int i = 0, j = 0; i < m_filenames.size(); i++)
  {
    CFileItem fileitem = CFileItem(m_filenames[i]);
    std::string filemimetype = fileitem.GetMimeType();
    InputStreamPtr inputstream(CDVDFactoryInputStream::CreateInputStream(m_pPlayer, m_filenames[i], filemimetype));
    if (!inputstream)
    {
      CLog::Log(LOGERROR, "CDVDPlayer::OpenInputStream - unable to create input stream for file [%s]", m_filenames[i].c_str());
      continue;
    }
    else
      inputstream->SetFileItem(fileitem);

    if (!inputstream->Open(m_filenames[i].c_str(), filemimetype))
    {
      CLog::Log(LOGERROR, "CDVDPlayer::OpenInputStream - error opening file [%s]", m_filenames[i].c_str());
      continue;
    }
    m_pInputStreams.push_back(inputstream);
  }
  return !m_pInputStreams.empty();
}

int CDVDInputStreamMultiSource::Read(uint8_t* buf, int buf_size)
{
  return -1;
}

int64_t CDVDInputStreamMultiSource::Seek(int64_t offset, int whence)
{
  return -1;
}

void CDVDInputStreamMultiSource::SetReadRate(unsigned rate)
{
  for (auto iter : m_pInputStreams)
    iter->SetReadRate(rate);
}
