/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenu.h"

#include "ServiceBroker.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/dialogs/ContextMenu.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIDialogContextMenu::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogContextMenu =
      new AddonToKodiFuncTable_kodi_gui_dialogContextMenu();

  addonInterface->toKodi->kodi_gui->dialogContextMenu->open = open;
}

void Interface_GUIDialogContextMenu::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->dialogContextMenu;
}

int Interface_GUIDialogContextMenu::open(KODI_HANDLE kodiBase,
                                         const char* heading,
                                         const char* entries[],
                                         unsigned int size)
{
  const auto* addon = static_cast<const CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::LogF(LOGERROR, "Invalid data");
    return -1;
  }

  if (!heading || !entries)
  {
    CLog::LogF(LOGERROR,
               "Invalid handler data (heading='{}', "
               "entries='{}') on addon '{}'",
               static_cast<const void*>(heading), static_cast<const void*>(entries), addon->ID());
    return -1;
  }

  CContextButtons choices;
  for (unsigned int i = 0; i < size; ++i)
    choices.Add(i, entries[i]);

  return CGUIDialogContextMenu::Show(choices);
}

} /* namespace ADDON */
