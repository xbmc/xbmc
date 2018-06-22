/*
 *      Copyright (C) 2012-2017 Team Kodi
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

#include "IRetroPlayerStream.h"

#include <memory>

class IAEStream;

namespace KODI
{
namespace RETRO
{
  class CRPProcessInfo;

  struct AudioStreamProperties : public StreamProperties
  {
    AudioStreamProperties(PCMFormat format, double sampleRate, AudioChannelMap channelMap) :
      format(format),
      sampleRate(sampleRate),
      channelMap(channelMap)
    {
    }

    PCMFormat format;
    double sampleRate;
    AudioChannelMap channelMap;
  };

  struct AudioStreamPacket : public StreamPacket
  {
    AudioStreamPacket(const uint8_t* data, size_t size) :
      data(data),
      size(size)
    {
    }

    const uint8_t* data;
    size_t size;
  };

  class CRetroPlayerAudio : public IRetroPlayerStream
  {
  public:
    explicit CRetroPlayerAudio(CRPProcessInfo& processInfo);
    ~CRetroPlayerAudio() override;

    void Enable(bool bEnabled) { m_bAudioEnabled = bEnabled; }

    // implementation of IRetroPlayerStream
    bool OpenStream(const StreamProperties& properties) override;
    bool GetStreamBuffer(unsigned int width, unsigned int height, StreamBuffer& buffer) override { return false; }
    void AddStreamData(const StreamPacket& packet) override;
    void CloseStream() override;

  private:
    CRPProcessInfo& m_processInfo;
    IAEStream* m_pAudioStream;
    bool m_bAudioEnabled;
  };
}
}
