/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#if defined(TARGET_DARWIN_IOS)
//hack around problem with xbmc's typedef int BOOL
// and obj-c's typedef unsigned char BOOL
#define BOOL XBMC_BOOL 
#include "system.h"
#undef BOOL

#ifdef HAS_EGL
#define BOOL XBMC_BOOL 
#include "WinSystemIOS.h"
#include "utils/log.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "guilib/Texture.h"
#include <vector>
#undef BOOL

#import <Foundation/Foundation.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#if defined(TARGET_DARWIN_IOS_ATV2)
#import "atv2/XBMCController.h"
#else
#import "ios/XBMCController.h"
#endif
#include "osx/DarwinUtils.h"
#import <dlfcn.h>

CWinSystemIOS::CWinSystemIOS() : CWinSystemBase()
{
  m_eWindowSystem = WINDOW_SYSTEM_IOS;

  m_iVSyncErrors = 0;
}

CWinSystemIOS::~CWinSystemIOS()
{
}

bool CWinSystemIOS::InitWindowSystem()
{
	return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemIOS::DestroyWindowSystem()
{
  return true;
}

bool CWinSystemIOS::CreateNewWindow(const CStdString& name, bool fullScreen, RESOLUTION_INFO& res, PHANDLE_EVENT_FUNC userFunction)
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
	
  if(!SetFullScreen(fullScreen, res, false))
    return false;

  [g_xbmcController setFramebuffer];

  m_bWindowCreated = true;

  m_eglext  = " ";
  m_eglext += (const char*) glGetString(GL_EXTENSIONS);
  m_eglext += " ";

  CLog::Log(LOGDEBUG, "EGL_EXTENSIONS:%s", m_eglext.c_str());
  return true;
}

bool CWinSystemIOS::DestroyWindow()
{
  return true;
}

bool CWinSystemIOS::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
	
  if (m_nWidth != newWidth || m_nHeight != newHeight)
  {
    m_nWidth  = newWidth;
    m_nHeight = newHeight;
  }

  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight, false, 0);

  return true;
}

bool CWinSystemIOS::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays)
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
	
  m_nWidth      = res.iWidth;
  m_nHeight     = res.iHeight;
  m_bFullScreen = fullScreen;

  CLog::Log(LOGDEBUG, "About to switch to %i x %i on screen %i",m_nWidth, m_nHeight, res.iScreen);
#ifndef TARGET_DARWIN_IOS_ATV2
  SwitchToVideoMode(res.iWidth, res.iHeight, res.fRefreshRate, res.iScreen);
#endif//TARGET_DARWIN_IOS_ATV2
  CRenderSystemGLES::ResetRenderSystem(res.iWidth, res.iHeight, fullScreen, res.fRefreshRate);
  
  return true;
}

UIScreenMode *getModeForResolution(int width, int height, int screenIdx)
{
  if( screenIdx >= [[UIScreen screens] count])
    return NULL;
    
  UIScreen *aScreen = [[UIScreen screens]objectAtIndex:screenIdx];
  for ( UIScreenMode *mode in [aScreen availableModes] )
  {
    //for main screen also find modes where width and height are
    //exchanged (because of the 90°degree rotated buildinscreens)
    if((mode.size.width == width && mode.size.height == height) || 
        (screenIdx == 0 && mode.size.width == height && mode.size.height == width))
    {
      CLog::Log(LOGDEBUG,"Found matching mode");
      return mode;
    }
  }
  CLog::Log(LOGERROR,"No matching mode found!");
  return NULL;
}

bool CWinSystemIOS::SwitchToVideoMode(int width, int height, double refreshrate, int screenIdx)
{
  bool ret = false;
  // SwitchToVideoMode will not return until the display has actually switched over.
  // This can take several seconds.
  if( screenIdx >= GetNumScreens())
    return false;
  
  //get the mode to pass to the controller
  UIScreenMode *newMode = getModeForResolution(width, height, screenIdx);

  if(newMode)
  {
    ret = [g_xbmcController changeScreen:screenIdx withMode:newMode];
  }
  return ret;
}

int CWinSystemIOS::GetNumScreens()
{
  return [[UIScreen screens] count];
}

bool CWinSystemIOS::GetScreenResolution(int* w, int* h, double* fps, int screenIdx)
{
  // Figure out the screen size. (default to main screen)
  if(screenIdx >= GetNumScreens())
    return false;
  UIScreen *screen = [[UIScreen screens] objectAtIndex:screenIdx];
  CGSize screenSize = [screen currentMode].size;
  *w = screenSize.width;
  *h = screenSize.height;
  *fps = 0.0;
  //if current mode is 0x0 (happens with external screens which arn't active)
  //then use the preferred mode
  if(*h == 0 || *w ==0)
  {
    UIScreenMode *firstMode = [screen preferredMode];
    *w = firstMode.size.width;
    *h = firstMode.size.height;
  }
  
  //for mainscreen use the eagl bounds
  //because mainscreen is build in
  //in 90° rotated
  if(screenIdx == 0)
  {
    *w = [g_xbmcController getScreenSize].width;
    *h = [g_xbmcController getScreenSize].height;
  }
  CLog::Log(LOGDEBUG,"Current resolution Screen: %i with %i x %i",screenIdx, *w, *h);  
  return true;
}

