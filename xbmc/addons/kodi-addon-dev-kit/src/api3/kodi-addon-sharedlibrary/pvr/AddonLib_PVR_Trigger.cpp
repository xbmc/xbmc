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
#include KITINCLUDE(ADDON_API_LEVEL, pvr/Trigger.hpp)

#include <string>
#include <stdarg.h>

API_NAMESPACE

namespace KodiAPI
{
namespace PVR
{
namespace Trigger
{

  void TimerUpdate(void)
  {
    g_interProcess.m_Callbacks->PVR.trigger_timer_update(g_interProcess.m_Handle);
  }

  void RecordingUpdate(void)
  {
    g_interProcess.m_Callbacks->PVR.trigger_recording_update(g_interProcess.m_Handle);
  }

  void ChannelUpdate(void)
  {
    g_interProcess.m_Callbacks->PVR.trigger_channel_update(g_interProcess.m_Handle);
  }

  void EpgUpdate(unsigned int iChannelUid)
  {
    g_interProcess.m_Callbacks->PVR.trigger_epg_update(g_interProcess.m_Handle, iChannelUid);
  }

  void ChannelGroupsUpdate(void)
  {
    g_interProcess.m_Callbacks->PVR.trigger_channel_groups_update(g_interProcess.m_Handle);
  }

} /* namespace Trigger */
} /* namespace PVR */
} /* namespace KodiAPI */

END_NAMESPACE()
