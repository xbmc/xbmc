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
#include "cores/RetroPlayer/RetroPlayerTypes.h"
#include "threads/CriticalSection.h"

#include "libavutil/pixfmt.h"

#include <memory>
#include <string>
#include <vector>

class CDataCacheCore;

namespace KODI
{
namespace RETRO
{
  class CRenderBufferManager;
  class CRenderContext;
  class CRenderSettings;
  class CRPBaseRenderer;
  class CRPProcessInfo;
  class IRenderBufferPool;

  using CreateRPProcessControl = CRPProcessInfo* (*)();

  class IRendererFactory
  {
  public:
    virtual ~IRendererFactory() = default;

    virtual std::string RenderSystemName() const = 0;
    virtual CRPBaseRenderer *CreateRenderer(const CRenderSettings &settings, CRenderContext &context, std::shared_ptr<IRenderBufferPool> bufferPool) = 0;
    virtual RenderBufferPoolVector CreateBufferPools() = 0;
  };

  class CRPProcessInfo
  {
  public:
    static CRPProcessInfo* CreateInstance();
    static void RegisterProcessControl(CreateRPProcessControl createFunc);
    static void RegisterRendererFactory(IRendererFactory *factory);

    virtual ~CRPProcessInfo();

    const std::string &GetPlatformName() const { return m_platformName; }
    std::string GetRenderSystemName(IRenderBufferPool *renderBufferPool) const;

    CRPBaseRenderer *CreateRenderer(IRenderBufferPool *renderBufferPool, const CRenderSettings &renderSettings);

    void SetDataCache(CDataCacheCore *cache);
    void ResetInfo();

    // rendering info
    CRenderContext &GetRenderContext() { return *m_renderContext; }
    CRenderBufferManager &GetBufferManager() { return *m_renderBufferManager; }
    bool HasScalingMethod(ESCALINGMETHOD scalingMethod) const;
    ESCALINGMETHOD GetDefaultScalingMethod() const { return m_defaultScalingMethod; }

    // player video
    void SetVideoPixelFormat(AVPixelFormat pixFormat);
    void SetVideoDimensions(int width, int height);
    void SetVideoFps(float fps);

    // player audio info
    void SetAudioChannels(const std::string &channels);
    void SetAudioSampleRate(int sampleRate);
    void SetAudioBitsPerSample(int bitsPerSample);

    // player states
    void SetSpeed(float speed);
    void SetPlayTimes(time_t start, int64_t current, int64_t min, int64_t max);

  protected:
    CRPProcessInfo(std::string platformName);

    static std::vector<ESCALINGMETHOD> GetScalingMethods();

    // Static factories
    static CreateRPProcessControl m_processControl;
    static std::vector<std::unique_ptr<IRendererFactory>> m_rendererFactories;
    static CCriticalSection m_createSection;

    // Construction parameters
    const std::string m_platformName;

    // Process info parameters
    CDataCacheCore *m_dataCache = nullptr;

    // Rendering parameters
    std::unique_ptr<CRenderBufferManager> m_renderBufferManager;

  private:
    // Rendering parameters
    std::unique_ptr<CRenderContext> m_renderContext;
    ESCALINGMETHOD m_defaultScalingMethod = VS_SCALINGMETHOD_AUTO;
  };

}
}
