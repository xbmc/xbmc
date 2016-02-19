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

#include "AddonExe_GUI_DialogSelect.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/GUI/AddonGUIDialogSelect.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_GUI_DialogSelect::Dialogs_Select_Show(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  int select;
  char* heading;
  uint32_t size;
  req.pop(API_STRING,    &heading);
  req.pop(API_INT,       &select);
  req.pop(API_UINT32_T,  &size);
  const char **entries = (const char**)malloc(size*sizeof(const char**));
  for (uint32_t i = 0; i < size; ++i)
    req.pop(API_STRING, &entries[i]);
  int selected = GUI::CAddOnDialog_Select::Open(heading, entries, size, select);
  free(entries);
  PROCESS_ADDON_RETURN_CALL_WITH_INT(API_SUCCESS, &selected);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
