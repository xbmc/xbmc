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

namespace ADDON
{

CScreenSaver::CScreenSaver(const AddonInfoPtr& addonInfo)
  : IAddonInstanceHandler(ADDON_INSTANCE_SCREENSAVER, addonInfo)
{
  m_name = Name();
  m_presets = CSpecialProtocol::TranslatePath(Path());
  m_profile = CSpecialProtocol::TranslatePath(Profile());

  m_ifc.screensaver = new AddonInstance_Screensaver;
  m_ifc.screensaver->props = new AddonProps_Screensaver();
  m_ifc.screensaver->props->x = 0;
  m_ifc.screensaver->props->y = 0;
  m_ifc.screensaver->props->device = CServiceBroker::GetWinSystem()->GetHWContext();
  m_ifc.screensaver->props->width = CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth();
  m_ifc.screensaver->props->height = CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight();
  m_ifc.screensaver->props->pixelRatio =
      CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo().fPixelRatio;
  m_ifc.screensaver->props->name = m_name.c_str();
  m_ifc.screensaver->props->presets = m_presets.c_str();
  m_ifc.screensaver->props->profile = m_profile.c_str();

  m_ifc.screensaver->toKodi = new AddonToKodiFuncTable_Screensaver();
  m_ifc.screensaver->toKodi->kodiInstance = this;

  m_ifc.screensaver->toAddon = new KodiToAddonFuncTable_Screensaver();

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
  delete m_ifc.screensaver->props;
  delete m_ifc.screensaver;
}

bool CScreenSaver::Start()
{
  if (m_ifc.screensaver->toAddon->Start)
    return m_ifc.screensaver->toAddon->Start(m_ifc.screensaver);
  return false;
}

void CScreenSaver::Stop()
{
  if (m_ifc.screensaver->toAddon->Stop)
    m_ifc.screensaver->toAddon->Stop(m_ifc.screensaver);
}

void CScreenSaver::Render()
{
  if (m_ifc.screensaver->toAddon->Render)
    m_ifc.screensaver->toAddon->Render(m_ifc.screensaver);
}

} /* namespace ADDON */
