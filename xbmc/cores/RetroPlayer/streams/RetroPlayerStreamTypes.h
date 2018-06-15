/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
