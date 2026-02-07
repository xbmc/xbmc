/*
 *  Copyright (C) 2019-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoGBM.h"

#include "cores/VideoPlayer/Buffers/VideoBufferPoolDMA.h"

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
  m_videoBufferManager.RegisterPool(std::make_shared<CVideoBufferPoolDMA>());
}

std::vector<AVPixelFormat> CProcessInfoGBM::GetRenderFormats()
{
  return {
      AV_PIX_FMT_YUV420P,
      AV_PIX_FMT_NV12,
      AV_PIX_FMT_P010,
  };
}

EINTERLACEMETHOD CProcessInfoGBM::GetFallbackDeintMethod()
{
#if defined(__arm__)
  return EINTERLACEMETHOD::VS_INTERLACEMETHOD_DEINTERLACE_HALF;
#else
  return CProcessInfo::GetFallbackDeintMethod();
#endif
}
