/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoWin10.h"

#include "cores/VideoPlayer/Process/ProcessInfo.h"

#include <set>

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoWin10::Create()
{
  return new CProcessInfoWin10();
}

void CProcessInfoWin10::Register()
{
  CProcessInfo::RegisterProcessControl("win10", CProcessInfoWin10::Create);
}

EINTERLACEMETHOD CProcessInfoWin10::GetFallbackDeintMethod()
{
  return EINTERLACEMETHOD::VS_INTERLACEMETHOD_AUTO;
}

std::vector<AVPixelFormat> CProcessInfoWin10::GetRenderFormats()
{
  return {
    AV_PIX_FMT_D3D11VA_VLD,
    AV_PIX_FMT_NV12,
    AV_PIX_FMT_YUV420P,
    AV_PIX_FMT_P010,
    AV_PIX_FMT_YUV420P10,
    AV_PIX_FMT_P016,
    AV_PIX_FMT_YUV420P16
  };
}
