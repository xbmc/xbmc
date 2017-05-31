/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DialogYesNo.h"
#include "addons/kodi-addon-dev-kit/include/kodi/gui/DialogYesNo.h"

#include "addons/AddonDll.h"
#include "dialogs/GUIDialogYesNo.h"
#include "messaging/helpers/DialogHelper.h"
#include "utils/log.h"

using namespace KODI::MESSAGING;
using KODI::MESSAGING::HELPERS::DialogResponse;

extern "C"
{
namespace ADDON
{

void Interface_GUIDialogYesNo::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogYesNo = static_cast<AddonToKodiFuncTable_kodi_gui_dialogYesNo*>(malloc(sizeof(AddonToKodiFuncTable_kodi_gui_dialogYesNo)));

  addonInterface->toKodi->kodi_gui->dialogYesNo->show_and_get_input_single_text = show_and_get_input_single_text;
  addonInterface->toKodi->kodi_gui->dialogYesNo->show_and_get_input_line_text = show_and_get_input_line_text;
  addonInterface->toKodi->kodi_gui->dialogYesNo->show_and_get_input_line_button_text = show_and_get_input_line_button_text;
}

void Interface_GUIDialogYesNo::DeInit(AddonGlobalInterface* addonInterface)
{
  free(addonInterface->toKodi->kodi_gui->dialogYesNo);
}

bool Interface_GUIDialogYesNo::show_and_get_input_single_text(void* kodiBase,
                                                              const char* heading,
                                                              const char* text,
                                                              bool* canceled,
                                                              const char* noLabel,
                                                              const char* yesLabel)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogYesNo::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!heading || !text || !canceled || !noLabel || !yesLabel)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogYesNo::%s - invalid handler data (heading='%p', text='%p', "
                        "canceled='%p', noLabel='%p', yesLabel='%p') on addon '%s'", __FUNCTION__,
                            heading, text, canceled, noLabel, yesLabel, addon->ID().c_str());
    return false;
  }

  DialogResponse result = HELPERS::ShowYesNoDialogText(heading, text, noLabel, yesLabel);
  *canceled = (result == DialogResponse::CANCELLED);
  return (result == DialogResponse::YES);
}

bool Interface_GUIDialogYesNo::show_and_get_input_line_text(void* kodiBase,
                                                            const char* heading,
                                                            const char* line0,
                                                            const char* line1,
                                                            const char* line2,
                                                            const char* noLabel,
                                                            const char* yesLabel)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogYesNo::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!heading || !line0 || !line1 || !line2 || !noLabel || !yesLabel)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogYesNo::%s - invalid handler data (heading='%p', line0='%p', line1='%p', line2='%p', "
                        "noLabel='%p', yesLabel='%p') on addon '%s'", __FUNCTION__,
                            heading, line0, line1, line2, noLabel, yesLabel, addon->ID().c_str());
    return false;
  }

  return HELPERS::ShowYesNoDialogLines(heading, line0, line1, line2, noLabel, yesLabel) ==
    DialogResponse::YES;
}

bool Interface_GUIDialogYesNo::show_and_get_input_line_button_text(void* kodiBase,
                                                                   const char* heading,
                                                                   const char* line0,
                                                                   const char* line1,
                                                                   const char* line2,
                                                                   bool* canceled,
                                                                   const char* noLabel,
                                                                   const char* yesLabel)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogYesNo::%s - invalid data", __FUNCTION__);
    return false;
  }

  if (!heading || !line0 || !line1 || !line2 || !canceled || !noLabel || !yesLabel)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogYesNo::%s - invalid handler data (heading='%p', line0='%p', line1='%p', line2='%p', "
                        "canceled='%p', noLabel='%p', yesLabel='%p') on addon '%s'", __FUNCTION__,
                            heading, line0, line1, line2, canceled, noLabel, yesLabel, addon->ID().c_str());
    return false;
  }

  DialogResponse result = HELPERS::ShowYesNoDialogLines(heading, line0, line1, line2, noLabel, yesLabel);
  *canceled = (result == DialogResponse::CANCELLED);
  return (result == DialogResponse::YES);
}

} /* namespace ADDON */
} /* extern "C" */
