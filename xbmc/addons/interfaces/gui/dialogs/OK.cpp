/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OK.h"

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/dialogs/OK.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/Variant.h"
#include "utils/log.h"
using namespace KODI::MESSAGING;

extern "C"
{
namespace ADDON
{

void Interface_GUIDialogOK::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogOK = static_cast<AddonToKodiFuncTable_kodi_gui_dialogOK*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_dialogOK)));

  addonInterface->toKodi->kodi_gui->dialogOK->show_and_get_input_single_text = show_and_get_input_single_text;
  addonInterface->toKodi->kodi_gui->dialogOK->show_and_get_input_line_text = show_and_get_input_line_text;
}

void Interface_GUIDialogOK::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->dialogOK);
}

void Interface_GUIDialogOK::show_and_get_input_single_text(void* kodiBase, const char *heading, const char *text)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon || !heading || !text)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogOK:%s - invalid data (addon='%p', heading='%p', text='%p')",
              __FUNCTION__, kodiBase, heading, text);
    return;
  }

  HELPERS::ShowOKDialogText(CVariant{heading}, CVariant{text});
}

void Interface_GUIDialogOK::show_and_get_input_line_text(void* kodiBase, const char *heading, const char *line0, const char *line1, const char *line2)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon || !heading || !line0 || !line1 || !line2)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogOK::%s - invalid data (addon='%p', heading='%p', line0='%p', "
              "line1='%p', line2='%p')",
              __FUNCTION__, kodiBase, heading, line0, line1, line2);
    return;
  }
  HELPERS::ShowOKDialogLines(CVariant{ heading }, CVariant{ line0 }, CVariant{ line1 }, CVariant{ line2 });
}

} /* namespace ADDON */
} /* extern "C" */
