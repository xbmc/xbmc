/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IGameClientStream.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"

#include <vector>

namespace KODI
{
namespace RETRO
{
  class IRetroPlayerStream;
  struct AudioStreamProperties;
}

namespace GAME
{

class CGameClientStreamAudio : public IGameClientStream
{
public:
  CGameClientStreamAudio(double sampleRate);
  ~CGameClientStreamAudio() override { CloseStream(); }

  // Implementation of IGameClientStream
  bool OpenStream(RETRO::IRetroPlayerStream* stream,
                  const game_stream_properties& properties) override;
  void CloseStream() override;
  void AddData(const game_stream_packet &packet) override;

private:
  // Utility functions
  static RETRO::AudioStreamProperties* TranslateProperties(const game_stream_audio_properties &properties, double sampleRate);

  // Construction parameters
  const double m_sampleRate;

  // Stream parameters
  RETRO::IRetroPlayerStream* m_stream = nullptr;
};

} // namespace GAME
} // namespace KODI
