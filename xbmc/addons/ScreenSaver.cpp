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

CScreenSaver::CScreenSaver(AddonProps props)
 : ADDON::CAddonDll(std::move(props))
{
  memset(&m_struct, 0, sizeof(m_struct));
}

CScreenSaver::CScreenSaver(const char *addonID)
 : ADDON::CAddonDll(AddonProps(addonID, ADDON_UNKNOWN))
{
  memset(&m_struct, 0, sizeof(m_struct));
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

  m_name = Name();
  m_presets = CSpecialProtocol::TranslatePath(Path());
  m_profile = CSpecialProtocol::TranslatePath(Profile());

#ifdef HAS_DX
  m_info.device = g_Windowing.Get3D11Context();
#else
  m_info.device = nullptr;
#endif
  m_info.x = 0;
  m_info.y = 0;
  m_info.width = g_graphicsContext.GetWidth();
  m_info.height = g_graphicsContext.GetHeight();
  m_info.pixelRatio = g_graphicsContext.GetResInfo().fPixelRatio;
  m_info.name = m_name.c_str();
  m_info.presets = m_presets.c_str();
  m_info.profile = m_profile.c_str();

  if (CAddonDll::Create(ADDON_INSTANCE_SCREENSAVER, &m_struct, &m_info) == ADDON_STATUS_OK)
    return true;

  return false;
}

void CScreenSaver::Start()
{
  // notify screen saver that they should start
  if (m_struct.Start)
    m_struct.Start();
}

void CScreenSaver::Stop()
{
  if (m_struct.Stop)
    m_struct.Stop();
}

void CScreenSaver::Render()
{
  // ask screensaver to render itself
  if (m_struct.Render)
    m_struct.Render();
}

void CScreenSaver::Destroy()
{
  if (URIUtils::HasExtension(LibPath(), ".py"))
  {
    /* FIXME: This is a hack but a proper fix is non-trivial. Basically this code
     * makes sure the addon gets terminated after we've moved out of the screensaver window.
     * If we don't do this, we may simply lockup.
     */
    g_alarmClock.Start(SCRIPT_ALARM, SCRIPT_TIMEOUT, "StopScript(" + LibPath() + ")", true, false);
    return;
  }

  memset(&m_struct, 0, sizeof(m_struct));
  CAddonDll::Destroy();
}

} /* namespace ADDON */
