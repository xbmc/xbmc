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

  // Lifecycle functions
  void Initialize(RETRO::IStreamManager& streamManager);
  void Deinitialize();

  // Stream management functions
  IGameClientStream* OpenStream(const game_stream_properties& properties);
  void CloseStream(IGameClientStream* stream);
  void SetGameTiming(const game_system_timing& timingInfo);

  // HW rendering functions
  bool EnableHardwareRendering(const game_hw_rendering_properties& properties);
  game_proc_address_t GetHwProcedureAddress(const char* sym);
  /*!
   * \brief Make a hardware-rendering client's context current on this thread
   *
   * Every call into the client has to be bracketed with this and
   * EndClientFrame(), because a client may negotiate hardware rendering or
   * open its rendering stream inside the call. End only after a successful begin.
   */
  bool BeginClientFrame();

  /*!
   * \brief Give this thread back the binding it had
   */
  void EndClientFrame();

  /*!
   * \brief Tell a hardware-rendering client its context is going away
   *
   * Called before the game is unloaded, while the client's context is still
   * current: a client that is told afterwards has already dismantled the state
   * its context_destroy goes on to use. Does nothing if there is no hardware
   * rendering client, and nothing on a second call.
   */
  void DestroyHwContext();

  bool HardwareRenderingAttempted() const
  {
    return m_hwProperties.context_type != GAME_HW_CONTEXT_NONE || !m_hwRefusedWanted.empty();
  }

  /*!
   * \brief What the client asked to render with, if that had to be refused
   *
   * Empty unless hardware rendering was turned down. Named for the user, e.g.
   * "OpenGL 4.3".
   */
  const std::string& HardwareRenderingRefusedWanted() const { return m_hwRefusedWanted; }

  /*!
   * \brief What this system can render with, if it is the reason for a refusal
   *
   * Empty when the display could not provide hardware rendering at all, as
   * opposed to providing too old a version of it.
   */
  const std::string& HardwareRenderingRefusedAvailable() const { return m_hwRefusedAvailable; }

private:
  // Utility functions
  std::unique_ptr<IGameClientStream> CreateStream(GAME_STREAM_TYPE streamType) const;

  // Construction parameters
  CGameClient& m_gameClient;

  // Initialization parameters
  RETRO::IStreamManager* m_streamManager = nullptr;

  // Stream parameters
  std::map<IGameClientStream*, RETRO::StreamPtr> m_streams;

  // Hardware rendering parameters
  game_hw_rendering_properties m_hwProperties{};

  // Why hardware rendering was refused, for the message the user sees
  std::string m_hwRefusedWanted;
  std::string m_hwRefusedAvailable;
};

} // namespace GAME
} // namespace KODI
