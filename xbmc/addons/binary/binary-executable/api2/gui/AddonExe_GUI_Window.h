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

struct CAddonExeCB_GUI_Window
{
  static bool Window_New(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_Delete(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_Show(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_Close(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_DoModal(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_SetFocusId(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetFocusId(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_SetProperty(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_SetPropertyInt(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_SetPropertyBool(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_SetPropertyDouble(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetProperty(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetPropertyInt(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetPropertyBool(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetPropertyDouble(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_ClearProperties(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_ClearProperty(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetListSize(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_ClearList(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_AddStringItem(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_AddItem(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_RemoveItem(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_RemoveItemFile(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetListItem(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_SetCurrentListPosition(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetCurrentListPosition(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_SetControlLabel(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_MarkDirtyRegion(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_SetCallbacks(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_Button(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_Edit(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_FadeLabel(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_Image(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_Label(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_Progress(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_RadioButton(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_Rendering(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_SettingsSlider(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_Slider(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_Spin(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
  static bool Window_GetControl_TextBox(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp);
};

}; /* namespace KodiAPI */
}; /* namespace V2 */
