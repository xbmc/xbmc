/*
 *      Copyright (C) 2017 Team Kodi
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

#include "cores/RetroPlayer/rendering/RenderSettings.h"
#include "cores/GameSettings.h"
#include "utils/Geometry.h"

extern "C" {
#include "libavutil/pixfmt.h"
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
    CRPBaseRenderer(const CRenderSettings &renderSettings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool);
    virtual ~CRPBaseRenderer();

    /*!
     * \brief Get the buffer pool used by this renderer
     */
    IRenderBufferPool *GetBufferPool() { return m_bufferPool.get(); }

    // Player functions
    bool Configure(AVPixelFormat format, unsigned int width, unsigned int height);
    void FrameMove();
    /*!
     * \brief Performs whatever necessary before rendering the frame
     */
    void PreRender(bool clear);
    void SetBuffer(IRenderBuffer *buffer);
    void RenderFrame(bool clear, uint8_t alpha);

    // Feature support
    virtual bool Supports(RENDERFEATURE feature) const = 0;
    bool IsCompatible(const CRenderVideoSettings &settings) const;
    virtual SCALINGMETHOD GetDefaultScalingMethod() const = 0;

    // Public renderer interface
    virtual void Flush();
    virtual void Deinitialize() { }

    // Get render settings
    const CRenderSettings &GetRenderSettings() const { return m_renderSettings; }

    // Set render settings
    void SetScalingMethod(SCALINGMETHOD method);
    void SetViewMode(VIEWMODE viewMode);
    void SetRenderRotation(unsigned int rotationDegCCW);

    bool IsVisible() const;

  protected:
    // Protected renderer interface
    virtual bool ConfigureInternal() { return true; }
    virtual void RenderInternal(bool clear, uint8_t alpha) = 0;
    virtual void FlushInternal() { }

    /*!
     * \brief Calculate driven dimensions
     */
    virtual void ManageRenderArea();

    float GetAspectRatio() const;
    unsigned int GetRotationDegCCW() const;

    // Construction parameters
    CRenderContext &m_context;
    std::shared_ptr<IRenderBufferPool> m_bufferPool;

    // Stream properties
    bool m_bConfigured = false;
    AVPixelFormat m_format = AV_PIX_FMT_NONE;
    unsigned int m_sourceWidth = 0;
    unsigned int m_sourceHeight = 0;
    unsigned int m_renderOrientation = 0; // Degrees counter-clockwise

    /*!
     * \brief Orientation of the previous frame
     *
     * For drawing the texture with glVertex4f (holds all 4 corner points of the
     * destination rect with correct orientation based on m_renderOrientation.
     */
    unsigned int m_oldRenderOrientation = 0;

    // Rendering properties
    CRenderSettings m_renderSettings;
    CRect m_dimensions;
    IRenderBuffer *m_renderBuffer = nullptr;

    // Geometry properties
    std::array<CPoint, 4> m_rotatedDestCoords;
    CRect m_oldDestRect; // destrect of the previous frame
    CRect m_sourceRect; // original size of the video

  private:
    /*!
     * \brief Performs whatever nessesary after a frame has been rendered
     */
    void PostRender();

    void MarkDirty();

    // Utility functions
    void GetScreenDimensions(float &screenWidth, float &screenHeight, float &screenPixelRatio);

    uint64_t m_renderFrameCount = 0;
    uint64_t m_lastRender = 0;
  };
}
}
