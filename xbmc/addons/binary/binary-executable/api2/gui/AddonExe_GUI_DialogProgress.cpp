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

#include "AddonExe_GUI_DialogProgress.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/GUI/AddonGUIDialogProgress.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_New(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  GUIHANDLE hdl = GUI::CAddOnDialog_Progress::New(addon);
  PROCESS_ADDON_RETURN_CALL_WITH_POINTER(API_SUCCESS, hdl);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_Delete(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  GUI::CAddOnDialog_Progress::Delete(addon, (GUIHANDLE)ptr);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_Open(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  GUI::CAddOnDialog_Progress::Open(addon, (GUIHANDLE)ptr);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetHeading(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  char *heading;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_STRING,   &heading);
  GUI::CAddOnDialog_Progress::SetHeading(addon, (GUIHANDLE)ptr, heading);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetLine(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  char *lineText;
  unsigned int line;
  req.pop(API_UINT64_T,      &ptr);
  req.pop(API_UNSIGNED_INT,  &line);
  req.pop(API_STRING,        &lineText);
  GUI::CAddOnDialog_Progress::SetLine(addon, (GUIHANDLE)ptr, line, lineText);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetCanCancel(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  bool bCanCancel;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_BOOLEAN,  &bCanCancel);
  GUI::CAddOnDialog_Progress::SetCanCancel(addon, (GUIHANDLE)ptr, bCanCancel);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_IsCanceled(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  bool canceled = GUI::CAddOnDialog_Progress::IsCanceled(addon, (GUIHANDLE)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(API_SUCCESS, &canceled);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetPercentage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  int iPercentage;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_INT,      &iPercentage);
  GUI::CAddOnDialog_Progress::SetPercentage(addon, (GUIHANDLE)ptr, iPercentage);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_GetPercentage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  unsigned int space = GUI::CAddOnDialog_Progress::GetPercentage(addon, (GUIHANDLE)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_INT(API_SUCCESS, &space);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_ShowProgressBar(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  bool bOnOff;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_BOOLEAN,  &bOnOff);
  GUI::CAddOnDialog_Progress::ShowProgressBar(addon, (GUIHANDLE)ptr, bOnOff);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetProgressMax(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  int iMax;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_INT,      &iMax);
  GUI::CAddOnDialog_Progress::SetProgressMax(addon, (GUIHANDLE)ptr, iMax);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_SetProgressAdvance(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  int nSteps;
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  req.pop(API_INT,      &nSteps);
  GUI::CAddOnDialog_Progress::SetProgressAdvance(addon, (GUIHANDLE)ptr, nSteps);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_GUI_DialogProgress::Dialogs_Progress_Abort(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  uint64_t ptr;
  req.pop(API_UINT64_T, &ptr);
  bool abort = GUI::CAddOnDialog_Progress::Abort(addon, (GUIHANDLE)ptr);
  PROCESS_ADDON_RETURN_CALL_WITH_BOOLEAN(API_SUCCESS, &abort);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
