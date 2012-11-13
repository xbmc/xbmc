/*
 *      Copyright (C) 2005-2012 Team XBMC
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
#include "ScreenSaver.h"
#include "settings/Settings.h"
#include "windowing/WindowingFactory.h"

#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#include "utils/AlarmClock.h"

// What sound does a python screensaver make?
#define PYTHON_ALARM "sssssscreensaver"

#define PYTHON_SCRIPT_TIMEOUT 5 // seconds
#endif

namespace ADDON
{

  CScreenSaver::CScreenSaver(const char *addonID)
   : ADDON::CAddonDll<DllScreenSaver, ScreenSaver, SCR_PROPS>(AddonProps(addonID, ADDON_UNKNOWN, "", ""))
  {
  }

bool CScreenSaver::CreateScreenSaver()
{
#ifdef HAS_PYTHON
  if (URIUtils::GetExtension(LibPath()).Equals(".py", false))
  {
    // Don't allow a previously-scheduled alarm to kill our new screensaver
    g_alarmClock.Stop(PYTHON_ALARM, true);

    if (!g_pythonParser.StopScript(LibPath()))
      g_pythonParser.evalFile(LibPath(), AddonPtr(new CScreenSaver(Props())));
    return true;
  }
#endif
 // pass it the screen width,height
 // and the name of the screensaver
  int iWidth = g_graphicsContext.GetWidth();
  int iHeight = g_graphicsContext.GetHeight();

  m_pInfo = new SCR_PROPS;
#ifdef HAS_DX
  m_pInfo->device     = g_Windowing.Get3DDevice();
#else
  m_pInfo->device     = NULL;
#endif
  m_pInfo->x          = 0;
  m_pInfo->y          = 0;
  m_pInfo->width      = iWidth;
  m_pInfo->height     = iHeight;
  m_pInfo->pixelRatio = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].fPixelRatio;
  m_pInfo->name       = strdup(Name().c_str());
  m_pInfo->presets    = strdup(CSpecialProtocol::TranslatePath(Path()).c_str());
  m_pInfo->profile    = strdup(CSpecialProtocol::TranslatePath(Profile()).c_str());

  if (CAddonDll<DllScreenSaver, ScreenSaver, SCR_PROPS>::Create() == ADDON_STATUS_OK)
    return true;

  return false;
}

void CScreenSaver::Start()
{
  // notify screen saver that they should start
  if (Initialized()) m_pStruct->Start();
}

void CScreenSaver::Render()
{
  // ask screensaver to render itself
  if (Initialized()) m_pStruct->Render();
}

void CScreenSaver::GetInfo(SCR_INFO *info)
{
  // get info from screensaver
  if (Initialized()) m_pStruct->GetInfo(info);
}

void CScreenSaver::Destroy()
{
#ifdef HAS_PYTHON
  if (URIUtils::GetExtension(LibPath()).Equals(".py", false))
  {
    g_alarmClock.Start(PYTHON_ALARM, PYTHON_SCRIPT_TIMEOUT, "StopScript(" + LibPath() + ")", true, false);
    return;
  }
#endif
  // Release what was allocated in method CScreenSaver::CreateScreenSaver.
  if (m_pInfo)
  {
    free((void *) m_pInfo->name);
    free((void *) m_pInfo->presets);
    free((void *) m_pInfo->profile);

    delete m_pInfo;
    m_pInfo = NULL;
  }

  CAddonDll<DllScreenSaver, ScreenSaver, SCR_PROPS>::Destroy();
}

} /*namespace ADDON*/

