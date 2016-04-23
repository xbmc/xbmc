/*
 *      Copyright (C) 2012-2016 Team KODI
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

#include "Addon_PVR.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/PVR/Addon_PVR.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace PVR
{
extern "C"
{

void CAddonInterfacesPVR::Init(struct CB_AddOnLib *interfaces)
{
  /* write Kodi PVR specific add-on function addresses to callback table */
  interfaces->PVR.add_menu_hook                  = V2::KodiAPI::PVR::CAddonInterfacesPVR::add_menu_hook;
  interfaces->PVR.recording                      = V2::KodiAPI::PVR::CAddonInterfacesPVR::recording;
  interfaces->PVR.connection_state_change        = V2::KodiAPI::PVR::CAddonInterfacesPVR::connection_state_change;
  interfaces->PVR.epg_event_state_change         = V2::KodiAPI::PVR::CAddonInterfacesPVR::epg_event_state_change;

  interfaces->PVR.transfer_epg_entry             = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_epg_entry;
  interfaces->PVR.transfer_channel_entry         = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_channel_entry;
  interfaces->PVR.transfer_channel_group         = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_channel_group;
  interfaces->PVR.transfer_channel_group_member  = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_channel_group_member;
  interfaces->PVR.transfer_timer_entry           = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_timer_entry;
  interfaces->PVR.transfer_recording_entry       = V2::KodiAPI::PVR::CAddonInterfacesPVR::transfer_recording_entry;

  interfaces->PVR.trigger_channel_update         = V2::KodiAPI::PVR::CAddonInterfacesPVR::trigger_channel_update;
  interfaces->PVR.trigger_channel_groups_update  = V2::KodiAPI::PVR::CAddonInterfacesPVR::trigger_channel_groups_update;
  interfaces->PVR.trigger_timer_update           = V2::KodiAPI::PVR::CAddonInterfacesPVR::trigger_timer_update;
  interfaces->PVR.trigger_recording_update       = V2::KodiAPI::PVR::CAddonInterfacesPVR::trigger_recording_update;
  interfaces->PVR.trigger_epg_update             = V2::KodiAPI::PVR::CAddonInterfacesPVR::trigger_epg_update;
}

} /* extern "C" */
} /* namespace PVR */

} /* namespace KodiAPI */
} /* namespace V3 */
