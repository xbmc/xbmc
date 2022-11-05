/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputStreamMultiSource.h"

#include "DVDFactoryInputStream.h"
#include "utils/log.h"

#include <map>

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
  for (const auto& iter : m_InputStreams)
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
  for (const auto& iter : m_InputStreams)
  {
    length = std::max(length, iter->GetLength());
  }

  return length;
}

bool CInputStreamMultiSource::IsEOF()
{
  if (m_InputStreams.empty())
    return true;

  for (const auto& iter : m_InputStreams)
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
  for (const auto& iter : m_InputStreams)
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
      CLog::Log(LOGERROR,
                "CDVDPlayer::OpenInputStream - unable to create input stream for file [{}]",
                m_filenames[i]);
      continue;
    }

    if (!inputstream->Open())
    {
      CLog::Log(LOGERROR, "CDVDPlayer::OpenInputStream - error opening file [{}]", m_filenames[i]);
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

void CInputStreamMultiSource::SetReadRate(uint32_t rate)
{
  for (const auto& iter : m_InputStreams)
    iter->SetReadRate(rate);
}
