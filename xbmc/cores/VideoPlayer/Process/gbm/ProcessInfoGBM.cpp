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
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

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
  return {
      AV_PIX_FMT_YUV420P,   AV_PIX_FMT_NV12,      AV_PIX_FMT_YUV420P9,  AV_PIX_FMT_YUV420P10,
      AV_PIX_FMT_YUV420P12, AV_PIX_FMT_YUV420P14, AV_PIX_FMT_YUV420P16,
      // TODO: verify YUV422 on existing GL renderer, add support to GLES renderer
      // AV_PIX_FMT_YUYV422,
      // AV_PIX_FMT_UYVY422,
  };
}
