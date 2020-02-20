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
#include "addons/kodi-addon-dev-kit/include/kodi/gui/dialogs/TextViewer.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

extern "C"
{
namespace ADDON
{

void Interface_GUIDialogTextViewer::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogTextViewer = static_cast<AddonToKodiFuncTable_kodi_gui_dialogTextViewer*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_dialogTextViewer)));

  addonInterface->toKodi->kodi_gui->dialogTextViewer->open = open;
}

void Interface_GUIDialogTextViewer::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->dialogTextViewer);
}

void Interface_GUIDialogTextViewer::open(void* kodiBase, const char *heading, const char *text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogTextViewer::%s - invalid data", __FUNCTION__);
    return;
  }

  CGUIDialogTextViewer* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogTextViewer>(WINDOW_DIALOG_TEXT_VIEWER);
  if (!heading || !text || !dialog)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogTextViewer::%s - invalid handler data (heading='%p', text='%p', "
              "dialog='%p') on addon '%s'",
              __FUNCTION__, heading, text, static_cast<void*>(dialog), addon->ID().c_str());
    return;
  }

  dialog->SetHeading(heading);
  dialog->SetText(text);
  dialog->Open();
}

} /* namespace ADDON */
} /* extern "C" */
