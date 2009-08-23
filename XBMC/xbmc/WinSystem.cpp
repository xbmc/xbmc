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
#include "WinSystem.h"


CWinSystemBase::CWinSystemBase()
{
  m_nWidth = 0;
  m_nHeight = 0;
  m_nTop = 0;
  m_nLeft = 0;
  m_bFullScreen = false;
  m_bWindowCreated = false;
  ZeroMemory(&m_DesktopRes, sizeof(RESOLUTION_INFO));
  m_nCurrentResolution = INVALID;
}

CWinSystemBase::~CWinSystemBase()
{

}

bool CWinSystemBase::InitWindowSystem()
{
  UpdateResolutions();

  m_bWindowCreated = true;

  return true;
}

void CWinSystemBase::GetResolutions(ResVector& vec)
{
 // add window resolution
  vec[WINDOW] = m_VecResInfo[0];
  vec[DESKTOP] = m_VecResInfo[1];

  for(unsigned int i = 0; i < m_VecResInfo.size(); i++)
  {
    vec.push_back(m_VecResInfo[i]);
  }
}

void CWinSystemBase::AddNewResolution(RESOLUTION_INFO newRes)
{
  // don't add res different than desktop
  if(newRes.iWidth != m_DesktopRes.iWidth || newRes.iHeight != m_DesktopRes.iHeight)
  {
    return;
  }

  // don't add if the refresh is the same
  if(newRes.fRefreshRate == m_DesktopRes.fRefreshRate)
  {
    return;
  }

  newRes.Overscan.left = 0;
  newRes.Overscan.top = 0;
  newRes.Overscan.right = newRes.iWidth;
  newRes.Overscan.bottom = newRes.iHeight;
  newRes.iScreen = PRIMARY_MONITOR;
  newRes.bFullScreen = true;
  newRes.iSubtitles = (int)(0.965 * newRes.iHeight);
  sprintf(newRes.strMode,
    "%dx%d @ %.2fHz (Full Screen)"
    , newRes.iWidth
    , newRes.iHeight
    , newRes.fRefreshRate);
  newRes.fPixelRatio = 1.0f;

  m_VecResInfo.push_back(newRes);
}

void CWinSystemBase::UpdateResolutions()
{
  m_VecResInfo.clear();

  GetDesktopRes(m_DesktopRes);

  // add 2 default res: 720p and desktop. the system will add the rest
  RESOLUTION_INFO res720p;

  res720p.iWidth = 1280;
  res720p.iHeight = 720;
  res720p.Overscan.left = 0;
  res720p.Overscan.top = 0;
  res720p.Overscan.right = res720p.iWidth;
  res720p.Overscan.bottom = res720p.iHeight;
  res720p.bFullScreen = false;
  res720p.iScreen = PRIMARY_MONITOR;
  res720p.iSubtitles = (int)(0.965 * 720);
 // res720p.dwFlags = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
  res720p.fPixelRatio = 1.0f;
  res720p.fRefreshRate = m_DesktopRes.fRefreshRate;
  sprintf(res720p.strMode, "720p 16:9 (Windowed)");

  m_VecResInfo.push_back(res720p);

  m_DesktopRes.Overscan.left = 0;
  m_DesktopRes.Overscan.top = 0;
  m_DesktopRes.Overscan.right = m_DesktopRes.iWidth;
  m_DesktopRes.Overscan.bottom = m_DesktopRes.iHeight;
  m_DesktopRes.iScreen = PRIMARY_MONITOR;
  m_DesktopRes.bFullScreen = true;
  m_DesktopRes.iSubtitles = (int)(0.965 * m_DesktopRes.iHeight);
  //m_DesktopRes.dwFlags = D3DPRESENTFLAG_WIDESCREEN;
  m_DesktopRes.fPixelRatio = 1.0f;
  sprintf(m_DesktopRes.strMode,
    "%dx%d @ %.2fHz (Full Screen)"
    , m_DesktopRes.iWidth
    , m_DesktopRes.iHeight
    , m_DesktopRes.fRefreshRate);

  m_VecResInfo.push_back(m_DesktopRes);

 // default res is 720p
  m_nCurrentResolution = 0;
}

bool CWinSystemBase::IsValidResolution(RESOLUTION_INFO res)
{
  if(m_VecResInfo.size() == 0)
    return false;

  return false;
}