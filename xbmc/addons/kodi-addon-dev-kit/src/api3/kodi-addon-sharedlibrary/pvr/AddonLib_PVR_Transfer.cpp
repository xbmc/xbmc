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

#include "InterProcess.h"
#include KITINCLUDE(ADDON_API_LEVEL, pvr/Transfer.hpp)

#include <string>
#include <stdarg.h>

API_NAMESPACE

namespace KodiAPI
{
namespace PVR
{
namespace Transfer
{

  void EpgEntry(const ADDON_HANDLE handle, const EPG_TAG* entry)
  {
    g_interProcess.m_Callbacks->PVR.transfer_epg_entry(g_interProcess.m_Handle, handle, entry);
  }

  void ChannelEntry(const ADDON_HANDLE handle, const PVR_CHANNEL* entry)
  {
    g_interProcess.m_Callbacks->PVR.transfer_channel_entry(g_interProcess.m_Handle, handle, entry);
  }

  void TimerEntry(const ADDON_HANDLE handle, const PVR_TIMER* entry)
  {
    g_interProcess.m_Callbacks->PVR.transfer_timer_entry(g_interProcess.m_Handle, handle, entry);
  }

  void RecordingEntry(const ADDON_HANDLE handle, const PVR_RECORDING* entry)
  {
    g_interProcess.m_Callbacks->PVR.transfer_recording_entry(g_interProcess.m_Handle, handle, entry);
  }

  void ChannelGroup(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP* entry)
  {
    g_interProcess.m_Callbacks->PVR.transfer_channel_group(g_interProcess.m_Handle, handle, entry);
  }

  void ChannelGroupMember(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER* entry)
  {
    g_interProcess.m_Callbacks->PVR.transfer_channel_group_member(g_interProcess.m_Handle, handle, entry);
  }

} /* namespace Transfer */
} /* namespace PVR */
} /* namespace KodiAPI */

END_NAMESPACE()
