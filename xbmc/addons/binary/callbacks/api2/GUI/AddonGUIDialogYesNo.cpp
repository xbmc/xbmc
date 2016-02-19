/*
 *      Copyright (C) 2015 Team KODI
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

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/callbacks/AddonCallbacks.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"
#include "dialogs/GUIDialogYesNo.h"
#include "messaging/helpers/DialogHelper.h"

#include "AddonGUIDialogYesNo.h"

using namespace ADDON;
using namespace KODI::MESSAGING;
using KODI::MESSAGING::HELPERS::DialogResponse;

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnDialog_YesNo::Init(::V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->GUI.Dialogs.YesNo.ShowAndGetInputSingleText       = CAddOnDialog_YesNo::ShowAndGetInputSingleText;
  callbacks->GUI.Dialogs.YesNo.ShowAndGetInputLineText         = CAddOnDialog_YesNo::ShowAndGetInputLineText;
  callbacks->GUI.Dialogs.YesNo.ShowAndGetInputLineButtonText   = CAddOnDialog_YesNo::ShowAndGetInputLineButtonText;
}

bool CAddOnDialog_YesNo::ShowAndGetInputSingleText(const char *heading,
                                                       const char *text,
                                                       bool       &bCanceled,
                                                       const char *noLabel,
                                                       const char *yesLabel)
{
  try
  {
    DialogResponse result = HELPERS::ShowYesNoDialogText(heading, text, noLabel, yesLabel);
    bCanceled = (result == DialogResponse::CANCELLED);
    return (result == DialogResponse::YES);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_YesNo::ShowAndGetInputLineText(const char *heading,
                                                     const char *line0,
                                                     const char *line1,
                                                     const char *line2,
                                                     const char *noLabel,
                                                     const char *yesLabel)
{
  try
  {
    return HELPERS::ShowYesNoDialogLines(heading, line0, line1, line2, noLabel, yesLabel) ==
      DialogResponse::YES;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDialog_YesNo::ShowAndGetInputLineButtonText(const char *heading,
                                                           const char *line0,
                                                           const char *line1,
                                                           const char *line2,
                                                           bool       &bCanceled,
                                                           const char *noLabel,
                                                           const char *yesLabel)
{
  try
  {
    DialogResponse result = HELPERS::ShowYesNoDialogLines(heading, line0, line1, line2, noLabel, yesLabel);
    bCanceled = (result == DialogResponse::CANCELLED);
    return (result == DialogResponse::YES);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

}; /* extern "C" */
}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
