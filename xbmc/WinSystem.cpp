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

#include "WinSystem.h"
#include "GraphicContext.h"
#include "Settings.h"

CWinSystemBase::CWinSystemBase()
{
  m_nWidth = 0;
  m_nHeight = 0;
  m_nTop = 0;
  m_nLeft = 0;
  m_bWindowCreated = false;
  m_bFullScreen = false;
  m_nScreen = 0;
  m_bBlankOtherDisplay = false;
}

CWinSystemBase::~CWinSystemBase()
{

}

bool CWinSystemBase::InitWindowSystem()
{
  UpdateResolutions();

  return true;
}

void CWinSystemBase::UpdateDesktopResolution(RESOLUTION_INFO& newRes, int screen, int width, int height, float refreshRate, uint32_t dwFlags)
{
  newRes.Overscan.left = 0;
  newRes.Overscan.top = 0;
  newRes.Overscan.right = width;
  newRes.Overscan.bottom = height;
  newRes.iScreen = screen;
  newRes.bFullScreen = true;
  newRes.iSubtitles = (int)(0.965 * height);
  newRes.dwFlags = dwFlags;
  newRes.fRefreshRate = refreshRate;
  newRes.fPixelRatio = 1.0f;
  newRes.iWidth = width;
  newRes.iHeight = height;
  newRes.strMode.Format("%dx%d", width, height);
  if (refreshRate > 1)
    newRes.strMode.Format("%s @ %.2f%s - Full Screen", newRes.strMode, refreshRate, dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  if (screen > 0)
    newRes.strMode.Format("%s #%d", newRes.strMode, screen + 1);
}

void CWinSystemBase::UpdateResolutions()
{
  // add the window res - defaults are fine.
  RESOLUTION_INFO& window = g_settings.m_ResInfo[RES_WINDOW];
  if (window.iWidth == 0)
    window.iWidth = 720;
  if (window.iHeight == 0)
    window.iHeight = 480;
  if (window.iSubtitles == 0)
    window.iSubtitles = (int)(0.965 * window.iHeight);
  window.fPixelRatio = 1.0f;
  window.strMode = "Windowed";
}

void CWinSystemBase::SetWindowResolution(int width, int height)
{
  RESOLUTION_INFO& window = g_settings.m_ResInfo[RES_WINDOW];
  window.iWidth = width;
  window.iHeight = height;
  window.iSubtitles = (int)(0.965 * window.iHeight);
  g_graphicsContext.ResetOverscan(window);
}

int CWinSystemBase::DesktopResolution(int screen)
{
  for (int idx = 0; idx < GetNumScreens(); idx++)
    if (g_settings.m_ResInfo[RES_DESKTOP + idx].iScreen == screen)
      return RES_DESKTOP + idx;
  // Uh? something's wrong, fallback to default res of main screen
  return RES_DESKTOP;
}

static void AddResolution(vector<RESOLUTION_WHR> &resolutions, int width, int height, int screenDefaultRes)
{
  for (unsigned int idx = 0; idx < resolutions.size(); idx++)
    if (resolutions[idx].width == width && resolutions[idx].height == height)
      return; // this width*height was taken care of.

  int screen = g_settings.m_ResInfo[screenDefaultRes].iScreen;
  float targetfps = g_settings.m_ResInfo[screenDefaultRes].fRefreshRate;
  //TODO: get it from another source if we can have stupid values like 0, 1?

  int bestmatch = RES_DESKTOP;
  float bestfitness = -1.0f;

  for (size_t i = (int)RES_DESKTOP; i < g_settings.m_ResInfo.size(); i++)
  {
    RESOLUTION_INFO &info = g_settings.m_ResInfo[i];

    if (info.iWidth  != width
    ||  info.iHeight != height
    ||  info.iScreen != screen)
      continue;

    float fitness = fabs(targetfps - info.fRefreshRate);

    if (bestfitness <0 || fitness <  bestfitness)
    {
      bestfitness = fitness;
      bestmatch = i;
      if (bestfitness == 0.0f) // perfect match
        break;
    }
  }

  RESOLUTION_WHR res = {g_settings.m_ResInfo[bestmatch].iWidth, g_settings.m_ResInfo[bestmatch].iHeight, bestmatch};
  resolutions.push_back(res);
}

static bool resSortPredicate (RESOLUTION_WHR i, RESOLUTION_WHR j)
{
  return  (i.width < j.width ||
          (i.width == j.width && i.height < j.height));
}

vector<RESOLUTION_WHR> CWinSystemBase::ScreenResolutions(int screen)
{
    int defResolution = DesktopResolution(screen);
    vector<RESOLUTION_WHR> resolutions;

    // Can't assume the resolutions are sorted in any way
    for (unsigned int idx = RES_DESKTOP; idx < g_settings.m_ResInfo.size(); idx++)
      if (g_settings.m_ResInfo[idx].iScreen == screen)
        AddResolution(resolutions, g_settings.m_ResInfo[idx].iWidth, g_settings.m_ResInfo[idx].iHeight, defResolution);

    sort(resolutions.begin(), resolutions.end(), resSortPredicate);

    return resolutions;
}
