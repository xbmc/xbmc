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

struct game_stream_video_properties;

namespace KODI
{
namespace RETRO
{
  class IRetroPlayerStream;
  struct VideoStreamProperties;
}

namespace GAME
{

class CGameClientStreamVideo : public IGameClientStream
{
public:
  CGameClientStreamVideo() = default;
  ~CGameClientStreamVideo() override = default;

  // Implementation of IGameClientStream
  bool OpenStream(RETRO::IRetroPlayerStream* stream,
                  const game_stream_properties& properties) override;
  void CloseStream() override;
  void AddData(const game_stream_packet& packet) override;

private:
  // Utility functions
  static RETRO::VideoStreamProperties* TranslateProperties(const game_stream_video_properties &properties);

  // Stream parameters
  RETRO::IRetroPlayerStream* m_stream;
};

} // namespace GAME
} // namespace KODI
