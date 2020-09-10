/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "YesNo.h"

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/dialogs/YesNo.h"
#include "dialogs/GUIDialogYesNo.h"
#include "messaging/helpers/DialogHelper.h"
#include "utils/log.h"

using namespace KODI::MESSAGING;
using KODI::MESSAGING::HELPERS::DialogResponse;

namespace ADDON
{

void Interface_GUIDialogYesNo::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->dialogYesNo = new AddonToKodiFuncTable_kodi_gui_dialogYesNo();

  addonInterface->toKodi->kodi_gui->dialogYesNo->show_and_get_input_single_text =
      show_and_get_input_single_text;
  addonInterface->toKodi->kodi_gui->dialogYesNo->show_and_get_input_line_text =
      show_and_get_input_line_text;
  addonInterface->toKodi->kodi_gui->dialogYesNo->show_and_get_input_line_button_text =
      show_and_get_input_line_button_text;
}

void Interface_GUIDialogYesNo::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->dialogYesNo;
}

bool Interface_GUIDialogYesNo::show_and_get_input_single_text(KODI_HANDLE kodiBase,
                                                              const char* heading,
                                                              const char* text,
                                                              bool* canceled,
                                                              const char* noLabel,
                                                              const char* yesLabel)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  if (!addon)
  {
    CLog::Log(LOGERROR, "Interface_GUIDialogYesNo::{} - invalid data", __func__);
    return false;
  }

  if (!heading || !text || !canceled || !noLabel || !yesLabel)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogYesNo::{} - invalid handler data (heading='{}', text='{}', "
              "canceled='{}', noLabel='{}', yesLabel='{}') on addon '{}'",
              __func__, static_cast<const void*>(heading), static_cast<const void*>(text),
              static_cast<void*>(canceled), static_cast<const void*>(noLabel),
              static_cast<const void*>(yesLabel), addon->ID());
    return false;
  }

  DialogResponse result = HELPERS::ShowYesNoDialogText(heading, text, noLabel, yesLabel);
  *canceled = (result == DialogResponse::CANCELLED);
  return (result == DialogResponse::YES);
}

bool Interface_GUIDialogYesNo::show_and_get_input_line_text(KODI_HANDLE kodiBase,
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
    CLog::Log(LOGERROR, "Interface_GUIDialogYesNo::{} - invalid data", __func__);
    return false;
  }

  if (!heading || !line0 || !line1 || !line2 || !noLabel || !yesLabel)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogYesNo::{} - invalid handler data (heading='{}', line0='{}', "
              "line1='{}', line2='{}', "
              "noLabel='{}', yesLabel='{}') on addon '{}'",
              __func__, static_cast<const void*>(heading), static_cast<const void*>(line0),
              static_cast<const void*>(line1), static_cast<const void*>(line2),
              static_cast<const void*>(noLabel), static_cast<const void*>(yesLabel), addon->ID());
    return false;
  }

  return HELPERS::ShowYesNoDialogLines(heading, line0, line1, line2, noLabel, yesLabel) ==
         DialogResponse::YES;
}

bool Interface_GUIDialogYesNo::show_and_get_input_line_button_text(KODI_HANDLE kodiBase,
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
    CLog::Log(LOGERROR, "Interface_GUIDialogYesNo::{} - invalid data", __func__);
    return false;
  }

  if (!heading || !line0 || !line1 || !line2 || !canceled || !noLabel || !yesLabel)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIDialogYesNo::{} - invalid handler data (heading='{}', line0='{}', "
              "line1='{}', line2='{}', "
              "canceled='{}', noLabel='{}', yesLabel='{}') on addon '{}'",
              __func__, static_cast<const void*>(heading), static_cast<const void*>(line0),
              static_cast<const void*>(line1), static_cast<const void*>(line2),
              static_cast<const void*>(canceled), static_cast<const void*>(noLabel),
              static_cast<const void*>(yesLabel), addon->ID());
    return false;
  }

  DialogResponse result =
      HELPERS::ShowYesNoDialogLines(heading, line0, line1, line2, noLabel, yesLabel);
  *canceled = (result == DialogResponse::CANCELLED);
  return (result == DialogResponse::YES);
}

} /* namespace ADDON */
