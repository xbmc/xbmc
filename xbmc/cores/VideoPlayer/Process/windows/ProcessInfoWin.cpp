/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoWin.h"

#include "cores/VideoPlayer/Process/ProcessInfo.h"

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoWin::Create()
{
  return new CProcessInfoWin();
}

void CProcessInfoWin::Register()
{
  RegisterProcessControl("win", Create);
}

EINTERLACEMETHOD CProcessInfoWin::GetFallbackDeintMethod()
{
  return VS_INTERLACEMETHOD_AUTO;
}

std::vector<AVPixelFormat> CProcessInfoWin::GetRenderFormats()
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
