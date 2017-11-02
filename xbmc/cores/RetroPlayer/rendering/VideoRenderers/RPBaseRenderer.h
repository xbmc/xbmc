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

#include "cores/IPlayer.h"
#include "cores/RetroPlayer/rendering/RenderSettings.h"
#include "guilib/Geometry.h"

#include "libavutil/pixfmt.h"

#include <atomic>
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
    bool Configure(AVPixelFormat format, unsigned int width, unsigned int height, unsigned int orientation);
    void FrameMove();
    /*!
     * \brief Performs whatever necessary before rendering the frame
     */
    void PreRender(bool clear);
    void SetBuffer(IRenderBuffer *buffer);
    void RenderFrame(bool clear, uint8_t alpha);

    // Feature support
    virtual bool Supports(ERENDERFEATURE feature) const = 0;
    bool IsCompatible(const CRenderVideoSettings &settings) const;
    virtual ESCALINGMETHOD GetDefaultScalingMethod() const = 0;

    // Public renderer interface
    virtual void Flush();
    virtual void Deinitialize() { }

    // Get render settings
    const CRenderSettings &GetRenderSettings() const { return m_renderSettings; }

    // Set render settings
    void SetScalingMethod(ESCALINGMETHOD method);
    void SetViewMode(ViewMode viewMode);

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

    /*!
     * \brief Get video rectangle and view window
     *
     * \param source is original size of the video
     * \param dest is the target rendering area honoring aspect ratio of source
     * \param view is the entire target rendering area for the video (including black bars)
     */
    void GetVideoRect(CRect &source, CRect &dest, CRect &view) const;
    float GetAspectRatio() const;

    // Construction parameters
    CRenderContext &m_context;
    std::shared_ptr<IRenderBufferPool> m_bufferPool;

    // Stream properties
    bool m_bConfigured = false;
    AVPixelFormat m_format = AV_PIX_FMT_NONE;
    unsigned int m_sourceWidth = 0;
    unsigned int m_sourceHeight = 0;
    float m_sourceFrameRatio = 1.0f;
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
    float m_pixelRatio = 1.0f;
    float m_zoomAmount = 1.0f;
    bool m_bNonLinearStretch = false;
    IRenderBuffer *m_renderBuffer = nullptr;

    // Geometry properties
    CPoint m_rotatedDestCoords[4];
    CRect m_oldDestRect; // destrect of the previous frame
    CRect m_sourceRect;
    CRect m_viewRect;

  private:
    bool IsNonLinearStretch() const { return m_bNonLinearStretch; }

    /*!
     * \brief Performs whatever nessesary after a frame has been rendered
     */
    void PostRender();

    void CalcNormalRenderRect(float offsetX, float offsetY, float width, float height, float inputFrameRatio, float zoomAmount);
    void CalculateViewMode();

    void UpdateDrawPoints(const CRect &destRect);
    void ReorderDrawPoints();
    void MarkDirty();
    float GetAllowedErrorInAspect() const;

    uint64_t m_renderFrameCount = 0;
    uint64_t m_lastRender = 0;
  };
}
}
