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

#include "InputStreamMultiSource.h"
#include "DVDFactoryInputStream.h"
#include "filesystem/File.h"
#include "filesystem/IFile.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include<map>

using namespace XFILE;

CInputStreamMultiSource::CInputStreamMultiSource(IVideoPlayer* pPlayer, const CFileItem& fileitem, const std::vector<std::string>& filenames) : InputStreamMultiStreams(DVDSTREAM_TYPE_MULTIFILES, fileitem),
  m_pPlayer(pPlayer),
  m_filenames(filenames)
{
}

CInputStreamMultiSource::~CInputStreamMultiSource()
{
  Close();
}

void CInputStreamMultiSource::Abort()
{
  for (auto iter : m_InputStreams)
    iter->Abort();
}

void CInputStreamMultiSource::Close()
{
  m_InputStreams.clear();
  CDVDInputStream::Close();
}

BitstreamStats CInputStreamMultiSource::GetBitstreamStats() const
{
  return m_stats;
}

int CInputStreamMultiSource::GetBlockSize()
{
  return 0;
}

bool CInputStreamMultiSource::GetCacheStatus(XFILE::SCacheStatus *status)
{
  return false;
}

int64_t CInputStreamMultiSource::GetLength()
{
  int64_t length = 0;
  for (auto iter : m_InputStreams)
  {
    length = std::max(length, iter->GetLength());
  }

  return length;
}

bool CInputStreamMultiSource::IsEOF()
{
  if (m_InputStreams.empty())
    return true;

  for (auto iter : m_InputStreams)
  {
    if (!(iter->IsEOF()))
      return false;
  }

  return true;
}

CDVDInputStream::ENextStream CInputStreamMultiSource::NextStream()
{
  bool eOF = IsEOF();
  if (m_InputStreams.empty() || eOF)
    return NEXTSTREAM_NONE;


  CDVDInputStream::ENextStream next;
  for (auto iter : m_InputStreams)
  {
    next = iter->NextStream();
    if (next != NEXTSTREAM_NONE)
      return next;
  }

  return NEXTSTREAM_RETRY;
}

bool CInputStreamMultiSource::Open()
{
  if (!m_pPlayer || m_filenames.empty())
    return false;

  for (unsigned int i = 0; i < m_filenames.size(); i++)
  {
    CFileItem fileitem = CFileItem(m_filenames[i], false);
    fileitem.SetMimeTypeForInternetFile();
    InputStreamPtr inputstream(CDVDFactoryInputStream::CreateInputStream(m_pPlayer, fileitem));
    if (!inputstream)
    {
      CLog::Log(LOGERROR, "CDVDPlayer::OpenInputStream - unable to create input stream for file [%s]", m_filenames[i].c_str());
      continue;
    }

    if (!inputstream->Open())
    {
      CLog::Log(LOGERROR, "CDVDPlayer::OpenInputStream - error opening file [%s]", m_filenames[i].c_str());
      continue;
    }
    m_InputStreams.push_back(inputstream);
  }
  return !m_InputStreams.empty();
}

int CInputStreamMultiSource::Read(uint8_t* buf, int buf_size)
{
  return -1;
}

int64_t CInputStreamMultiSource::Seek(int64_t offset, int whence)
{
  return -1;
}

void CInputStreamMultiSource::SetReadRate(unsigned rate)
{
  for (auto iter : m_InputStreams)
    iter->SetReadRate(rate);
}
