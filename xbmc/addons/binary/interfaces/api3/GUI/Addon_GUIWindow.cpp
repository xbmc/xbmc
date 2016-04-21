/*
 *      Copyright (C) 2015-2016 Team KODI
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

#include "Addon_GUIWindow.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/GUI/Addon_GUIWindow.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnWindow::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.Window.New                     = V2::KodiAPI::GUI::CAddOnWindow::New;
  interfaces->GUI.Window.Delete                  = V2::KodiAPI::GUI::CAddOnWindow::Delete;
  interfaces->GUI.Window.SetCallbacks            = V2::KodiAPI::GUI::CAddOnWindow::SetCallbacks;
  interfaces->GUI.Window.Show                    = V2::KodiAPI::GUI::CAddOnWindow::Show;
  interfaces->GUI.Window.Close                   = V2::KodiAPI::GUI::CAddOnWindow::Close;
  interfaces->GUI.Window.DoModal                 = V2::KodiAPI::GUI::CAddOnWindow::DoModal;
  interfaces->GUI.Window.SetFocusId              = V2::KodiAPI::GUI::CAddOnWindow::SetFocusId;
  interfaces->GUI.Window.GetFocusId              = V2::KodiAPI::GUI::CAddOnWindow::GetFocusId;
  interfaces->GUI.Window.SetProperty             = V2::KodiAPI::GUI::CAddOnWindow::SetProperty;
  interfaces->GUI.Window.SetPropertyInt          = V2::KodiAPI::GUI::CAddOnWindow::SetPropertyInt;
  interfaces->GUI.Window.SetPropertyBool         = V2::KodiAPI::GUI::CAddOnWindow::SetPropertyBool;
  interfaces->GUI.Window.SetPropertyDouble       = V2::KodiAPI::GUI::CAddOnWindow::SetPropertyDouble;
  interfaces->GUI.Window.GetProperty             = V2::KodiAPI::GUI::CAddOnWindow::GetProperty;
  interfaces->GUI.Window.GetPropertyInt          = V2::KodiAPI::GUI::CAddOnWindow::GetPropertyInt;
  interfaces->GUI.Window.GetPropertyBool         = V2::KodiAPI::GUI::CAddOnWindow::GetPropertyBool;
  interfaces->GUI.Window.GetPropertyDouble       = V2::KodiAPI::GUI::CAddOnWindow::GetPropertyDouble;
  interfaces->GUI.Window.ClearProperties         = V2::KodiAPI::GUI::CAddOnWindow::ClearProperties;
  interfaces->GUI.Window.ClearProperty           = V2::KodiAPI::GUI::CAddOnWindow::ClearProperty;
  interfaces->GUI.Window.GetListSize             = V2::KodiAPI::GUI::CAddOnWindow::GetListSize;
  interfaces->GUI.Window.ClearList               = V2::KodiAPI::GUI::CAddOnWindow::ClearList;
  interfaces->GUI.Window.AddItem                 = V2::KodiAPI::GUI::CAddOnWindow::AddItem;
  interfaces->GUI.Window.AddStringItem           = V2::KodiAPI::GUI::CAddOnWindow::AddStringItem;
  interfaces->GUI.Window.RemoveItem              = V2::KodiAPI::GUI::CAddOnWindow::RemoveItem;
  interfaces->GUI.Window.RemoveItemFile          = V2::KodiAPI::GUI::CAddOnWindow::RemoveItemFile;
  interfaces->GUI.Window.GetListItem             = V2::KodiAPI::GUI::CAddOnWindow::GetListItem;
  interfaces->GUI.Window.SetCurrentListPosition  = V2::KodiAPI::GUI::CAddOnWindow::SetCurrentListPosition;
  interfaces->GUI.Window.GetCurrentListPosition  = V2::KodiAPI::GUI::CAddOnWindow::GetCurrentListPosition;
  interfaces->GUI.Window.SetControlLabel         = V2::KodiAPI::GUI::CAddOnWindow::SetControlLabel;
  interfaces->GUI.Window.MarkDirtyRegion         = V2::KodiAPI::GUI::CAddOnWindow::MarkDirtyRegion;

  interfaces->GUI.Window.GetControl_Button       = V2::KodiAPI::GUI::CAddOnWindow::GetControl_Button;
  interfaces->GUI.Window.GetControl_Edit         = V2::KodiAPI::GUI::CAddOnWindow::GetControl_Edit;
  interfaces->GUI.Window.GetControl_FadeLabel    = V2::KodiAPI::GUI::CAddOnWindow::GetControl_FadeLabel;
  interfaces->GUI.Window.GetControl_Image        = V2::KodiAPI::GUI::CAddOnWindow::GetControl_Image;
  interfaces->GUI.Window.GetControl_Label        = V2::KodiAPI::GUI::CAddOnWindow::GetControl_Label;
  interfaces->GUI.Window.GetControl_Spin         = V2::KodiAPI::GUI::CAddOnWindow::GetControl_Spin;
  interfaces->GUI.Window.GetControl_RadioButton  = V2::KodiAPI::GUI::CAddOnWindow::GetControl_RadioButton;
  interfaces->GUI.Window.GetControl_Progress     = V2::KodiAPI::GUI::CAddOnWindow::GetControl_Progress;
  interfaces->GUI.Window.GetControl_RenderAddon  = V2::KodiAPI::GUI::CAddOnWindow::GetControl_RenderAddon;
  interfaces->GUI.Window.GetControl_Slider       = V2::KodiAPI::GUI::CAddOnWindow::GetControl_Slider;
  interfaces->GUI.Window.GetControl_SettingsSlider= V2::KodiAPI::GUI::CAddOnWindow::GetControl_SettingsSlider;
  interfaces->GUI.Window.GetControl_TextBox      = V2::KodiAPI::GUI::CAddOnWindow::GetControl_TextBox;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V2 */
