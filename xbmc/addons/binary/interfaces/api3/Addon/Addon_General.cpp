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

#include "Addon_General.h"
#include "addons/binary/interfaces/api2/Addon/Addon_General.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace AddOn
{
extern "C"
{

void CAddOnGeneral::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->General.get_addon_info                 = V2::KodiAPI::AddOn::CAddOnGeneral::get_addon_info;
  interfaces->General.get_setting                    = V2::KodiAPI::AddOn::CAddOnGeneral::get_setting;
  interfaces->General.open_settings_dialog           = V2::KodiAPI::AddOn::CAddOnGeneral::open_settings_dialog;
  interfaces->General.queue_notification             = V2::KodiAPI::AddOn::CAddOnGeneral::queue_notification;
  interfaces->General.queue_notification_from_type   = V2::KodiAPI::AddOn::CAddOnGeneral::queue_notification_from_type;
  interfaces->General.queue_notification_with_image  = V2::KodiAPI::AddOn::CAddOnGeneral::queue_notification_with_image;
  interfaces->General.get_md5                        = V2::KodiAPI::AddOn::CAddOnGeneral::get_md5;
  interfaces->General.unknown_to_utf8                = V2::KodiAPI::AddOn::CAddOnGeneral::unknown_to_utf8;
  interfaces->General.get_localized_string           = V2::KodiAPI::AddOn::CAddOnGeneral::get_localized_string;
  interfaces->General.get_language                   = V2::KodiAPI::AddOn::CAddOnGeneral::get_language;
  interfaces->General.get_dvd_menu_language          = V2::KodiAPI::AddOn::CAddOnGeneral::get_dvd_menu_language;
  interfaces->General.start_server                   = V2::KodiAPI::AddOn::CAddOnGeneral::start_server;
  interfaces->General.audio_suspend                  = V2::KodiAPI::AddOn::CAddOnGeneral::audio_suspend;
  interfaces->General.audio_resume                   = V2::KodiAPI::AddOn::CAddOnGeneral::audio_resume;
  interfaces->General.get_volume                     = V2::KodiAPI::AddOn::CAddOnGeneral::get_volume;
  interfaces->General.set_volume                     = V2::KodiAPI::AddOn::CAddOnGeneral::set_volume;
  interfaces->General.is_muted                       = V2::KodiAPI::AddOn::CAddOnGeneral::is_muted;
  interfaces->General.toggle_mute                    = V2::KodiAPI::AddOn::CAddOnGeneral::toggle_mute;
  interfaces->General.get_optical_state              = V2::KodiAPI::AddOn::CAddOnGeneral::get_optical_state;
  interfaces->General.eject_optical_drive            = V2::KodiAPI::AddOn::CAddOnGeneral::eject_optical_drive;
  interfaces->General.kodi_version                   = V2::KodiAPI::AddOn::CAddOnGeneral::kodi_version;
  interfaces->General.kodi_quit                      = V2::KodiAPI::AddOn::CAddOnGeneral::kodi_quit;
  interfaces->General.htpc_shutdown                  = V2::KodiAPI::AddOn::CAddOnGeneral::htpc_shutdown;
  interfaces->General.htpc_restart                   = V2::KodiAPI::AddOn::CAddOnGeneral::htpc_restart;
  interfaces->General.execute_script                 = V2::KodiAPI::AddOn::CAddOnGeneral::execute_script;
  interfaces->General.execute_builtin                = V2::KodiAPI::AddOn::CAddOnGeneral::execute_builtin;
  interfaces->General.execute_jsonrpc                = V2::KodiAPI::AddOn::CAddOnGeneral::execute_jsonrpc;
  interfaces->General.get_region                     = V2::KodiAPI::AddOn::CAddOnGeneral::get_region;
  interfaces->General.get_free_mem                   = V2::KodiAPI::AddOn::CAddOnGeneral::get_free_mem;
  interfaces->General.get_global_idle_time           = V2::KodiAPI::AddOn::CAddOnGeneral::get_global_idle_time;
  interfaces->General.translate_path                 = V2::KodiAPI::AddOn::CAddOnGeneral::translate_path;
}

} /* extern "C" */
} /* namespace AddOn */

} /* namespace KodiAPI */
} /* namespace V3 */
