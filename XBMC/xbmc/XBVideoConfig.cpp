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
#include "include.h"
#include "Settings.h"
#include "XBVideoConfig.h"
#ifdef HAS_GLX
#include "Surface.h"
using namespace Surface;
#include <X11/extensions/Xinerama.h>
#endif
#ifdef HAS_XRANDR
#include "XRandR.h"
#endif

#ifdef __APPLE__
#include "CocoaUtils.h"
#endif

using namespace std;

XBVideoConfig g_videoConfig;

XBVideoConfig::XBVideoConfig()
{
  bHasPAL = false;
  bHasNTSC = false;
  m_dwVideoFlags = 0;
  m_VSyncMode = VSYNC_DISABLED;
}

XBVideoConfig::~XBVideoConfig()
{}

bool XBVideoConfig::HasPAL() const
{
  return true;
}

bool XBVideoConfig::HasPAL60() const
{
  return false; // no need for pal60 mode IMO
}

bool XBVideoConfig::HasNTSC() const
{
  return true;
}

bool XBVideoConfig::HasWidescreen() const
{
  return true;
}

bool XBVideoConfig::HasLetterbox() const
{
  return true;
}

bool XBVideoConfig::Has480p() const
{
  return true;
}

bool XBVideoConfig::Has720p() const
{
  return true;
}

bool XBVideoConfig::Has1080i() const
{
  return true;
}

bool XBVideoConfig::HasHDPack() const
{
  return true;
}