void CWinSystemIOS::UpdateResolutions()
{
  // Add display resolution
  int w, h;
  double fps;
  CWinSystemBase::UpdateResolutions();

  //first screen goes into the current desktop mode
  if(GetScreenResolution(&w, &h, &fps, 0))
  {
    UpdateDesktopResolution(g_settings.m_ResInfo[RES_DESKTOP], 0, w, h, fps);
  }

#ifndef TARGET_DARWIN_IOS_ATV2
  //see resolution.h enum RESOLUTION for how the resolutions
  //have to appear in the g_settings.m_ResInfo vector
  //add the desktop resolutions of the other screens
  for(int i = 1; i < GetNumScreens(); i++)
  {
    RESOLUTION_INFO res;      
    //get current resolution of screen i
    if(GetScreenResolution(&w, &h, &fps, i))
    {
      UpdateDesktopResolution(res, i, w, h, fps);
      g_settings.m_ResInfo.push_back(res);
    }
  }
  
  //now just fill in the possible reolutions for the attached screens
  //and push to the m_ResInfo vector
  FillInVideoModes();
#endif //TARGET_DARWIN_IOS_ATV2
}

void CWinSystemIOS::FillInVideoModes()
{
  // Add full screen settings for additional monitors
  int numDisplays = GetNumScreens();

  for (int disp = 0; disp < numDisplays; disp++)
  {
    RESOLUTION_INFO res;
    int w, h;
    // atm we don't get refreshrate info from iOS
    // but this may change in the future. In that case
    // we will adapt this code for filling some
    // usefull info into this local var :)
    double refreshrate = 0.0;
    //screen 0 is mainscreen - 1 has to be the external one...
    UIScreen *aScreen = [[UIScreen screens]objectAtIndex:disp];
    //found external screen
    for ( UIScreenMode *mode in [aScreen availableModes] )
    {
      w = mode.size.width;
      h = mode.size.height;
      UpdateDesktopResolution(res, disp, w, h, refreshrate);
      CLog::Log(LOGNOTICE, "Found possible resolution for display %d with %d x %d\n", disp, w, h);      

      //overwrite the mode str because  UpdateDesktopResolution adds a
      //"Full Screen". Since the current resolution is there twice
      //this would lead to 2 identical resolution entrys in the guisettings.xml.
      //That would cause problems with saving screen overscan calibration
      //because the wrong entry is picked on load.
      //So we just use UpdateDesktopResolutions for the current DESKTOP_RESOLUTIONS
      //in UpdateResolutions. And on all othere resolutions make a unique
      //mode str by doing it without appending "Full Screen".
      //this is what linux does - though it feels that there shouldn't be
      //the same resolution twice... - thats why i add a FIXME here.
      res.strMode.Format("%dx%d @ %.2f", w, h, refreshrate);
      g_graphicsContext.ResetOverscan(res);
      g_settings.m_ResInfo.push_back(res);
    }
  }
}

bool CWinSystemIOS::IsExtSupported(const char* extension)
{
  if(strncmp(extension, "EGL_", 4) != 0)
    return CRenderSystemGLES::IsExtSupported(extension);

  CStdString name;

  name  = " ";
  name += extension;
  name += " ";

  return m_eglext.find(name) != string::npos;
}

bool CWinSystemIOS::BeginRender()
{
  bool rtn;

  [g_xbmcController setFramebuffer];

  rtn = CRenderSystemGLES::BeginRender();
  return rtn;
}

bool CWinSystemIOS::EndRender()
{
  bool rtn;

  rtn = CRenderSystemGLES::EndRender();
  return rtn;
}

void CWinSystemIOS::InitDisplayLink(void)
{
}
void CWinSystemIOS::DeinitDisplayLink(void)
{
}
double CWinSystemIOS::GetDisplayLinkFPS(void)
{
  double fps;

  fps = [g_xbmcController getDisplayLinkFPS];
  return fps;
}

bool CWinSystemIOS::PresentRenderImpl(const CDirtyRegionList &dirty)
{
  //glFlush;
  [g_xbmcController presentFramebuffer];
  return true;
}

void CWinSystemIOS::SetVSyncImpl(bool enable)
{
  #if 0	
    // set swapinterval if possible
    void *eglSwapInterval;	
    eglSwapInterval = dlsym( RTLD_DEFAULT, "eglSwapInterval" );
    if ( eglSwapInterval )
    {
      ((void(*)(int))eglSwapInterval)( 1 ) ;
    }
  #endif
  m_iVSyncMode = 10;
}

void CWinSystemIOS::ShowOSMouse(bool show)
{
}

bool CWinSystemIOS::HasCursor()
{
  if( DarwinIsAppleTV2() )
  {
    return true;
  }
  else//apple touch devices
  {
    return false;
  }
}

void CWinSystemIOS::NotifyAppActiveChange(bool bActivated)
{
  if (bActivated && m_bWasFullScreenBeforeMinimize && !g_graphicsContext.IsFullScreenRoot())
    g_graphicsContext.ToggleFullScreenRoot();
}

bool CWinSystemIOS::Minimize()
{
  m_bWasFullScreenBeforeMinimize = g_graphicsContext.IsFullScreenRoot();
  if (m_bWasFullScreenBeforeMinimize)
    g_graphicsContext.ToggleFullScreenRoot();

  return true;
}

bool CWinSystemIOS::Restore()
{
  return false;
}

bool CWinSystemIOS::Hide()
{
  return true;
}

bool CWinSystemIOS::Show(bool raise)
{
  return true;
}
#endif

#endif
