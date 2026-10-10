/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoWin.h"

#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoPlayer/VideoRenderers/windows/RendererShaders.h"

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
  return CRendererShaders::GetRenderFormats();
}
