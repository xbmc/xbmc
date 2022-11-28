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

#include <stdint.h>

extern "C"
{
#include <libavutil/pixfmt.h>
}

namespace KODI
{
namespace RETRO
{
class CRPProcessInfo;
class CRPRenderManager;

struct HwFramebufferProperties : public StreamProperties
{
  HwFramebufferProperties(AVPixelFormat pixfmt,
                          unsigned int nominalWidth,
                          unsigned int nominalHeight,
                          unsigned int maxWidth,
                          unsigned int maxHeight,
                          float pixelAspectRatio)
    : pixfmt(pixfmt),
      nominalWidth(nominalWidth),
      nominalHeight(nominalHeight),
      maxWidth(maxWidth),
      maxHeight(maxHeight),
      pixelAspectRatio(pixelAspectRatio)
  {
  }

  AVPixelFormat pixfmt;
  unsigned int nominalWidth;
  unsigned int nominalHeight;
  unsigned int maxWidth;
  unsigned int maxHeight;
  float pixelAspectRatio;
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
  HwFramebufferPacket(uintptr_t framebuffer) : framebuffer(framebuffer) {}

  uintptr_t framebuffer{};
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
  // Construction parameters
  CRPRenderManager& m_renderManager;
  CRPProcessInfo& m_processInfo;
};
} // namespace RETRO
} // namespace KODI