#ifdef HAS_SDL
void XBVideoConfig::GetDesktopResolution(int &w, int &h) const
{
#ifdef __APPLE__
  Cocoa_GetScreenResolution(&w, &h);
#elif defined(_WIN32PC)
    DEVMODE devmode;
    ZeroMemory(&devmode, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    w = devmode.dmPelsWidth;
    h = devmode.dmPelsHeight;
#endif
  
#ifdef HAS_GLX
  Display * pRootDisplay = XOpenDisplay(NULL);
  if (pRootDisplay == NULL)
  {
     fprintf(stderr, "Cannot get root display. Is X11 running and is your DISPLAY variable set?\n");
     exit(1);
  }

  int screen = DefaultScreen(pRootDisplay);
  int width = DisplayWidth(pRootDisplay, screen);
  int height = DisplayHeight(pRootDisplay, screen);
  XineramaScreenInfo *info;
  int num;
  info = XineramaQueryScreens(pRootDisplay, &num);
  if (info)
  {
    int desired = 0;
    width = info[0].width;
    height = info[0].height;
    const char *variable = SDL_getenv("SDL_VIDEO_FULLSCREEN_HEAD");
    if (variable)
    {
      desired = SDL_atoi(variable);
      for (int i = 0 ; i<num ; i++)
      {
        if (info[i].screen_number==desired)
        {
          width = info[i].width;
          height = info[i].height;
          break;
        }
      }
    }
    XFree(info);
  }
  w = width;
  h = height;
#endif
}
#endif

#ifndef HAS_SDL
void XBVideoConfig::GetModes(LPDIRECT3D8 pD3D)
{
  bHasPAL = false;
  bHasNTSC = false;
  DWORD numModes = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
  D3DDISPLAYMODE mode;
  CLog::Log(LOGINFO, "Available videomodes:");
  for ( DWORD i = 0; i < numModes; i++ )
  {
    pD3D->EnumAdapterModes( 0, i, &mode );

    // Skip modes we don't care about
    if ( mode.Format != D3DFMT_LIN_A8R8G8B8 )
      continue;
    // ignore 640 wide modes
    if ( mode.Width < 720)
      continue;

    // If we get here, we found an acceptable mode
    CLog::Log(LOGINFO, "Found mode: %ix%i at %iHz", mode.Width, mode.Height, mode.RefreshRate);
    if (mode.Width == 720 && mode.Height == 576 && mode.RefreshRate == 50)
      bHasPAL = true;
    if (mode.Width == 720 && mode.Height == 480 && mode.RefreshRate == 60)
      bHasNTSC = true;
  }
}

#elif defined(HAS_XRANDR)

void XBVideoConfig::GetModes()
{
  CLog::Log(LOGINFO, "Available videomodes (xrandr):");
  vector<XOutput>::iterator outiter;
  vector<XOutput> outs;
  outs = g_xrandr.GetModes();
  CLog::Log(LOGINFO, "Number of connected outputs: %d", outs.size());
  string modename = "";

  m_iNumResolutions = 0;

  for (outiter = outs.begin() ; outiter != outs.end() ; outiter++)
  {
    XOutput out = *outiter;
    vector<XMode>::iterator modeiter;
    CLog::Log(LOGINFO, "Output '%s' has %d modes", out.name.c_str(), out.modes.size());

    for (modeiter = out.modes.begin() ; modeiter!=out.modes.end() ; modeiter++)
    {
      XMode mode = *modeiter;
      CLog::Log(LOGINFO, "ID:%s Name:%s Refresh:%f Width:%d Height:%d",
                mode.id.c_str(), mode.name.c_str(), mode.hz, mode.w, mode.h);
      if (m_iNumResolutions<MAX_RESOLUTIONS)
      {
        m_ResInfo[m_iNumResolutions].iWidth = mode.w;
        m_ResInfo[m_iNumResolutions].iHeight = mode.h;
        if (mode.h>0 && mode.w>0 && out.hmm>0 && out.wmm>0)
        {
          m_ResInfo[m_iNumResolutions].fPixelRatio = 
            ((float)out.wmm/(float)mode.w) / (((float)out.hmm/(float)mode.h));
        }
        else
        {
          m_ResInfo[m_iNumResolutions].fPixelRatio = 1.0f;
        }
        CLog::Log(LOGINFO, "Pixel Ratio: %f", m_ResInfo[m_iNumResolutions].fPixelRatio);
        m_ResInfo[m_iNumResolutions].iSubtitles = (int)(0.9*mode.h);
        m_ResInfo[m_iNumResolutions].fRefreshRate = mode.hz;
        snprintf(m_ResInfo[m_iNumResolutions].strMode,
                 sizeof(m_ResInfo[m_iNumResolutions].strMode),
                 "%s: %s @ %.2fHz",
                 out.name.c_str(), mode.name.c_str(), mode.hz);
        snprintf(m_ResInfo[m_iNumResolutions].strOutput,
                 sizeof(m_ResInfo[m_iNumResolutions].strOutput),
                 "%s", out.name.c_str());
        snprintf(m_ResInfo[m_iNumResolutions].strId,
                 sizeof(m_ResInfo[m_iNumResolutions].strId),
                 "%s", mode.id.c_str());
        if ((float)mode.w / (float)mode.h >= 1.59)
          m_ResInfo[m_iNumResolutions].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
        else
          m_ResInfo[m_iNumResolutions].dwFlags = 0;
        g_graphicsContext.ResetOverscan(m_ResInfo[m_iNumResolutions]);
        g_settings.m_ResInfo[CUSTOM+m_iNumResolutions] = m_ResInfo[m_iNumResolutions];
        m_iNumResolutions++;
        if (mode.w == 720 && mode.h == 576)
          bHasPAL = true;
        if (mode.w == 720 && mode.h == 480)
          bHasNTSC = true;
      }
    }
  }
  g_graphicsContext.ResetScreenParameters(DESKTOP);
  g_graphicsContext.ResetScreenParameters(WINDOW);
}

#else

void XBVideoConfig::GetModes()
{
   CLog::Log(LOGINFO, "Available videomodes:");
   SDL_Rect        **modes;

   /* Get available fullscreen/hardware modes */
#ifdef HAS_SDL_OPENGL
   modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
#else
   modes = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);
#endif
   m_iNumResolutions = 0;
   if (modes == (SDL_Rect **)0)
   {
     CLog::Log(LOGINFO, "No modes available");
     return;
   }

   if (modes == (SDL_Rect **)-1)
   {
     bHasPAL = true;
     bHasNTSC = true;
     CLog::Log(LOGINFO, "All resolutions available");
     return;
   }

   for (int i = 0; modes[i]; ++i)
   {
     // ignore 640 wide modes
     if (modes[i]->w < 720)
       continue;
     if (i<MAX_RESOLUTIONS)
     {
       m_ResInfo[m_iNumResolutions].iScreen = 0;
       m_ResInfo[m_iNumResolutions].iWidth = modes[i]->w;
       m_ResInfo[m_iNumResolutions].iHeight = modes[i]->h;
       m_ResInfo[m_iNumResolutions].fPixelRatio = 1.0f;
       m_ResInfo[m_iNumResolutions].iSubtitles = (int)(0.9*modes[i]->h);
       snprintf(m_ResInfo[m_iNumResolutions].strMode,
                sizeof(m_ResInfo[m_iNumResolutions].strMode),
                "%d x %d", modes[i]->w, modes[i]->h);
       if ((float)modes[i]->w / (float)modes[i]->h >= 1.59)
         m_ResInfo[m_iNumResolutions].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
       else
         m_ResInfo[m_iNumResolutions].dwFlags = 0;
       g_graphicsContext.ResetOverscan(m_ResInfo[m_iNumResolutions]);
       CLog::Log(LOGINFO, "Found mode: %ix%i", modes[i]->w, modes[i]->h);
       g_settings.m_ResInfo[CUSTOM+m_iNumResolutions] = m_ResInfo[m_iNumResolutions];
       m_iNumResolutions++;
       if (modes[i]->w == 720 && modes[i]->h == 576)
         bHasPAL = true;
       if (modes[i]->w == 720 && modes[i]->h == 480)
         bHasNTSC = true;
     }
   }
   
#ifdef __APPLE__
   // Add other fullscreen settings for other monitors.
   int numDisplays = Cocoa_GetNumDisplays();
   for (int i=1; i<numDisplays; i++)
   {
     int w, h;
     Cocoa_GetScreenResolutionOfAnotherScreen(i, &w, &h);
     CLog::Log(LOGINFO, "Extra display %d is %dx%d\n", i, w, h);
     
     m_ResInfo[m_iNumResolutions].iScreen = i;
     m_ResInfo[m_iNumResolutions].iWidth = w;
     m_ResInfo[m_iNumResolutions].iHeight = h;
     m_ResInfo[m_iNumResolutions].fPixelRatio = 1.0f;
     m_ResInfo[m_iNumResolutions].iSubtitles = (int)(0.9*h);
     snprintf(m_ResInfo[m_iNumResolutions].strMode,
               sizeof(m_ResInfo[m_iNumResolutions].strMode),
               "%d x %d (Full screen #%d)", w, h, i+1);
     if ((float)w / (float)h >= 1.59)
       m_ResInfo[m_iNumResolutions].dwFlags = D3DPRESENTFLAG_WIDESCREEN;
     else
       m_ResInfo[m_iNumResolutions].dwFlags = 0;
     g_graphicsContext.ResetOverscan(m_ResInfo[m_iNumResolutions]);
     g_settings.m_ResInfo[CUSTOM+m_iNumResolutions] = m_ResInfo[m_iNumResolutions];
     m_iNumResolutions++;
     if (w == 720 && h == 576)
       bHasPAL = true;
     if (w == 720 && h == 480)
       bHasNTSC = true;
   }
#endif
   
   g_graphicsContext.ResetScreenParameters(DESKTOP);
   g_graphicsContext.ResetScreenParameters(WINDOW);
}
#endif

