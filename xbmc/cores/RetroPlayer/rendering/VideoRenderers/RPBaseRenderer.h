/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"
#include "cores/RetroPlayer/rendering/RenderSettings.h"
#include "utils/Geometry.h"

extern "C"
{
#include <libavutil/pixfmt.h>
}

#include <array>
#include <memory>
#include <stdint.h>

namespace KODI
{
namespace RETRO
{
class CRenderContext;
class IRenderBuffer;
class IRenderBufferPool;

class CRPBaseRenderer
{
public:
  CRPBaseRenderer(const CRenderSettings& renderSettings,
                  CRenderContext& context,
                  std::shared_ptr<IRenderBufferPool> bufferPool);
  virtual ~CRPBaseRenderer();

  /*!
   * \brief Get the buffer pool used by this renderer
   */
  IRenderBufferPool* GetBufferPool() { return m_bufferPool.get(); }

  // Player functions
  bool Configure(AVPixelFormat format);
  void FrameMove();
  /*!
   * \brief Performs whatever necessary before rendering the frame
   */
  void PreRender(bool clear);
  void SetBuffer(IRenderBuffer* buffer);
  void RenderFrame(bool clear, uint8_t alpha);

  // Feature support
  virtual bool Supports(RENDERFEATURE feature) const = 0;
  bool IsCompatible(const CRenderVideoSettings& settings) const;
  virtual SCALINGMETHOD GetDefaultScalingMethod() const = 0;

  // Public renderer interface
  virtual void Flush();

  // Get render settings
  const CRenderSettings& GetRenderSettings() const { return m_renderSettings; }

  // Set render settings
  void SetScalingMethod(SCALINGMETHOD method);
  void SetStretchMode(STRETCHMODE stretchMode);
  void SetRenderRotation(unsigned int rotationDegCCW);
  void SetPixels(const std::string& pixelPath);

  // Rendering properties
  bool IsVisible() const;
  IRenderBuffer* GetRenderBuffer() const;

protected:
  // Protected renderer interface
  virtual bool ConfigureInternal() { return true; }
  virtual void RenderInternal(bool clear, uint8_t alpha) = 0;
  virtual void FlushInternal() {}

  // Construction parameters
  CRenderContext& m_context;
  std::shared_ptr<IRenderBufferPool> m_bufferPool;

  // Stream properties
  bool m_bConfigured = false;
  AVPixelFormat m_format = AV_PIX_FMT_NONE;

  // Rendering properties
  CRenderSettings m_renderSettings;
  IRenderBuffer* m_renderBuffer = nullptr;

  // Geometry properties
  CRect m_sourceRect;
  std::array<CPoint, 4> m_rotatedDestCoords{};

private:
  /*!
   * \brief Calculate driven dimensions
   */
  virtual void ManageRenderArea(const IRenderBuffer& renderBuffer);

  /*!
   * \brief Performs whatever necessary after a frame has been rendered
   */
  void PostRender();

  void MarkDirty();

  // Utility functions
  void GetScreenDimensions(float& screenWidth, float& screenHeight, float& screenPixelRatio);

  uint64_t m_renderFrameCount = 0;
  uint64_t m_lastRender = 0;
};
} // namespace RETRO
} // namespace KODI
