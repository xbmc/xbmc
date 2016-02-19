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

#include "InterProcess_Player_InfoTagVideo.h"
#include "InterProcess.h"

extern "C"
{

bool CKODIAddon_InterProcess_Player_InfoTagVideo::AddonInfoTagVideo_GetFromPlayer(PLAYERHANDLE player, AddonInfoTagVideo* infoTag)
{
  return g_interProcess.m_Callbacks->AddonInfoTagVideo.GetFromPlayer(g_interProcess.m_Handle->addonData, player, infoTag);
}

void CKODIAddon_InterProcess_Player_InfoTagVideo::AddonInfoTagVideo_Release(AddonInfoTagVideo* infoTag)
{
  g_interProcess.m_Callbacks->AddonInfoTagVideo.Release(g_interProcess.m_Handle->addonData, infoTag);
}

}; /* extern "C" */
