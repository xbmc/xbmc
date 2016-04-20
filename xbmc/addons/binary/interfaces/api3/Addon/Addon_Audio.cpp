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

#include "Addon_Audio.h"
#include "addons/binary/interfaces/api3/AddonInterfaceBase.h"
#include "addons/binary/interfaces/api2/Addon/Addon_Audio.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api3/.internal/AddonLib_internal.hpp"

namespace V3
{
namespace KodiAPI
{

namespace AddOn
{
extern "C"
{

void CAddOnAudio::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->Audio.soundplay_get_handle       = V2::KodiAPI::AddOn::CAddOnAudio::soundplay_get_handle;
  interfaces->Audio.soundplay_release_handle   = V2::KodiAPI::AddOn::CAddOnAudio::soundplay_release_handle;
  interfaces->Audio.soundplay_play             = V2::KodiAPI::AddOn::CAddOnAudio::soundplay_play;
  interfaces->Audio.soundplay_stop             = V2::KodiAPI::AddOn::CAddOnAudio::soundplay_stop;
  interfaces->Audio.soundplay_set_channel      = V2::KodiAPI::AddOn::CAddOnAudio::soundplay_set_channel;
  interfaces->Audio.soundplay_get_channel      = V2::KodiAPI::AddOn::CAddOnAudio::soundplay_get_channel;
  interfaces->Audio.soundplay_set_volume       = V2::KodiAPI::AddOn::CAddOnAudio::soundplay_set_volume;
  interfaces->Audio.soundplay_get_volume       = V2::KodiAPI::AddOn::CAddOnAudio::soundplay_get_volume;
}

} /* extern "C" */
} /* namespace AddOn */

} /* namespace KodiAPI */
} /* namespace V3 */
