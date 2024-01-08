/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPProcessInfo.h"

#include "ServiceBroker.h"
#include "cores/DataCacheCore.h"
#include "cores/RetroPlayer/buffers/RenderBufferManager.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

#include <mutex>

extern "C"
{
#include <libavutil/pixdesc.h>
}

#include <utility>

using namespace KODI;
using namespace RETRO;

CreateRPProcessControl CRPProcessInfo::m_processControl = nullptr;
std::vector<std::unique_ptr<IRendererFactory>> CRPProcessInfo::m_rendererFactories;
CCriticalSection CRPProcessInfo::m_createSection;

CRPProcessInfo::CRPProcessInfo(std::string platformName)
  : m_platformName(std::move(platformName)),
    m_renderBufferManager(new CRenderBufferManager),
    m_renderContext(new CRenderContext(CServiceBroker::GetRenderSystem(),
                                       CServiceBroker::GetWinSystem(),
                                       CServiceBroker::GetWinSystem()->GetGfxContext(),
                                       CDisplaySettings::GetInstance(),
                                       CMediaSettings::GetInstance(),
                                       CServiceBroker::GetGameServices(),
                                       CServiceBroker::GetGUI()))
{
  for (auto& rendererFactory : m_rendererFactories)
  {
    RenderBufferPoolVector bufferPools = rendererFactory->CreateBufferPools(*m_renderContext);
    if (!bufferPools.empty())
      m_renderBufferManager->RegisterPools(rendererFactory.get(), std::move(bufferPools));
  }

  // Initialize default scaling method
  for (auto scalingMethod : GetScalingMethods())
  {
    if (HasScalingMethod(scalingMethod))
    {
      m_defaultScalingMethod = scalingMethod;
      break;
    }
  }
}

CRPProcessInfo::~CRPProcessInfo() = default;

std::unique_ptr<CRPProcessInfo> CRPProcessInfo::CreateInstance()
{
  std::unique_ptr<CRPProcessInfo> processInfo;

  std::unique_lock<CCriticalSection> lock(m_createSection);

  if (m_processControl != nullptr)
  {
    processInfo = m_processControl();

    if (processInfo)
      CLog::Log(LOGINFO, "RetroPlayer[PROCESS]: Created process info for {}",
                processInfo->GetPlatformName());
    else
      CLog::Log(LOGERROR, "RetroPlayer[PROCESS]: Failed to create process info");
  }
  else
  {
    CLog::Log(LOGERROR, "RetroPlayer[PROCESS]: No process control registered");
  }

  return processInfo;
}

void CRPProcessInfo::RegisterProcessControl(const CreateRPProcessControl& createFunc)
{
  m_processControl = createFunc;
}

void CRPProcessInfo::RegisterRendererFactory(IRendererFactory* factory)
{
  std::unique_lock<CCriticalSection> lock(m_createSection);

  CLog::Log(LOGINFO, "RetroPlayer[RENDER]: Registering renderer factory for {}",
            factory->RenderSystemName());

  m_rendererFactories.emplace_back(factory);
}

std::string CRPProcessInfo::GetRenderSystemName(IRenderBufferPool* renderBufferPool) const
{
  return m_renderBufferManager->GetRenderSystemName(renderBufferPool);
}

CRPBaseRenderer* CRPProcessInfo::CreateRenderer(IRenderBufferPool* renderBufferPool,
                                                const CRenderSettings& renderSettings)
{
  std::unique_lock<CCriticalSection> lock(m_createSection);

  for (auto& rendererFactory : m_rendererFactories)
  {
    RenderBufferPoolVector bufferPools = m_renderBufferManager->GetPools(rendererFactory.get());
    for (auto& bufferPool : bufferPools)
    {
      if (bufferPool.get() == renderBufferPool)
        return rendererFactory->CreateRenderer(renderSettings, *m_renderContext,
                                               std::move(bufferPool));
    }
  }

  CLog::Log(LOGERROR, "RetroPlayer[RENDER]: Failed to find a suitable renderer factory");

  return nullptr;
}

void CRPProcessInfo::SetDataCache(CDataCacheCore* cache)
{
  m_dataCache = cache;
  ;
}

void CRPProcessInfo::ResetInfo()
{
  if (m_dataCache != nullptr)
  {
    m_dataCache->Reset();
  }
}

bool CRPProcessInfo::HasScalingMethod(SCALINGMETHOD scalingMethod) const
{
  return m_renderBufferManager->HasScalingMethod(scalingMethod);
}

std::vector<SCALINGMETHOD> CRPProcessInfo::GetScalingMethods()
{
  return {
      SCALINGMETHOD::NEAREST,
      SCALINGMETHOD::LINEAR,
  };
}

//******************************************************************************
// video codec
//******************************************************************************
void CRPProcessInfo::SetVideoPixelFormat(AVPixelFormat pixFormat)
{
  const char* videoPixelFormat = av_get_pix_fmt_name(pixFormat);

  if (m_dataCache != nullptr)
    m_dataCache->SetVideoPixelFormat(videoPixelFormat != nullptr ? videoPixelFormat : "");
}

void CRPProcessInfo::SetVideoDimensions(int width, int height)
{
  if (m_dataCache != nullptr)
    m_dataCache->SetVideoDimensions(width, height);
}

void CRPProcessInfo::SetVideoFps(float fps)
{
  if (m_dataCache != nullptr)
    m_dataCache->SetVideoFps(fps);
}

//******************************************************************************
// player audio info
//******************************************************************************
void CRPProcessInfo::SetAudioChannels(const std::string& channels)
{
  if (m_dataCache != nullptr)
    m_dataCache->SetAudioChannels(channels);
}

void CRPProcessInfo::SetAudioSampleRate(int sampleRate)
{
  if (m_dataCache != nullptr)
    m_dataCache->SetAudioSampleRate(sampleRate);
}

void CRPProcessInfo::SetAudioBitsPerSample(int bitsPerSample)
{
  if (m_dataCache != nullptr)
    m_dataCache->SetAudioBitsPerSample(bitsPerSample);
}

//******************************************************************************
// player states
//******************************************************************************
void CRPProcessInfo::SetSpeed(float speed)
{
  if (m_dataCache != nullptr)
    m_dataCache->SetSpeed(1.0f, speed);
}

void CRPProcessInfo::SetPlayTimes(time_t start, int64_t current, int64_t min, int64_t max)
{
  if (m_dataCache != nullptr)
    m_dataCache->SetPlayTimes(start, current, min, max);
}
