/*
 *      Copyright (C) 2012-2017 Team Kodi
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

#include "IRetroPlayerStream.h"

extern "C" {
#include "libavutil/pixfmt.h"
}

namespace KODI
{
namespace RETRO
{
  class CRPProcessInfo;
  class CRPRenderManager;

  struct VideoStreamProperties : public StreamProperties
  {
    VideoStreamProperties(AVPixelFormat pixfmt, unsigned int nominalWidth, unsigned int nominalHeight, unsigned int maxWidth, unsigned int maxHeight, float pixelAspectRatio) :
      pixfmt(pixfmt),
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
    AVPixelFormat pixfmt;
    uint8_t *data;
    size_t size;
  };

  struct VideoStreamPacket: public StreamPacket
  {
    VideoStreamPacket(unsigned int width, unsigned int height, VideoRotation rotation, const uint8_t *data, size_t size) :
      width(width),
      height(height),
      rotation(rotation),
      data(data),
      size(size)
    {
    }

    unsigned int width;
    unsigned int height;
    VideoRotation rotation;
    const uint8_t *data;
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
    void AddStreamData(const StreamPacket &packet) override;
    void CloseStream() override;

  private:
    // Construction parameters
    CRPRenderManager& m_renderManager;
    CRPProcessInfo& m_processInfo;

    // Stream properties
    bool m_bOpen = false;
  };
}
}
