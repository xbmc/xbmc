/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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

#include "stdafx.h"

#ifdef __APPLE__

#include "WinSystemOSX.h"
#include "SpecialProtocol.h"
#include "CocoaInterface.h"
#include "Settings.h"
#include "Texture.h"


CWinSystemOSX::CWinSystemOSX() : CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_OSX;
  m_glContext = 0;
  m_SDLSurface = NULL;
}

CWinSystemOSX::~CWinSystemOSX()
{
  DestroyWindowSystem();
};

bool CWinSystemOSX::InitWindowSystem()
{
  SDL_EnableUNICODE(1);

  // set repeat to 10ms to ensure repeat time < frame time
  // so that hold times can be reliably detected
  SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, 10);
  
  if (!CWinSystemBase::InitWindowSystem())
    return false;
  
  return true;
}

bool CWinSystemOSX::DestroyWindowSystem()
{  
  return true;
}

bool CWinSystemOSX::CreateNewWindow(const CStdString& name, int width, int height, bool fullScreen, PHANDLE_EVENT_FUNC userFunction)
{
  m_nWidth = width;
  m_nHeight = height;
  m_bFullScreen = fullScreen;
    
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  
  // Enable vertical sync to avoid any tearing.
  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);  
  
  m_SDLSurface = SDL_SetVideoMode(m_nWidth, m_nHeight, 0, SDL_OPENGL | (m_bFullScreen ? 0 : SDL_RESIZABLE));
  if (!m_SDLSurface)
  {
    return false;
  }
  
  // the context SDL creates isn't full screen compatible, so we create new one
  m_glContext = Cocoa_GL_ReplaceSDLWindowContext();  
  Cocoa_GL_MakeCurrentContext(m_glContext);

  CTexture iconTexture;
  iconTexture.LoadFromFile("special://xbmc/media/icon.png");
  SDL_WM_SetIcon(SDL_CreateRGBSurfaceFrom(iconTexture.GetPixels(), iconTexture.GetWidth(), iconTexture.GetHeight(), iconTexture.GetBPP(), iconTexture.GetPitch(), 0xff0000, 0x00ff00, 0x0000ff, 0xff000000L), NULL);
    
  SDL_WM_SetCaption("XBMC Media Center", NULL);
    
  m_bWindowCreated = true;

  return true;
}

bool CWinSystemOSX::DestroyWindow()
{
  return true;
}
    
bool CWinSystemOSX::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  m_nWidth = newWidth;
  m_nHeight = newHeight;
  
  m_glContext = Cocoa_GL_ResizeWindow(m_glContext, newWidth, newHeight);
  Cocoa_GL_MakeCurrentContext(m_glContext);
  
  return true;
}

bool CWinSystemOSX::SetFullScreen(bool fullScreen, int screen, int width, int height, bool blankOtherDisplays, bool alwaysOnTop)
{  
  m_nWidth = width;
  m_nHeight = height;
  m_bFullScreen = fullScreen;
  
  Cocoa_GL_SetFullScreen(screen, width, height, fullScreen, blankOtherDisplays, g_advancedSettings.m_osx_GLFullScreen);
  m_glContext = Cocoa_GL_GetCurrentContext();
  Cocoa_GL_MakeCurrentContext(m_glContext);
  
  return true;
}

void CWinSystemOSX::UpdateResolutions()
{
  CWinSystemBase::UpdateResolutions();
  
  // Add desktop resolution
  int w, h;
  Cocoa_GetScreenResolution(&w, &h);
  UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, w, h, Cocoa_GetScreenRefreshRate(0));
  
  // Add full screen settings for additional monitors
  int numDisplays = Cocoa_GetNumDisplays();
  for (int i = 1; i < numDisplays; i++)
  {
     Cocoa_GetScreenResolutionOfAnotherScreen(i, &w, &h);
     CLog::Log(LOGINFO, "Extra display %d is %dx%d\n", i, w, h);

     RESOLUTION_INFO res;

     UpdateDesktopResolution(res, i, w, h, Cocoa_GetScreenRefreshRate(i));
     g_graphicsContext.ResetOverscan(res);
     g_settings.m_ResInfo.push_back(res);
  }  
}

#endif
