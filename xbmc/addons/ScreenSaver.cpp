/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ScreenSaver.h"

#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"

using namespace ADDON;
using namespace KODI::ADDONS;

namespace
{
void get_properties(const KODI_HANDLE hdl, struct KODI_ADDON_SCREENSAVER_PROPS* props)
{
  if (hdl)
    static_cast<CScreenSaver*>(hdl)->GetProperties(props);
}
} // namespace

CScreenSaver::CScreenSaver(const AddonInfoPtr& addonInfo)
  : IAddonInstanceHandler(ADDON_INSTANCE_SCREENSAVER, addonInfo)
{
  m_ifc.screensaver = new AddonInstance_Screensaver;
  m_ifc.screensaver->toAddon = new KodiToAddonFuncTable_Screensaver();
  m_ifc.screensaver->toKodi = new AddonToKodiFuncTable_Screensaver();
  m_ifc.screensaver->toKodi->get_properties = get_properties;

  /* Open the class "kodi::addon::CInstanceScreensaver" on add-on side */
  if (CreateInstance() != ADDON_STATUS_OK)
    CLog::Log(LOGFATAL, "Screensaver: failed to create instance for '{}' and not usable!", ID());
}

CScreenSaver::~CScreenSaver()
{
  /* Destroy the class "kodi::addon::CInstanceScreensaver" on add-on side */
  DestroyInstance();

  delete m_ifc.screensaver->toAddon;
  delete m_ifc.screensaver->toKodi;
  delete m_ifc.screensaver;
}

bool CScreenSaver::Start()
{
  if (m_ifc.screensaver->toAddon->start)
    return m_ifc.screensaver->toAddon->start(m_ifc.hdl);
  return false;
}

void CScreenSaver::Stop()
{
  if (m_ifc.screensaver->toAddon->stop)
    m_ifc.screensaver->toAddon->stop(m_ifc.hdl);
}

void CScreenSaver::Render()
{
  if (m_ifc.screensaver->toAddon->render)
    m_ifc.screensaver->toAddon->render(m_ifc.hdl);
}

void CScreenSaver::GetProperties(struct KODI_ADDON_SCREENSAVER_PROPS* props)
{
  if (!props)
    return;

  const auto winSystem = CServiceBroker::GetWinSystem();
  if (!winSystem)
    return;

  props->x = 0;
  props->y = 0;
  props->device = winSystem->GetHWContext();
  props->width = winSystem->GetGfxContext().GetWidth();
  props->height = winSystem->GetGfxContext().GetHeight();
  props->pixelRatio = winSystem->GetGfxContext().GetResInfo().fPixelRatio;
}
