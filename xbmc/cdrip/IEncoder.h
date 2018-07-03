/*
 *      Copyright (C) 2014 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <string>
#include <stdint.h>
#include "addons/kodi-addon-dev-kit/include/kodi/addon-instance/AudioEncoder.h"

class IEncoder
{
public:
  virtual ~IEncoder() = default;
  virtual bool Init(AddonToKodiFuncTable_AudioEncoder& callbacks) = 0;
  virtual int Encode(int nNumBytesRead, uint8_t* pbtStream) = 0;
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

