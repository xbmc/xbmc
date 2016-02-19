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

#include "InterProcess_Player_PlayList.h"
#include "InterProcess.h"

extern "C"
{

PLAYERHANDLE CKODIAddon_InterProcess_Player_PlayList::PlayList_New(int playlist)
{
  return g_interProcess.m_Callbacks->AddonPlayList.New(g_interProcess.m_Handle->addonData, playlist);
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_Delete(PLAYERHANDLE handle)
{
  g_interProcess.m_Callbacks->AddonPlayList.Delete(g_interProcess.m_Handle->addonData, handle);
}

bool CKODIAddon_InterProcess_Player_PlayList::PlayList_LoadPlaylist(PLAYERHANDLE handle, const std::string& filename, int playlist)
{
  return g_interProcess.m_Callbacks->AddonPlayList.LoadPlaylist(g_interProcess.m_Handle->addonData, handle, filename.c_str(), playlist);
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_AddItemURL(PLAYERHANDLE handle, const std::string& url, int index)
{
  g_interProcess.m_Callbacks->AddonPlayList.AddItemURL(g_interProcess.m_Handle->addonData, handle, url.c_str(), index);
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_AddItemList(PLAYERHANDLE handle, const GUIHANDLE listitem, int index)
{
  g_interProcess.m_Callbacks->AddonPlayList.AddItemList(g_interProcess.m_Handle->addonData, handle, listitem, index);
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_RemoveItem(PLAYERHANDLE handle, const std::string& url)
{
  g_interProcess.m_Callbacks->AddonPlayList.RemoveItem(g_interProcess.m_Handle->addonData, handle, url.c_str());
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_ClearList(PLAYERHANDLE handle)
{
  g_interProcess.m_Callbacks->AddonPlayList.ClearList(g_interProcess.m_Handle->addonData, handle);
}

int CKODIAddon_InterProcess_Player_PlayList::PlayList_GetListSize(PLAYERHANDLE handle)
{
  return g_interProcess.m_Callbacks->AddonPlayList.GetListSize(g_interProcess.m_Handle->addonData, handle);
}

int CKODIAddon_InterProcess_Player_PlayList::PlayList_GetListPosition(PLAYERHANDLE handle)
{
  return g_interProcess.m_Callbacks->AddonPlayList.GetListPosition(g_interProcess.m_Handle->addonData, handle);
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_Shuffle(PLAYERHANDLE handle, bool shuffle)
{
  g_interProcess.m_Callbacks->AddonPlayList.Shuffle(g_interProcess.m_Handle->addonData, handle, shuffle);
}

void* CKODIAddon_InterProcess_Player_PlayList::PlayList_GetItem(PLAYERHANDLE handle, long position)
{
  return g_interProcess.m_Callbacks->AddonPlayList.GetItem(g_interProcess.m_Handle->addonData, handle, position);
}

}; /* extern "C" */
