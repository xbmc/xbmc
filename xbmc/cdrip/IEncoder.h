/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>

#include "PlatformDefs.h" // for ssize_t

namespace KODI
{
namespace CDRIP
{

class IEncoder
{
public:
  virtual ~IEncoder() = default;
  virtual bool Init() = 0;
  virtual ssize_t Encode(uint8_t* pbtStream, size_t nNumBytesRead) = 0;
  virtual bool Close() = 0;

  // tag info
  std::string m_strComment;
  std::string m_strArtist;
  std::string m_strAlbumArtist;
  std::string m_strTitle;
  std::string m_strAlbum;
  std::string m_strGenre;
  std::string m_strTrack;
  std::string m_strYear;
  std::string m_strFile;
  int m_iTrackLength = 0;
  int m_iInChannels = 0;
  int m_iInSampleRate = 0;
  int m_iInBitsPerSample = 0;
};

} /* namespace CDRIP */
} /* namespace KODI */
