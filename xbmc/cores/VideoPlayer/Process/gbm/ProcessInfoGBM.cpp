/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoGBM.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/Buffers/VideoBufferPoolDMA.h"
#include "rendering/RenderSystem.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoGBM::Create()
{
  return new CProcessInfoGBM();
}

void CProcessInfoGBM::Register()
{
  CProcessInfo::RegisterProcessControl("gbm", CProcessInfoGBM::Create);
}

CProcessInfoGBM::CProcessInfoGBM()
{
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_VIDEOPLAYER_USEPRIMEDECODER))
    m_videoBufferManager.RegisterPool(std::make_shared<CVideoBufferPoolDMA>());
}

EINTERLACEMETHOD CProcessInfoGBM::GetFallbackDeintMethod()
{
#if defined(__arm__)
  return EINTERLACEMETHOD::VS_INTERLACEMETHOD_DEINTERLACE_HALF;
#else
  return CProcessInfo::GetFallbackDeintMethod();
#endif
}

std::vector<AVPixelFormat> CProcessInfoGBM::GetRenderFormats()
{
  std::vector<AVPixelFormat> formats = {
      AV_PIX_FMT_YUV420P,
      AV_PIX_FMT_NV12,
  };

  // For software decode, the ffmpeg codec queries GetRenderFormats before any
  // renderer is created. Since GLES 2.0 vs 3.0+ is a runtime (not compile-time)
  // issue, this method gates >8-bit formats on behalf of the eventual renderer.
  // >8-bit requires GL_EXT_texture_norm16; without it, ffmpeg downconverts to
  // 8-bit, preserving existing behavior.
#if defined(HAS_GLES)
  auto renderSystem = CServiceBroker::GetRenderSystem();
  if (!renderSystem)
  {
    CLog::Log(LOGERROR, "CProcessInfoGBM::GetRenderFormats: render system not initialized");
    return formats;
  }
  if (renderSystem->IsExtSupported("GL_EXT_texture_norm16"))
#endif
  {
    formats.push_back(AV_PIX_FMT_YUV420P9);
    formats.push_back(AV_PIX_FMT_YUV420P10);
    formats.push_back(AV_PIX_FMT_YUV420P12);
    formats.push_back(AV_PIX_FMT_YUV420P14);
    formats.push_back(AV_PIX_FMT_YUV420P16);
  }

  // TODO: verify YUV422 on existing GL renderer, add support to GLES renderer
  // formats.push_back(AV_PIX_FMT_YUYV422);
  // formats.push_back(AV_PIX_FMT_UYVY422);

  return formats;
}
