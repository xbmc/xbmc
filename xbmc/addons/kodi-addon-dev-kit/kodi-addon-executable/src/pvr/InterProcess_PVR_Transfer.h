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

#include "kodi/api2/AddonLib.hpp"
#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_PVR_Transfer
  {
    void EpgEntry(const ADDON_HANDLE handle, const EPG_TAG* entry);
    void ChannelEntry(const ADDON_HANDLE handle, const PVR_CHANNEL* entry);
    void TimerEntry(const ADDON_HANDLE handle, const PVR_TIMER* entry);
    void RecordingEntry(const ADDON_HANDLE handle, const PVR_RECORDING* entry);
    void ChannelGroup(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP* entry);
    void ChannelGroupMember(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER* entry);
  };

}; /* extern "C" */