RESOLUTION XBVideoConfig::GetSafeMode() const
{
  // Get the desktop resolution to see what we're dealing with here.
  int w, h;
  GetDesktopResolution(w, h);
  
  // If we've got quite a few pixels, go with 720p.
  if (w > 1280 && h > 720)
    return HDTV_720p;
  
  // A few less pixels, but still enough for 480p.
  if (w > 854 && h > 420)
    return HDTV_480p_16x9;
  
  if (HasPAL()) 
    return PAL_4x3;
  
  return NTSC_4x3;
}

RESOLUTION XBVideoConfig::GetBestMode() const
{
  RESOLUTION bestRes = INVALID;
  RESOLUTION resolutions[] = {HDTV_1080i, HDTV_720p, HDTV_480p_16x9, HDTV_480p_4x3, NTSC_16x9, NTSC_4x3, PAL_16x9, PAL_4x3, PAL60_16x9, PAL60_4x3, INVALID};
  UCHAR i = 0;
  while (resolutions[i] != INVALID)
  {
    if (IsValidResolution(resolutions[i]))
    {
      bestRes = resolutions[i];
      break;
    }
    i++;
  }
  return bestRes;
}

bool XBVideoConfig::IsValidResolution(RESOLUTION res) const
{
  bool bCanDoWidescreen = HasWidescreen();
  if (HasPAL())
  {
    bool bCanDoPAL60 = HasPAL60();
    if (res == PAL_4x3) return true;
    if (res == PAL_16x9 && bCanDoWidescreen) return true;
    if (res == PAL60_4x3 && bCanDoPAL60) return true;
    if (res == PAL60_16x9 && bCanDoPAL60 && bCanDoWidescreen) return true;
  }
  if (HasNTSC())
  {
    if (res == NTSC_4x3) return true;
    if (res == NTSC_16x9 && bCanDoWidescreen) return true;
    if (res == HDTV_480p_4x3 && Has480p()) return true;
    if (res == HDTV_480p_16x9 && Has480p() && bCanDoWidescreen) return true;
    if (res == HDTV_720p && Has720p()) return true;
    if (res == HDTV_1080i && Has1080i()) return true;
  }
  if (res>=WINDOW && res<(CUSTOM+m_iNumResolutions))
  {
    return true;
  }
  return false;
}

