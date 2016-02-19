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

struct CAddonExeCB_GUI_DialogKeyboard
{
  static bool Dialogs_Keyboard_ShowAndGetInputWithHead(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Dialogs_Keyboard_ShowAndGetInput(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Dialogs_Keyboard_ShowAndGetNewPasswordWithHead(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Dialogs_Keyboard_ShowAndGetNewPassword(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Dialogs_Keyboard_ShowAndVerifyNewPasswordWithHead(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Dialogs_Keyboard_ShowAndVerifyNewPassword(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Dialogs_Keyboard_ShowAndVerifyPassword(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Dialogs_Keyboard_ShowAndGetFilter(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Dialogs_Keyboard_SendTextToActiveKeyboard(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Dialogs_Keyboard_isKeyboardActivated(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
};

}; /* namespace KodiAPI */
}; /* namespace V2 */
