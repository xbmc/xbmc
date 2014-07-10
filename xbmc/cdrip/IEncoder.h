#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
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

#include <string>
#include <stdint.h>
#include "addons/include/xbmc_audioenc_types.h"

class IEncoder
{
public:
  IEncoder() :
    m_iTrackLength(0),
    m_iInChannels(0),
    m_iInSampleRate(0),
    m_iInBitsPerSample(0)
  {
  }
  virtual ~IEncoder() {}
  virtual bool Init(audioenc_callbacks &callbacks) = 0;
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
  int m_iTrackLength;
  int m_iInChannels;
  int m_iInSampleRate;
  int m_iInBitsPerSample;
};