#ifndef HAS_SDL
//pre: XBVideoConfig::GetModes has been called before this function
RESOLUTION XBVideoConfig::GetInitialMode(LPDIRECT3D8 pD3D, D3DPRESENT_PARAMETERS *p3dParams)
{
  bool bHasPal = HasPAL();
  DWORD numModes = pD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
  D3DDISPLAYMODE mode;
  for ( DWORD i = 0; i < numModes; i++ )
  {
    pD3D->EnumAdapterModes( 0, i, &mode );

    // ignore 640 wide modes
    if ( mode.Width < 720)
      continue;

    p3dParams->BackBufferWidth = mode.Width;
    p3dParams->BackBufferHeight = mode.Height;
    p3dParams->FullScreen_RefreshRateInHz = mode.RefreshRate;
    if ((bHasPal) && ((mode.Height != 576) || (mode.RefreshRate != 50)))
    {
      continue;
    }
    //take the first available mode
  }

  if (HasPAL())
  {
    if (HasWidescreen() && (p3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN))
    {
      return PAL_16x9;
    }
    else
    {
      return PAL_4x3;
    }
  }
  if (HasWidescreen() && (p3dParams->Flags & D3DPRESENTFLAG_WIDESCREEN))
  {
    return NTSC_16x9;
  }
  else
  {
    return NTSC_4x3;
  }
}
#else
RESOLUTION XBVideoConfig::GetInitialMode()
{
  // TODO LINUX -- fix!!!
  return PAL_4x3;
}
#endif

CStdString XBVideoConfig::GetAVPack() const
{
  return "Unknown";
}

void XBVideoConfig::PrintInfo() const
{
}

bool XBVideoConfig::NeedsSave()
{
  return false;
}

void XBVideoConfig::Set480p(bool bEnable)
{
}

void XBVideoConfig::Set720p(bool bEnable)
{
}

void XBVideoConfig::Set1080i(bool bEnable)
{
}

void XBVideoConfig::SetNormal()
{
}

void XBVideoConfig::SetLetterbox(bool bEnable)
{
}

void XBVideoConfig::SetWidescreen(bool bEnable)
{
}

void XBVideoConfig::Save()
{
  if (!NeedsSave()) return;
}

