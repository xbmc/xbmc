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
#include "ScreenSaver.h"
#include "ServiceBroker.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GraphicContext.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "settings/Settings.h"
#include "utils/AlarmClock.h"
#include "utils/URIUtils.h"
#include "windowing/WindowingFactory.h"

// What sound does a python screensaver make?
#define SCRIPT_ALARM "sssssscreensaver"

#define SCRIPT_TIMEOUT 15 // seconds

namespace ADDON
{

CScreenSaver::CScreenSaver(const char *addonID)
    : ADDON::CAddonDll(AddonProps(addonID, ADDON_UNKNOWN))
{
  memset(&m_info, 0, sizeof(m_info));
}

bool CScreenSaver::IsInUse() const
{
  return CServiceBroker::GetSettings().GetString(CSettings::SETTING_SCREENSAVER_MODE) == ID();
}

bool CScreenSaver::CreateScreenSaver()
{
  if (CScriptInvocationManager::GetInstance().HasLanguageInvoker(LibPath()))
  {
    // Don't allow a previously-scheduled alarm to kill our new screensaver
    g_alarmClock.Stop(SCRIPT_ALARM, true);

    if (!CScriptInvocationManager::GetInstance().Stop(LibPath()))
      CScriptInvocationManager::GetInstance().ExecuteAsync(LibPath(), AddonPtr(new CScreenSaver(*this)));
    return true;
  }
 // pass it the screen width,height
 // and the name of the screensaver
  int iWidth = g_graphicsContext.GetWidth();
  int iHeight = g_graphicsContext.GetHeight();

#ifdef HAS_DX
  m_info.device     = g_Windowing.Get3D11Context();
#else
  m_info.device     = NULL;
#endif
  m_info.x          = 0;
  m_info.y          = 0;
  m_info.width      = iWidth;
  m_info.height     = iHeight;
  m_info.pixelRatio = g_graphicsContext.GetResInfo().fPixelRatio;
  m_info.name       = strdup(Name().c_str());
  m_info.presets    = strdup(CSpecialProtocol::TranslatePath(Path()).c_str());
  m_info.profile    = strdup(CSpecialProtocol::TranslatePath(Profile()).c_str());

  if (CAddonDll::Create(ADDON_INSTANCE_SCREENSAVER, &m_struct, &m_info) == ADDON_STATUS_OK)
    return true;

  return false;
}

void CScreenSaver::Start()
{
  // notify screen saver that they should start
  if (Initialized()) m_struct.Start();
}

void CScreenSaver::Stop()
{
  // notify screen saver that they should start
  if (Initialized()) m_struct.Stop();
}

void CScreenSaver::Render()
{
  // ask screensaver to render itself
  if (Initialized()) m_struct.Render();
}

void CScreenSaver::Destroy()
{
#ifdef HAS_PYTHON
  if (URIUtils::HasExtension(LibPath(), ".py"))
  {
    /* FIXME: This is a hack but a proper fix is non-trivial. Basically this code
     * makes sure the addon gets terminated after we've moved out of the screensaver window.
     * If we don't do this, we may simply lockup.
     */
    g_alarmClock.Start(SCRIPT_ALARM, SCRIPT_TIMEOUT, "StopScript(" + LibPath() + ")", true, false);
    return;
  }
#endif
  // Release what was allocated in method CScreenSaver::CreateScreenSaver.
  if (m_info.name)
  {
    free((void *) m_info.name);
    m_info.name = nullptr;
  }
  if (m_info.presets)
  {
    free((void *) m_info.presets);
    m_info.presets = nullptr;
  }
  if (m_info.profile)
  {
    free((void *) m_info.profile);
    m_info.profile = nullptr;
  }

  CAddonDll::Destroy();
}

} /*namespace ADDON*/

