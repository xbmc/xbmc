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
#include KITINCLUDE(ADDON_API_LEVEL, pvr/General.hpp)

#include <string>
#include <stdarg.h>

API_NAMESPACE

namespace KodiAPI
{
namespace PVR
{
namespace General
{

  void AddMenuHook(PVR_MENUHOOK* hook)
  {
    g_interProcess.m_Callbacks->PVR.add_menu_hook(g_interProcess.m_Handle, hook);
  }

  void Recording(const std::string& strRecordingName, const std::string& strFileName, bool bOn)
  {
    g_interProcess.m_Callbacks->PVR.recording(g_interProcess.m_Handle, strRecordingName.c_str(), strFileName.c_str(), bOn);
  }

  void ConnectionStateChange(const std::string& strConnectionString, PVR_CONNECTION_STATE newState, const std::string& strMessage)
  {
    g_interProcess.m_Callbacks->PVR.connection_state_change(g_interProcess.m_Handle, strConnectionString.c_str(), newState, strMessage.c_str());
  }

  void EpgEventStateChange(EPG_TAG *tag, unsigned int iUniqueChannelId, EPG_EVENT_STATE newState)
  {
    g_interProcess.m_Callbacks->PVR.epg_event_state_change(g_interProcess.m_Handle, tag, iUniqueChannelId, newState);
  }

} /* namespace General */
} /* namespace PVR */
} /* namespace KodiAPI */

END_NAMESPACE()
