/*
 *      Copyright (C) 2018 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <array>
#include <memory>

namespace KODI
{
namespace RETRO
{
class IRetroPlayerStream;

struct DeleteStream
{
  void operator()(IRetroPlayerStream* stream);
};

using StreamPtr = std::unique_ptr<IRetroPlayerStream, DeleteStream>;

enum class StreamType
{
  AUDIO,
  VIDEO,
  SW_BUFFER,
  HW_BUFFER,
};

enum class PCMFormat
{
 FMT_UNKNOWN,
 FMT_S16NE,
};

enum class AudioChannel
{
  CH_NULL, // Channel list terminator
  CH_FL,
  CH_FR,
  CH_FC,
  CH_LFE,
  CH_BL,
  CH_BR,
  CH_FLOC,
  CH_FROC,
  CH_BC,
  CH_SL,
  CH_SR,
  CH_TFL,
  CH_TFR,
  CH_TFC,
  CH_TC,
  CH_TBL,
  CH_TBR,
  CH_TBC,
  CH_BLOC,
  CH_BROC,
  CH_COUNT
};

using AudioChannelMap = std::array<AudioChannel, static_cast<unsigned int>(AudioChannel::CH_COUNT)>;

enum class PixelFormat
{
  FMT_UNKNOWN,
  FMT_0RGB8888,
  FMT_RGB565,
  FMT_0RGB1555,
};

enum class VideoRotation
{
  ROTATION_0,
  ROTATION_90_CCW,
  ROTATION_180_CCW,
  ROTATION_270_CCW,
};

}
}
