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

#include "AddonExe_PVR_General.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/PVR/AddonCallbacksPVR.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_PVR_General::AddMenuHook(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  PVR_MENUHOOK hook;
  for (unsigned int i = 0; i < sizeof(PVR_MENUHOOK); ++i)
    req.pop(API_UINT8_T, &hook+i);
  PVR::CAddonCallbacksPVR::add_menu_hook(addon, &hook);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_PVR_General::Recording(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  char* strName;
  char* strFileName;
  bool bOnOff;
  req.pop(API_STRING, &strName);
  req.pop(API_STRING, &strFileName);
  req.pop(API_BOOLEAN, &bOnOff);
  PVR::CAddonCallbacksPVR::recording(addon, strName, strFileName, bOnOff);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
