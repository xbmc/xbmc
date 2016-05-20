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
#include KITINCLUDE(ADDON_API_LEVEL, gui/ListItem.hpp)
#include KITINCLUDE(ADDON_API_LEVEL, player/PlayList.hpp)

API_NAMESPACE

namespace KodiAPI
{

namespace Player
{

  CPlayList::CPlayList(AddonPlayListType playlist)
   : m_ControlHandle(nullptr),
     m_playlist(playlist)
  {
    m_ControlHandle = g_interProcess.m_Callbacks->AddonPlayList.New(g_interProcess.m_Handle, playlist);
    if (!m_ControlHandle)
      fprintf(stderr, "ERROR: CPlayList can't create control class from Kodi !!!\n");
  }

  CPlayList::~CPlayList()
  {
    g_interProcess.m_Callbacks->AddonPlayList.Delete(g_interProcess.m_Handle, m_ControlHandle);
  }

  AddonPlayListType CPlayList::GetPlayListType() const
  {
    return m_playlist;
  }

  bool CPlayList::LoadPlaylist(const std::string& filename)
  {
    return g_interProcess.m_Callbacks->AddonPlayList.LoadPlaylist(g_interProcess.m_Handle, m_ControlHandle, filename.c_str(), m_playlist);
  }

  void CPlayList::AddItem(const std::string& url, int index)
  {
    g_interProcess.m_Callbacks->AddonPlayList.AddItemURL(g_interProcess.m_Handle, m_ControlHandle, url.c_str(), index);
  }

  void CPlayList::AddItem(const API_NAMESPACE_NAME::KodiAPI::GUI::CListItem* listitem, int index)
  {
    g_interProcess.m_Callbacks->AddonPlayList.AddItemList(g_interProcess.m_Handle, m_ControlHandle, listitem->GetListItemHandle(), index);
  }

  void CPlayList::RemoveItem(const std::string& url)
  {
    g_interProcess.m_Callbacks->AddonPlayList.RemoveItem(g_interProcess.m_Handle, m_ControlHandle, url.c_str());
  }

  void CPlayList::ClearList()
  {
    g_interProcess.m_Callbacks->AddonPlayList.ClearList(g_interProcess.m_Handle, m_ControlHandle);
  }

  int CPlayList::GetListSize()
  {
    return g_interProcess.m_Callbacks->AddonPlayList.GetListSize(g_interProcess.m_Handle, m_ControlHandle);
  }

  int CPlayList::GetListPosition()
  {
    return g_interProcess.m_Callbacks->AddonPlayList.GetListPosition(g_interProcess.m_Handle, m_ControlHandle);
  }

  void CPlayList::Shuffle(bool shuffle)
  {
    g_interProcess.m_Callbacks->AddonPlayList.Shuffle(g_interProcess.m_Handle, m_ControlHandle, shuffle);
  }

  GUI::CListItem* CPlayList::operator [](long i)
  {
    return static_cast<GUI::CListItem*>(g_interProcess.m_Callbacks->AddonPlayList.GetItem(g_interProcess.m_Handle, m_ControlHandle, i));
  }

} /* namespace Player */
} /* namespace KodiAPI */

END_NAMESPACE()
