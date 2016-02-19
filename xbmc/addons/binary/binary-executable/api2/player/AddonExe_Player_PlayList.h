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

struct CAddonExeCB_Player_PlayList
{
  static bool PlayList_New(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool PlayList_Delete(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool PlayList_LoadPlaylist(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool PlayList_AddItemURL(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool PlayList_AddItemList(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool PlayList_RemoveItem(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool PlayList_ClearList(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool PlayList_GetListSize(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool PlayList_GetListPosition(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool PlayList_Shuffle(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool PlayList_GetItem(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
};

}; /* namespace KodiAPI */
}; /* namespace V2 */
