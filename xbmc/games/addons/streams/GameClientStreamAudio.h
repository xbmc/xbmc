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
