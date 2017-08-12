/*
 *      Copyright (C) 2017 Team XBMC
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
#pragma once

#include "rendering/gl/RenderSystemGL.h"
#include "utils/GlobalsHandling.h"
#include "WinSystemWaylandEGLContext.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CWinSystemWaylandEGLContextGL : public CWinSystemWaylandEGLContext, public CRenderSystemGL
{
public:
  bool InitWindowSystem() override;
protected:
  void SetContextSize(CSizeInt size) override;
  void SetVSyncImpl(bool enable) override;
  void PresentRenderImpl(bool rendered) override;
};

}
}
}

XBMC_GLOBAL_REF(KODI::WINDOWING::WAYLAND::CWinSystemWaylandEGLContextGL, g_Windowing);
#define g_Windowing XBMC_GLOBAL_USE(KODI::WINDOWING::WAYLAND::CWinSystemWaylandEGLContextGL)