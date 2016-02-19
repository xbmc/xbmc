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

#include "InterProcess_Player_InfoTagMusic.h"
#include "InterProcess.h"

extern "C"
{

bool CKODIAddon_InterProcess_Player_InfoTagMusic::AddonInfoTagMusic_GetFromPlayer(PLAYERHANDLE player, AddonInfoTagMusic* infoTag)
{
  return g_interProcess.m_Callbacks->AddonInfoTagMusic.GetFromPlayer(g_interProcess.m_Handle->addonData, player, infoTag);
}

void CKODIAddon_InterProcess_Player_InfoTagMusic::AddonInfoTagMusic_Release(AddonInfoTagMusic* infoTag)
{
  g_interProcess.m_Callbacks->AddonInfoTagMusic.Release(g_interProcess.m_Handle->addonData, infoTag);
}

}; /* extern "C" */
