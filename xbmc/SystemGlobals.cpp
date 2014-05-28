/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#ifndef ROOT_SYSTEM_H_INCLUDED
#define ROOT_SYSTEM_H_INCLUDED
#include "system.h"
#endif

#ifndef ROOT_CORES_VIDEORENDERERS_RENDERMANAGER_H_INCLUDED
#define ROOT_CORES_VIDEORENDERERS_RENDERMANAGER_H_INCLUDED
#include "cores/VideoRenderers/RenderManager.h"
#endif

#ifndef ROOT_INPUT_MOUSESTAT_H_INCLUDED
#define ROOT_INPUT_MOUSESTAT_H_INCLUDED
#include "input/MouseStat.h"
#endif

#ifndef ROOT_APPLICATION_H_INCLUDED
#define ROOT_APPLICATION_H_INCLUDED
#include "Application.h"
#endif

#ifndef ROOT_GUILARGETEXTUREMANAGER_H_INCLUDED
#define ROOT_GUILARGETEXTUREMANAGER_H_INCLUDED
#include "GUILargeTextureManager.h"
#endif

#ifndef ROOT_GUILIB_TEXTUREMANAGER_H_INCLUDED
#define ROOT_GUILIB_TEXTUREMANAGER_H_INCLUDED
#include "guilib/TextureManager.h"
#endif

#ifndef ROOT_UTILS_ALARMCLOCK_H_INCLUDED
#define ROOT_UTILS_ALARMCLOCK_H_INCLUDED
#include "utils/AlarmClock.h"
#endif

#ifndef ROOT_GUIINFOMANAGER_H_INCLUDED
#define ROOT_GUIINFOMANAGER_H_INCLUDED
#include "GUIInfoManager.h"
#endif

#ifndef ROOT_FILESYSTEM_DLLLIBCURL_H_INCLUDED
#define ROOT_FILESYSTEM_DLLLIBCURL_H_INCLUDED
#include "filesystem/DllLibCurl.h"
#endif

#ifndef ROOT_FILESYSTEM_DIRECTORYCACHE_H_INCLUDED
#define ROOT_FILESYSTEM_DIRECTORYCACHE_H_INCLUDED
#include "filesystem/DirectoryCache.h"
#endif

#ifndef ROOT_GUIPASSWORD_H_INCLUDED
#define ROOT_GUIPASSWORD_H_INCLUDED
#include "GUIPassword.h"
#endif

#ifndef ROOT_LANGINFO_H_INCLUDED
#define ROOT_LANGINFO_H_INCLUDED
#include "LangInfo.h"
#endif

#ifndef ROOT_UTILS_LANGCODEEXPANDER_H_INCLUDED
#define ROOT_UTILS_LANGCODEEXPANDER_H_INCLUDED
#include "utils/LangCodeExpander.h"
#endif

#ifndef ROOT_PARTYMODEMANAGER_H_INCLUDED
#define ROOT_PARTYMODEMANAGER_H_INCLUDED
#include "PartyModeManager.h"
#endif

#ifndef ROOT_PLAYLISTPLAYER_H_INCLUDED
#define ROOT_PLAYLISTPLAYER_H_INCLUDED
#include "PlayListPlayer.h"
#endif

#ifndef ROOT_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#define ROOT_GUILIB_LOCALIZESTRINGS_H_INCLUDED
#include "guilib/LocalizeStrings.h"
#endif

#ifndef ROOT_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#define ROOT_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#include "guilib/GUIWindowManager.h"
#endif

#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#if defined(TARGET_WINDOWS)
#include "input/windows/WINJoystick.h"
#elif defined(HAS_SDL_JOYSTICK) 
#include "input/SDLJoystick.h"
#endif

#if defined(HAS_FILESYSTEM_RAR)
#include "filesystem/RarManager.h"
#endif
#ifndef ROOT_FILESYSTEM_ZIPMANAGER_H_INCLUDED
#define ROOT_FILESYSTEM_ZIPMANAGER_H_INCLUDED
#include "filesystem/ZipManager.h"
#endif


#ifdef TARGET_RASPBERRY_PI
#include "linux/RBP.h"
#endif

  CXBMCRenderManager g_renderManager;
  CLangInfo          g_langInfo;
  CLangCodeExpander  g_LangCodeExpander;
  CLocalizeStrings   g_localizeStrings;
  CLocalizeStrings   g_localizeStringsTemp;

  XFILE::CDirectoryCache g_directoryCache;

  CGUITextureManager g_TextureManager;
  CGUILargeTextureManager g_largeTextureManager;
  CMouseStat         g_Mouse;
#if defined(HAS_SDL_JOYSTICK) 
  CJoystick          g_Joystick; 
#endif
  CGUIPassword       g_passwordManager;
  CGUIInfoManager    g_infoManager;

  XCURL::DllLibCurlGlobal g_curlInterface;
  CPartyModeManager     g_partyModeManager;

#ifdef HAS_PYTHON
  XBPython           g_pythonParser;
#endif
  CAlarmClock        g_alarmClock;
  PLAYLIST::CPlayListPlayer g_playlistPlayer;

#ifdef TARGET_RASPBERRY_PI
  CRBP               g_RBP;
#endif

#ifdef HAS_FILESYSTEM_RAR
  CRarManager g_RarManager;
#endif
  CZipManager g_ZipManager;

