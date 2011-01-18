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
#include "GUISettings.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "utils/CharsetConverter.h"
#include "utils/AlarmClock.h"
#include "utils/DownloadQueueManager.h"
#include "utils/GUIInfoManager.h"
#include "FileSystem/DllLibCurl.h"
#include "FileSystem/DirectoryCache.h"
#include "GUIPassword.h"
#include "LangInfo.h"
#include "LangCodeExpander.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIWindowManager.h"
#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif

  CGUISettings       g_guiSettings;
  CAdvancedSettings  g_advancedSettings;
  CSettings          g_settings;

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
  CCharsetConverter  g_charsetConverter;
  CLangInfo          g_langInfo;
  CLangCodeExpander  g_LangCodeExpander;
  CLocalizeStrings   g_localizeStrings;
  CLocalizeStrings   g_localizeStringsTemp;

  CGraphicContext    g_graphicsContext;
  CGUIWindowManager  g_windowManager;
  XFILE::CDirectoryCache g_directoryCache;

  CGUITextureManager g_TextureManager;
  CGUILargeTextureManager g_largeTextureManager;
  CMouseStat         g_Mouse;
  CGUIPassword       g_passwordManager;
  CGUIInfoManager    g_infoManager;

  XCURL::DllLibCurlGlobal g_curlInterface;
  CDownloadQueueManager g_DownloadManager;
  CPartyModeManager     g_partyModeManager;

#ifdef HAS_PYTHON
  XBPython           g_pythonParser;
#endif
  CAlarmClock        g_alarmClock;
  PLAYLIST::CPlayListPlayer g_playlistPlayer;
  CApplication       g_application;
