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

#include "IRenderCallback.h"
#include "IRenderManager.h"
#include "RenderVideoSettings.h"
#include "threads/CriticalSection.h"

extern "C" {
#include "libavutil/pixfmt.h"
}

#include <atomic>
#include <map>
#include <memory>
#include <set>
#include <vector>

struct SwsContext;

namespace KODI
{
namespace RETRO
{
  class CGUIRenderTargetFactory;
  class CRenderContext;
  class CRenderSettings;
  class CRPBaseRenderer;
  class CRPProcessInfo;
  class IGUIRenderSettings;
  class IRenderBuffer;
  class IRenderBufferPool;

  class CRPRenderManager : public IRenderManager,
                           public IRenderCallback
  {
  public:
    CRPRenderManager(CRPProcessInfo &processInfo);
    ~CRPRenderManager() override = default;

    void Initialize();
    void Deinitialize();

    CGUIRenderTargetFactory *GetGUIRenderTargetFactory() { return m_renderControlFactory.get(); }

    // Functions called from game loop
    bool Configure(AVPixelFormat format, unsigned int width, unsigned int height, unsigned int orientation);
    void AddFrame(const uint8_t* data, unsigned int size);

    // Functions called from the player
    void SetSpeed(double speed);

    // Functions called from render thread
    void FrameMove();
    void Flush();
    void TriggerUpdateResolution();

    // Implementation of IRenderManager
    void RenderWindow(bool bClear, const RESOLUTION_INFO &coordsRes) override;
    void RenderControl(bool bClear, bool bUseAlpha, const CRect &renderRegion, const IGUIRenderSettings *renderSettings) override;
    void ClearBackground() override;

    // Implementation of IRenderCallback
    bool SupportsRenderFeature(ERENDERFEATURE feature) const override;
    bool SupportsScalingMethod(ESCALINGMETHOD method) const override;

  private:
    std::shared_ptr<CRPBaseRenderer> GetRenderer(const IGUIRenderSettings *renderSettings);
    std::shared_ptr<CRPBaseRenderer> GetRenderer(IRenderBufferPool *bufferPool, const CRenderSettings &renderSettings);

    void RenderInternal(const std::shared_ptr<CRPBaseRenderer> &renderer, bool bClear, uint32_t alpha);

    bool HasRenderBuffer(IRenderBufferPool *bufferPool);
    IRenderBuffer *GetRenderBuffer(IRenderBufferPool *bufferPool);
    void CreateRenderBuffer(IRenderBufferPool *bufferPool);

    void UpdateResolution();

    void CopyFrame(IRenderBuffer *renderBuffer, const uint8_t *data, size_t size, AVPixelFormat format);

    CRenderVideoSettings GetEffectiveSettings(const IGUIRenderSettings *settings) const;

    // Construction parameters
    CRPProcessInfo &m_processInfo;
    CRenderContext &m_renderContext;

    // Stream properties
    AVPixelFormat m_format = AV_PIX_FMT_NONE;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    unsigned int m_orientation = 0; // Degrees counter-clockwise

    // Render properties
    enum class RENDER_STATE
    {
      UNCONFIGURED,
      CONFIGURING,
      CONFIGURED,
    };
    RENDER_STATE m_state = RENDER_STATE::UNCONFIGURED;
    std::atomic<double> m_speed;
    std::shared_ptr<IGUIRenderSettings> m_renderSettings;
    std::set<std::shared_ptr<CRPBaseRenderer>> m_renderers;
    std::shared_ptr<CGUIRenderTargetFactory> m_renderControlFactory;
    std::vector<IRenderBuffer*> m_renderBuffers;
    std::vector<uint8_t> m_cachedFrame;
    std::map<AVPixelFormat, SwsContext*> m_scalers;
    bool m_bHasCachedFrame = false;
    bool m_bTriggerUpdateResolution = false;
    CCriticalSection m_stateMutex;
    CCriticalSection m_bufferMutex;
  };
}
}
