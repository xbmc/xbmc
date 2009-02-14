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
#include "InfoLoader.h"
#include "Weather.h"
#include "SystemInfo.h"
#include "GUIWindowManager.h"

CBackgroundLoader::CBackgroundLoader(CInfoLoader *callback) : CThread()
{
  m_callback = callback;
}

CBackgroundLoader::~CBackgroundLoader()
{
}

void CBackgroundLoader::Start()
{
  CThread::Create(true);
}

void CBackgroundLoader::Process()
{
  if (m_callback)
  {
    GetInformation();
    // and inform our callback that we're done
    m_callback->LoaderFinished();
  }
}

CInfoLoader::CInfoLoader(const char *type)
{
  m_refreshTime = 0;
  m_busy = true;
  m_backgroundLoader = NULL;
  m_type = type;
}

CInfoLoader::~CInfoLoader()
{
  if (m_backgroundLoader)
    delete m_backgroundLoader;
}

void CInfoLoader::Refresh()
{
  if (!m_backgroundLoader)
  {
    if (m_type == "weather")
      m_backgroundLoader = new CBackgroundWeatherLoader(this);
    else if (m_type == "sysinfo")
      m_backgroundLoader = new CBackgroundSystemInfoLoader(this);

    if (!m_backgroundLoader)
    {
      CLog::Log(LOGERROR, "Unable to start the background %s loader", m_type.c_str());
      return;
    }

    m_backgroundLoader->Start();
  }
  m_busy = true;
}

void CInfoLoader::LoaderFinished()
{
  m_refreshTime = timeGetTime() + TimeToNextRefreshInMs();
  m_backgroundLoader = NULL;
  if (m_type == "weather" && m_busy)
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_WEATHER_FETCHED);
    m_gWindowManager.SendThreadMessage(msg);
  }
  m_busy = false;
}

const char *CInfoLoader::GetInfo(DWORD dwInfo)
{
  // Refresh if need be
  if (m_refreshTime < timeGetTime())
  {
    Refresh();
  }
  if (m_busy && (m_type != "sysinfo") )
  {
    return BusyInfo(dwInfo);
  }
  return TranslateInfo(dwInfo);
}

const char *CInfoLoader::BusyInfo(DWORD dwInfo)
{
  m_busyText = g_localizeStrings.Get(503);
  return m_busyText.c_str();
}

const char *CInfoLoader::TranslateInfo(DWORD dwInfo)
{
  return "";
}

void CInfoLoader::ResetTimer()
{
  m_refreshTime = timeGetTime();
}

