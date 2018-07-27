/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoWin.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "rendering/dx/RenderContext.h"
#include <set>

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoWin::Create()
{
  return new CProcessInfoWin();
}

void CProcessInfoWin::Register()
{
  CProcessInfo::RegisterProcessControl("win", CProcessInfoWin::Create);
}

EINTERLACEMETHOD CProcessInfoWin::GetFallbackDeintMethod()
{
  return VS_INTERLACEMETHOD_AUTO;
}

std::vector<AVPixelFormat> CProcessInfoWin::GetRenderFormats()
{
  auto processor = DX::Windowing()->m_processorFormats;
  auto shaders = DX::Windowing()->m_shaderFormats;

  std::set<AVPixelFormat> formats;
  formats.insert(processor.begin(), processor.end());
  formats.insert(shaders.begin(), shaders.end());

  return std::vector<AVPixelFormat>(formats.begin(), formats.end());
}
