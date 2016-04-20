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

#include "Addon_GUIListItem.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/GUI/Addon_GUIListItem.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace GUI
{
extern "C"
{

void CAddOnListItem::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->GUI.ListItem.Create                = V2::KodiAPI::GUI::CAddOnListItem::Create;
  interfaces->GUI.ListItem.Destroy               = V2::KodiAPI::GUI::CAddOnListItem::Destroy;
  interfaces->GUI.ListItem.GetLabel              = V2::KodiAPI::GUI::CAddOnListItem::GetLabel;
  interfaces->GUI.ListItem.SetLabel              = V2::KodiAPI::GUI::CAddOnListItem::SetLabel;
  interfaces->GUI.ListItem.GetLabel2             = V2::KodiAPI::GUI::CAddOnListItem::GetLabel2;
  interfaces->GUI.ListItem.SetLabel2             = V2::KodiAPI::GUI::CAddOnListItem::SetLabel2;
  interfaces->GUI.ListItem.GetIconImage          = V2::KodiAPI::GUI::CAddOnListItem::GetIconImage;
  interfaces->GUI.ListItem.SetIconImage          = V2::KodiAPI::GUI::CAddOnListItem::SetIconImage;
  interfaces->GUI.ListItem.GetOverlayImage       = V2::KodiAPI::GUI::CAddOnListItem::GetOverlayImage;
  interfaces->GUI.ListItem.SetOverlayImage       = V2::KodiAPI::GUI::CAddOnListItem::SetOverlayImage;
  interfaces->GUI.ListItem.SetThumbnailImage     = V2::KodiAPI::GUI::CAddOnListItem::SetThumbnailImage;
  interfaces->GUI.ListItem.SetArt                = V2::KodiAPI::GUI::CAddOnListItem::SetArt;
  interfaces->GUI.ListItem.GetArt                = V2::KodiAPI::GUI::CAddOnListItem::GetArt;
  interfaces->GUI.ListItem.SetArtFallback        = V2::KodiAPI::GUI::CAddOnListItem::SetArtFallback;
  interfaces->GUI.ListItem.HasArt                = V2::KodiAPI::GUI::CAddOnListItem::HasArt;
  interfaces->GUI.ListItem.Select                = V2::KodiAPI::GUI::CAddOnListItem::Select;
  interfaces->GUI.ListItem.IsSelected            = V2::KodiAPI::GUI::CAddOnListItem::IsSelected;
  interfaces->GUI.ListItem.HasIcon               = V2::KodiAPI::GUI::CAddOnListItem::HasIcon;
  interfaces->GUI.ListItem.HasOverlay            = V2::KodiAPI::GUI::CAddOnListItem::HasOverlay;
  interfaces->GUI.ListItem.IsFileItem            = V2::KodiAPI::GUI::CAddOnListItem::IsFileItem;
  interfaces->GUI.ListItem.IsFolder              = V2::KodiAPI::GUI::CAddOnListItem::IsFolder;
  interfaces->GUI.ListItem.SetProperty           = V2::KodiAPI::GUI::CAddOnListItem::SetProperty;
  interfaces->GUI.ListItem.GetProperty           = V2::KodiAPI::GUI::CAddOnListItem::GetProperty;
  interfaces->GUI.ListItem.ClearProperty         = V2::KodiAPI::GUI::CAddOnListItem::ClearProperty;
  interfaces->GUI.ListItem.ClearProperties       = V2::KodiAPI::GUI::CAddOnListItem::ClearProperties;
  interfaces->GUI.ListItem.HasProperties         = V2::KodiAPI::GUI::CAddOnListItem::HasProperties;
  interfaces->GUI.ListItem.HasProperty           = V2::KodiAPI::GUI::CAddOnListItem::HasProperty;
  interfaces->GUI.ListItem.SetPath               = V2::KodiAPI::GUI::CAddOnListItem::SetPath;
  interfaces->GUI.ListItem.GetPath               = V2::KodiAPI::GUI::CAddOnListItem::GetPath;
  interfaces->GUI.ListItem.GetDuration           = V2::KodiAPI::GUI::CAddOnListItem::GetDuration;
  interfaces->GUI.ListItem.SetSubtitles          = V2::KodiAPI::GUI::CAddOnListItem::SetSubtitles;
  interfaces->GUI.ListItem.SetMimeType           = V2::KodiAPI::GUI::CAddOnListItem::SetMimeType;
  interfaces->GUI.ListItem.SetContentLookup      = V2::KodiAPI::GUI::CAddOnListItem::SetContentLookup;
  interfaces->GUI.ListItem.AddContextMenuItems   = V2::KodiAPI::GUI::CAddOnListItem::AddContextMenuItems;
  interfaces->GUI.ListItem.AddStreamInfo         = V2::KodiAPI::GUI::CAddOnListItem::AddStreamInfo;
}

} /* extern "C" */
} /* namespace GUI */

} /* namespace KodiAPI */
} /* namespace V3 */
