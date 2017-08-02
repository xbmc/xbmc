/*
 *      Copyright (C) 2005-2016 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ProcessInfoWin.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "windowing/WindowingFactory.h"
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
  auto processor = g_Windowing.m_processorFormats;
  auto shaders = g_Windowing.m_shaderFormats;

  std::set<AVPixelFormat> formats;
  formats.insert(processor.begin(), processor.end());
  formats.insert(shaders.begin(), shaders.end());

  return std::vector<AVPixelFormat>(formats.begin(), formats.end());
}
