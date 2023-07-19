/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRetroPlayerStream.h"
#include "cores/AudioEngine/Interfaces/AE.h"

#include <memory>

class IAEStream;

namespace KODI
{
namespace RETRO
{
class CRPProcessInfo;

struct AudioStreamProperties : public StreamProperties
{
  AudioStreamProperties(PCMFormat format, double sampleRate, AudioChannelMap channelMap)
    : format(format), sampleRate(sampleRate), channelMap(channelMap)
  {
  }

  PCMFormat format;
  double sampleRate;
  AudioChannelMap channelMap;
};

struct AudioStreamPacket : public StreamPacket
{
  AudioStreamPacket(const uint8_t* data, size_t size) : data(data), size(size) {}

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
  bool GetStreamBuffer(unsigned int width, unsigned int height, StreamBuffer& buffer) override
  {
    return false;
  }
  void AddStreamData(const StreamPacket& packet) override;
  void CloseStream() override;

private:
  CRPProcessInfo& m_processInfo;
  IAE::StreamPtr m_pAudioStream;
  bool m_bAudioEnabled = true;
};
} // namespace RETRO
} // namespace KODI
