#pragma once
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

#include "addons/binary/binary-executable/requestpacket.h"
#include "addons/binary/binary-executable/responsepacket.h"

struct AddonCB;

namespace V2
{
namespace KodiAPI
{

struct CAddonExeCB_GUI_General
{
  static bool GUIGeneral_Lock(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GUIGeneral_Unlock(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GUIGeneral_GetScreenHeight(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GUIGeneral_GetScreenWidth(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GUIGeneral_GetVideoResolution(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GUIGeneral_GetCurrentWindowDialogId(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool GUIGeneral_GetCurrentWindowId(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
};

}; /* namespace KodiAPI */
}; /* namespace V2 */
