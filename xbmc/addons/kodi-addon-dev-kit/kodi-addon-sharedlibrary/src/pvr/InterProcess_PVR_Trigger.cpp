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

#include "InterProcess_PVR_Trigger.h"
#include "InterProcess.h"

extern "C"
{

void CKODIAddon_InterProcess_PVR_Trigger::TriggerTimerUpdate(void)
{
  g_interProcess.m_Callbacks->PVR.trigger_timer_update(g_interProcess.m_Handle);
}

void CKODIAddon_InterProcess_PVR_Trigger::TriggerRecordingUpdate(void)
{
  g_interProcess.m_Callbacks->PVR.trigger_recording_update(g_interProcess.m_Handle);
}

void CKODIAddon_InterProcess_PVR_Trigger::TriggerChannelUpdate(void)
{
  g_interProcess.m_Callbacks->PVR.trigger_channel_update(g_interProcess.m_Handle);
}

void CKODIAddon_InterProcess_PVR_Trigger::TriggerEpgUpdate(unsigned int iChannelUid)
{
  g_interProcess.m_Callbacks->PVR.trigger_epg_update(g_interProcess.m_Handle, iChannelUid);
}

void CKODIAddon_InterProcess_PVR_Trigger::TriggerChannelGroupsUpdate(void)
{
  g_interProcess.m_Callbacks->PVR.trigger_channel_groups_update(g_interProcess.m_Handle);
}

}; /* extern "C" */
