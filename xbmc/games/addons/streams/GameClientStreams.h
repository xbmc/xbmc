/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "cores/RetroPlayer/streams/RetroPlayerStreamTypes.h"

#include <map>

namespace KODI
{
namespace RETRO
{
class IStreamManager;
}

namespace GAME
{

class CGameClient;
class IGameClientStream;

/*!
 * \ingroup games
 */
class CGameClientStreams
{
public:
  CGameClientStreams(CGameClient& gameClient);

  void Initialize(RETRO::IStreamManager& streamManager);
  void Deinitialize();

  IGameClientStream* OpenStream(const game_stream_properties& properties);
  void CloseStream(IGameClientStream* stream);

private:
  // Utility functions
  std::unique_ptr<IGameClientStream> CreateStream(GAME_STREAM_TYPE streamType) const;

  // Construction parameters
  CGameClient& m_gameClient;

  // Initialization parameters
  RETRO::IStreamManager* m_streamManager = nullptr;

  // Stream parameters
  std::map<IGameClientStream*, RETRO::StreamPtr> m_streams;
};

} // namespace GAME
} // namespace KODI
