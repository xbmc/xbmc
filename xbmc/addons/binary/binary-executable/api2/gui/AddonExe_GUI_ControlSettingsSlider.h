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

struct CAddonExeCB_GUI_ControlSettingsSlider
{
  static bool Control_SettingsSlider_SetVisible(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_SetEnabled(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_SetText(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_Reset(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_SetIntRange(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_SetIntValue(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_GetIntValue(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_SetIntInterval(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_SetPercentage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_GetPercentage(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_SetFloatRange(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_SetFloatValue(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_GetFloatValue(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Control_SettingsSlider_SetFloatInterval(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
};

}; /* namespace KodiAPI */
}; /* namespace V2 */
