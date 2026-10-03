/*
 *  Copyright (C) 2018-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IGameClientStream.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"

#include <memory>

namespace KODI
{
namespace RETRO
{
class IRetroPlayerStream;
struct HwFramebufferProperties;
} // namespace RETRO

namespace GAME
{

class IHwFramebufferCallback
{
public:
  virtual ~IHwFramebufferCallback() = default;

  /*!
   * \brief Initialize client GPU resources with the hardware context current
   *
   * The add-on's stream handle must be installed before invoking this callback.
   */
  virtual bool HardwareContextReset() = 0;

  /*!
   * \brief Called before the HW context is destroyed
   *
   * Gives the client a chance to release its GPU resources while its context
   * is still current.
   */
  virtual void HardwareContextDestroy() = 0;
};

class CGameClientStreamHwFramebuffer : public IGameClientStream
{
public:
  CGameClientStreamHwFramebuffer(IHwFramebufferCallback& callback,
                                 const game_hw_rendering_properties& hwProperties);
  ~CGameClientStreamHwFramebuffer() override = default;

  // Implementation of IGameClientStream
  bool OpenStream(RETRO::IRetroPlayerStream* stream,
                  const game_stream_properties& properties) override;
  void CloseStream() override;
  bool GetBuffer(unsigned int width, unsigned int height, game_stream_buffer& buffer) override;
  void AddData(const game_stream_packet& packet) override;

  bool ResetHwContext();

  /*!
   * \brief Tell the client its context is going away, at most once
   *
   * Separate from CloseStream() so the client can be told while the game is
   * still loaded, which is the only order some clients survive. Calling it
   * again, including from CloseStream(), does nothing.
   */
  void DestroyHwContext();
  void AbandonHwContext();

  // Public utility functions
  static void LogHwProperties(const game_hw_rendering_properties& hwProperties);
  static std::string GetContextName(GAME_HW_CONTEXT_TYPE contextType,
                                    unsigned int versionMajor,
                                    unsigned int versionMinor);

private:
  // Private utility functions
  static std::unique_ptr<RETRO::HwFramebufferProperties> TranslateProperties(
      const game_hw_rendering_properties& hwProperties,
      const game_stream_hw_framebuffer_properties& streamProperties);

  // Construction parameters
  IHwFramebufferCallback& m_callback;

  // Stream parameters
  RETRO::IRetroPlayerStream* m_stream{nullptr};

  bool m_hwContextEnded{false};
  bool m_hwContextResetStarted{false};
  bool m_hwContextReady{false};

  // Hardware rendering parameters
  const std::unique_ptr<const game_hw_rendering_properties> m_hwProperties;
};

} // namespace GAME
} // namespace KODI
