/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#pragma once

#include "IRetroPlayerStream.h"
#include "RetroPlayerStreamTypes.h"
#include "cores/RetroPlayer/buffers/IRenderBufferPool.h"

#include <memory>
#include <stdint.h>

extern "C"
{
#include <libavutil/pixfmt.h>
}

//! @todo RetroPlayer needs an abstraction for GAME_HW_CONTEXT_TYPE
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"

namespace KODI
{
namespace RETRO
{
class CRPProcessInfo;
class CRPRenderManager;

struct HwFramebufferProperties : public StreamProperties
{
  HwFramebufferProperties(GAME_HW_CONTEXT_TYPE contextType,
                          bool depth,
                          bool stencil,
                          bool bottomLeftOrigin,
                          unsigned int versionMajor,
                          unsigned int versionMinor,
                          bool cacheContext,
                          bool debugContext,
                          unsigned int maxWidth,
                          unsigned int maxHeight,
                          float nominalDisplayAspectRatio)
    : contextType(contextType),
      depth(depth),
      stencil(stencil),
      bottomLeftOrigin(bottomLeftOrigin),
      versionMajor(versionMajor),
      versionMinor(versionMinor),
      cacheContext(cacheContext),
      debugContext(debugContext),
      maxWidth(maxWidth),
      maxHeight(maxHeight),
      nominalDisplayAspectRatio(nominalDisplayAspectRatio)
  {
  }

  GAME_HW_CONTEXT_TYPE contextType;
  bool depth;
  bool stencil;
  bool bottomLeftOrigin;
  unsigned int versionMajor;
  unsigned int versionMinor;
  bool cacheContext;
  bool debugContext;
  unsigned int maxWidth;
  unsigned int maxHeight;
  float nominalDisplayAspectRatio;
};

struct HwFramebufferBuffer : public StreamBuffer
{
  HwFramebufferBuffer() = default;
  HwFramebufferBuffer(uintptr_t framebuffer) : framebuffer(framebuffer) {}

  uintptr_t framebuffer{};
};

struct HwFramebufferPacket : public StreamPacket
{
  HwFramebufferPacket() = default;
  HwFramebufferPacket(uintptr_t framebuffer,
                      unsigned int width,
                      unsigned int height,
                      float displayAspectRatio,
                      VideoRotation rotation)
    : framebuffer(framebuffer),
      width(width),
      height(height),
      displayAspectRatio(displayAspectRatio),
      rotation(rotation)
  {
  }

  uintptr_t framebuffer{};

  //! \brief Size of the image the client drew, which the framebuffer may exceed
  unsigned int width{};
  unsigned int height{};
  float displayAspectRatio{};
  VideoRotation rotation{VideoRotation::ROTATION_0};
};

class CRetroPlayerRendering : public IRetroPlayerStream
{
public:
  CRetroPlayerRendering(CRPRenderManager& m_renderManager, CRPProcessInfo& m_processInfo);

  ~CRetroPlayerRendering() override;

  // Implementation of IRetroPlayerStream
  bool OpenStream(const StreamProperties& properties) override;
  bool GetStreamBuffer(unsigned int width, unsigned int height, StreamBuffer& buffer) override;
  void AddStreamData(const StreamPacket& packet) override;
  void CloseStream() override;

private:
  /*!
   * \brief Configure the render manager for the given frame size
   */
  bool Configure(unsigned int width, unsigned int height);

  /*!
   * \brief Translate the client's request into a rendering-system-agnostic form
   */
  static HwContextProperties TranslateContextProperties(const HwFramebufferProperties& properties);

  // Construction parameters
  CRPRenderManager& m_renderManager;
  CRPProcessInfo& m_processInfo;

  // Stream parameters
  bool m_bOpen = false;
  bool m_loggedHardwareFrame{false};
  std::unique_ptr<HwFramebufferProperties> m_hwProperties;
  unsigned int m_width = 0;
  unsigned int m_height = 0;
  unsigned int m_frameWidth{0};
  unsigned int m_frameHeight{0};
};
} // namespace RETRO
} // namespace KODI
