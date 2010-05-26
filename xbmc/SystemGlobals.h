#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "WindowingFactory.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "GraphicContext.h"
#include "MouseStat.h"
#include "Application.h"
#include "GUILargeTextureManager.h"
#include "AudioContext.h"
#include "GUISettings.h"

struct CSystemGlobals
{
  // Classes that must be initialized/destructed in a specific order because of dependencies.

  CGUISettings m_guiSettings;

#if defined(_WIN32) && defined(HAS_GL)
  CWinSystemWin32GL  m_Windowing;
#endif

#if defined(_WIN32) && defined(HAS_DX)
  CWinSystemWin32DX  m_Windowing;
#endif

#if defined(__APPLE__)
  CWinSystemOSXGL    m_Windowing;
#endif

#if defined(HAS_GLX)
  CWinSystemX11GL    m_Windowing;
#endif

  CGUILargeTextureManager m_largeTextureManager;
  CXBMCRenderManager m_renderManager;
  CAudioContext      m_audioContext;
  CGraphicContext    m_graphicsContext;
  CMouseStat         m_Mouse;
  CApplication       m_application;
};

extern CSystemGlobals g_SystemGlobals;
