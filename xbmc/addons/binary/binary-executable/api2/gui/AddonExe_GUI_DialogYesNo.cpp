/*
 *      Copyright (C) 2016 Team KODI
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

#include "AddonExe_GUI_DialogYesNo.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/GUI/AddonGUIDialogYesNo.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_GUI_DialogYesNo::Dialogs_YesNo_ShowAndGetInputSingleText(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* heading;
  char* text;
  bool  bCanceled;
  char* noLabel;
  char* yesLabel;
  req.pop(API_STRING, &heading);
  req.pop(API_STRING, &text);
  req.pop(API_STRING, &noLabel);
  req.pop(API_STRING, &yesLabel);
  bool ret = GUI::CAddOnDialog_YesNo::ShowAndGetInputSingleText(heading, text, bCanceled, noLabel, yesLabel);
  PROCESS_ADDON_RETURN_CALL_WITH_TWO_BOOLEAN(API_SUCCESS, &ret, &bCanceled);
  return true;
}

bool CAddonExeCB_GUI_DialogYesNo::Dialogs_YesNo_ShowAndGetInputLineText(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* heading;
  char* line0;
  char* line1;
  char* line2;
  char* noLabel;
  char* yesLabel;
  req.pop(API_STRING, &heading);
  req.pop(API_STRING, &line0);
  req.pop(API_STRING, &line1);
  req.pop(API_STRING, &line2);
  req.pop(API_STRING, &noLabel);
  req.pop(API_STRING, &yesLabel);
  bool ret = GUI::CAddOnDialog_YesNo::ShowAndGetInputLineText(heading, line0, line1, line2, noLabel, yesLabel);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(API_SUCCESS, &ret);
  return true;
}

bool CAddonExeCB_GUI_DialogYesNo::Dialogs_YesNo_ShowAndGetInputLineButtonText(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* heading;
  char* line0;
  char* line1;
  char* line2;
  bool  bCanceled;
  char* noLabel;
  char* yesLabel;
  req.pop(API_STRING, &heading);
  req.pop(API_STRING, &line0);
  req.pop(API_STRING, &line1);
  req.pop(API_STRING, &line2);
  req.pop(API_STRING, &noLabel);
  req.pop(API_STRING, &yesLabel);
  bool ret = GUI::CAddOnDialog_YesNo::ShowAndGetInputLineButtonText(heading, line0, line1, line2, bCanceled, noLabel, yesLabel);
  PROCESS_ADDON_RETURN_CALL_WITH_TWO_BOOLEAN(API_SUCCESS, &ret, &bCanceled);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
