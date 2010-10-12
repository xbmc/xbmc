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
#include "system.h"
#include "WindowingFactory.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "GraphicContext.h"
#include "MouseStat.h"
#include "Application.h"
#include "GUILargeTextureManager.h"
#include "TextureManager.h"
#include "AudioContext.h"
#include "GUISettings.h"

  CGUISettings g_guiSettings;

#if defined(_WIN32) && defined(HAS_GL)
  CWinSystemWin32GL  g_Windowing;
#endif

#if defined(_WIN32) && defined(HAS_DX)
  CWinSystemWin32DX  g_Windowing;
#endif

#if defined(__APPLE__)
  CWinSystemOSXGL    g_Windowing;
#endif

#if defined(HAS_GLX)
  CWinSystemX11GL    g_Windowing;
#endif

  CXBMCRenderManager g_renderManager;
  CAudioContext      g_audioContext;
  CGraphicContext    g_graphicsContext;
  CGUITextureManager g_TextureManager;
  CGUILargeTextureManager g_largeTextureManager;
  CMouseStat         g_Mouse;
  CApplication       g_application;
