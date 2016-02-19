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

#include "AddonExe_GUI_DialogTextViewer.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/GUI/AddonGUIDialogTextViewer.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_GUI_DialogTextViewer::Dialogs_TextViewer_Show(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* heading;
  char* text;
  req.pop(API_STRING, &heading);
  req.pop(API_STRING, &text);
  GUI::CAddOnDialog_TextViewer::Open(heading, text);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
