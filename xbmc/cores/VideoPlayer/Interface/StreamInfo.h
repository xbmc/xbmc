/*
*      Copyright (C) 2016 Team Kodi
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
#include "utils/Geometry.h"

template <typename T> class CRectGen;
typedef CRectGen<float>  CRect;

enum StreamFlags
{
  FLAG_NONE = 0x0000
  , FLAG_DEFAULT = 0x0001
  , FLAG_DUB = 0x0002
  , FLAG_ORIGINAL = 0x0004
  , FLAG_COMMENT = 0x0008
  , FLAG_LYRICS = 0x0010
  , FLAG_KARAOKE = 0x0020
  , FLAG_FORCED = 0x0040
  , FLAG_HEARING_IMPAIRED = 0x0080
  , FLAG_VISUAL_IMPAIRED = 0x0100
};

struct StreamInfo
{
  bool valid = false;
  int bitrate = 0;
  std::string language;
  std::string name;
  std::string codecName;
  StreamFlags flags = StreamFlags::FLAG_NONE;

protected:
  StreamInfo() = default;
  virtual ~StreamInfo() = default;
};

struct AudioStreamInfo : StreamInfo
{
  int channels = 0;
  int samplerate = 0;
  int bitspersample = 0;
};

struct SubtitleStreamInfo : StreamInfo
{};

struct VideoStreamInfo : StreamInfo
{
  float videoAspectRatio = 0.0f;
  int height = 0;
  int width = 0;
  CRect SrcRect;
  CRect DestRect;
  std::string stereoMode;
  int angles = 0;
};

struct ProgramInfo
{
  int id = -1;
  bool playing = false;
  std::string name;
};
