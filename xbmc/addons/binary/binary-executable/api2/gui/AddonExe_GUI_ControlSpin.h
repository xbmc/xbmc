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

struct CAddonExeCB_GUI_ControlSpin
{
  static bool Control_Spin_SetVisible(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_SetEnabled(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_SetText(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_Reset(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_SetType(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_AddStringLabel(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_AddIntLabel(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_SetStringValue(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_GetStringValue(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_SetIntRange(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_SetIntValue(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_GetIntValue(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_SetFloatRange(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_SetFloatValue(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_GetFloatValue(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_Spin_SetFloatInterval(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
};

}; /* namespace KodiAPI */
}; /* namespace V2 */
