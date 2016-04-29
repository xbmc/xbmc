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

#include "Addon_PlayList.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/Player/Addon_PlayList.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace Player
{
extern "C"
{

void CAddOnPlayList::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->AddonPlayList.New                          = V2::KodiAPI::Player::CAddOnPlayList::New;
  interfaces->AddonPlayList.Delete                       = V2::KodiAPI::Player::CAddOnPlayList::Delete;
  interfaces->AddonPlayList.LoadPlaylist                 = V2::KodiAPI::Player::CAddOnPlayList::LoadPlaylist;
  interfaces->AddonPlayList.AddItemURL                   = V2::KodiAPI::Player::CAddOnPlayList::AddItemURL;
  interfaces->AddonPlayList.AddItemList                  = V2::KodiAPI::Player::CAddOnPlayList::AddItemList;
  interfaces->AddonPlayList.RemoveItem                   = V2::KodiAPI::Player::CAddOnPlayList::RemoveItem;
  interfaces->AddonPlayList.ClearList                    = V2::KodiAPI::Player::CAddOnPlayList::ClearList;
  interfaces->AddonPlayList.GetListSize                  = V2::KodiAPI::Player::CAddOnPlayList::GetListSize;
  interfaces->AddonPlayList.GetListPosition              = V2::KodiAPI::Player::CAddOnPlayList::GetListPosition;
  interfaces->AddonPlayList.Shuffle                      = V2::KodiAPI::Player::CAddOnPlayList::Shuffle;
}

} /* extern "C" */
} /* namespace Player */

} /* namespace KodiAPI */
} /* namespace V3 */
