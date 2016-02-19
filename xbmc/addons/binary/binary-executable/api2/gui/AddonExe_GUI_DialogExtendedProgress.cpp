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

#include "AddonExe_GUI_DialogExtendedProgress.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/GUI/AddonGUIDialogExtendedProgressBar.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_New(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* title;
  req.pop(API_STRING, &title);
  GUIHANDLE hdl = GUI::CAddOnDialog_ExtendedProgress::New(addon, title);
  PROCESS_ADDON_RETURN_CALL_WITH_POINTER(API_SUCCESS, hdl);
  return true;
}

bool CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Delete(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  GUI::CAddOnDialog_ExtendedProgress::Delete(addon, (GUIHANDLE)ptr);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Title(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  uint32_t retValue = API_SUCCESS;
  unsigned int iMaxStringSize = 256*sizeof(char);
  req.pop(API_UINT64_T, &ptr);
  char* title = (char*)malloc(iMaxStringSize);
  GUI::CAddOnDialog_ExtendedProgress::Title(addon, (GUIHANDLE)ptr, *title, iMaxStringSize);
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T, &retValue);
  resp.push(API_STRING,    title);
  resp.finalise();
  free(title);
  return true;
}

bool CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetTitle(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  char *title;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_STRING,   &title);
  GUI::CAddOnDialog_ExtendedProgress::SetTitle(addon, (GUIHANDLE)ptr, title);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Text(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  uint32_t retValue = API_SUCCESS;
  unsigned int iMaxStringSize = 256*sizeof(char);
  req.pop(API_UINT64_T, &ptr);
  char* text = (char*)malloc(iMaxStringSize);
  GUI::CAddOnDialog_ExtendedProgress::Text(addon, (GUIHANDLE)ptr, *text, iMaxStringSize);
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T, &retValue);
  resp.push(API_STRING,    text);
  resp.finalise();
  free(text);
  return true;
}

bool CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetText(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  char *text;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_STRING,   &text);
  GUI::CAddOnDialog_ExtendedProgress::SetTitle(addon, (GUIHANDLE)ptr, text);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_IsFinished(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  bool finished = GUI::CAddOnDialog_ExtendedProgress::IsFinished(addon, (GUIHANDLE)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(API_SUCCESS, &finished);
  return true;
}

bool CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_MarkFinished(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  GUI::CAddOnDialog_ExtendedProgress::MarkFinished(addon, (GUIHANDLE)ptr);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_Percentage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  float percentage = GUI::CAddOnDialog_ExtendedProgress::Percentage(addon, (GUIHANDLE)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_FLOAT(API_SUCCESS, &percentage);
  return true;
}

bool CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetPercentage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  float percentage;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_INT,      &percentage);
  GUI::CAddOnDialog_ExtendedProgress::SetPercentage(addon, (GUIHANDLE)ptr, percentage);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogExtendedProgress::Dialogs_ExtendedProgress_SetProgress(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  int itemCount;
  int currentItem;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_INT,      &currentItem);
  req.pop(API_INT,      &itemCount);
  GUI::CAddOnDialog_ExtendedProgress::SetProgress(addon, (GUIHANDLE)ptr, currentItem, itemCount);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
