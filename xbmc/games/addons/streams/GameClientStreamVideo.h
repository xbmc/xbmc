/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
} // namespace RETRO

namespace GAME
{

/*!
 * \ingroup games
 */
class CGameClientStreamVideo : public IGameClientStream
{
public:
  CGameClientStreamVideo() = default;
  ~CGameClientStreamVideo() override { CloseStream(); }

  // Implementation of IGameClientStream
  bool OpenStream(RETRO::IRetroPlayerStream* stream,
                  const game_stream_properties& properties) override;
  void CloseStream() override;
  void AddData(const game_stream_packet& packet) override;

protected:
  // Stream parameters
  RETRO::IRetroPlayerStream* m_stream = nullptr;

private:
  // Utility functions
  static RETRO::VideoStreamProperties* TranslateProperties(
      const game_stream_video_properties& properties);
};

} // namespace GAME
} // namespace KODI
