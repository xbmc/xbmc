/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextViewer.h"

#include "ServiceBroker.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/dialogs/TextViewer.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIDialogTextViewer::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogTextViewer =
      new AddonToKodiFuncTable_kodi_gui_dialogTextViewer();

  addonInterface->toKodi->kodi_gui->dialogTextViewer->open = open;
}

void Interface_GUIDialogTextViewer::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->dialogTextViewer;
}

void Interface_GUIDialogTextViewer::open(KODI_HANDLE kodiBase,
                                         const char* heading,
                                         const char* text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogTextViewer::{} - invalid data", __func__);
    return;
  }

  CGUIDialogTextViewer* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogTextViewer>(
          WINDOW_DIALOG_TEXT_VIEWER);
  if (!heading || !text || !dialog)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogTextViewer::{} - invalid handler data (heading='{}', text='{}', "
              "dialog='{}') on addon '{}'",
              __func__, static_cast<const void*>(heading), static_cast<const void*>(text),
              static_cast<void*>(dialog), addon->ID());
    return;
  }

  dialog->SetHeading(heading);
  dialog->SetText(text);
  dialog->Open();
}

} /* namespace ADDON */
