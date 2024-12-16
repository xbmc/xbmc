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
   * \brief Invalidates the current HW context and reinitializes GPU resources
   *
   * Any GL state is lost, and must not be deinitialized explicitly.
   */
  virtual void HardwareContextReset() = 0;
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

  // Public utility functions
  static void LogHwProperties(const game_hw_rendering_properties& hwProperties);

private:
  // Private utility functions
  static std::string GetContextName(GAME_HW_CONTEXT_TYPE contextType,
                                    unsigned int versionMajor,
                                    unsigned int versionMinor);
  static std::unique_ptr<RETRO::HwFramebufferProperties> TranslateProperties(
      const game_hw_rendering_properties& hwProperties);

  // Construction parameters
  IHwFramebufferCallback& m_callback;

  // Stream parameters
  RETRO::IRetroPlayerStream* m_stream{nullptr};

  // Hardware rendering parameters
  const std::unique_ptr<const game_hw_rendering_properties> m_hwProperties;
};

} // namespace GAME
} // namespace KODI
