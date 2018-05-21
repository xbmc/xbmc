/*
 *      Copyright (C) 2016-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "cores/AudioEngine/Utils/AEChannelData.h"

#include "libavcodec/avcodec.h"
#include "libavutil/pixfmt.h"

#include <stdint.h>

class CAEChannelInfo;

namespace KODI
{
namespace GAME
{
  class IGameAudioCallback
  {
  public:
    virtual ~IGameAudioCallback() = default;

    virtual unsigned int NormalizeSamplerate(unsigned int samplerate) const = 0;
    virtual bool OpenPCMStream(AEDataFormat format, unsigned int samplerate, const CAEChannelInfo& channelLayout) = 0;
    virtual bool OpenEncodedStream(AVCodecID codec, unsigned int samplerate, const CAEChannelInfo& channelLayout) = 0;
    virtual void AddData(const uint8_t* data, unsigned int size) = 0;
    virtual void CloseStream() = 0;
  };

  class IGameVideoCallback
  {
  public:
    virtual ~IGameVideoCallback() = default;

    virtual bool OpenPixelStream(AVPixelFormat pixfmt, unsigned int width, unsigned int height, unsigned int orientationDeg) = 0;
    virtual bool OpenEncodedStream(AVCodecID codec) = 0;
    virtual void AddData(const uint8_t* data, unsigned int size) = 0;
    virtual void CloseStream() = 0;
  };

  class IGameInputCallback
  {
  public:
    virtual ~IGameInputCallback() = default;

    virtual void PollInput() = 0;
  };
}
}
