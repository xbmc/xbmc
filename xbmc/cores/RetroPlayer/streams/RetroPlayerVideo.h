/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IRetroPlayerStream.h"
#include "cores/RetroPlayer/RetroPlayerTypes.h"

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

struct VideoStreamProperties : public StreamProperties
{
  VideoStreamProperties(AVPixelFormat pixfmt,
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

struct VideoStreamBuffer : public StreamBuffer
{
  VideoStreamBuffer() = default;

  VideoStreamBuffer(
      AVPixelFormat pixfmt, uint8_t* data, size_t size, DataAccess access, DataAlignment alignment)
    : pixfmt(pixfmt), data(data), size(size), access(access), alignment(alignment)
  {
  }

  AVPixelFormat pixfmt{AV_PIX_FMT_NONE};
  uint8_t* data{nullptr};
  size_t size{0};
  DataAccess access{DataAccess::READ_WRITE};
  DataAlignment alignment{DataAlignment::DATA_UNALIGNED};
};

struct VideoStreamPacket : public StreamPacket
{
  VideoStreamPacket(unsigned int width,
                    unsigned int height,
                    VideoRotation rotation,
                    const uint8_t* data,
                    size_t size)
    : width(width), height(height), rotation(rotation), data(data), size(size)
  {
  }

  unsigned int width;
  unsigned int height;
  VideoRotation rotation;
  const uint8_t* data;
  size_t size;
};

/*!
 * \brief Renders video frames provided by the game loop
 *
 * \sa CRPRenderManager
 */
class CRetroPlayerVideo : public IRetroPlayerStream
{
public:
  CRetroPlayerVideo(CRPRenderManager& m_renderManager, CRPProcessInfo& m_processInfo);
  ~CRetroPlayerVideo() override;

  // implementation of IRetroPlayerStream
  bool OpenStream(const StreamProperties& properties) override;
  bool GetStreamBuffer(unsigned int width, unsigned int height, StreamBuffer& buffer) override;
  void AddStreamData(const StreamPacket& packet) override;
  void CloseStream() override;

private:
  // Construction parameters
  CRPRenderManager& m_renderManager;
  CRPProcessInfo& m_processInfo;

  // Stream properties
  bool m_bOpen = false;
};
} // namespace RETRO
} // namespace KODI
