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
#include "kodi/api2/gui/ListItem.hpp"
#include "kodi/api2/player/PlayList.hpp"

namespace V2
{
namespace KodiAPI
{

namespace Player
{

  CPlayList::CPlayList(AddonPlayListType playlist)
   : m_ControlHandle(nullptr),
     m_playlist(playlist)
  {
    m_ControlHandle = g_interProcess.PlayList_New(m_playlist);
    if (!m_ControlHandle)
      fprintf(stderr, "ERROR: CPlayList can't create control class from Kodi !!!\n");
  }

  CPlayList::~CPlayList()
  {
    g_interProcess.PlayList_Delete(m_ControlHandle);
  }

  AddonPlayListType CPlayList::GetPlayListType() const
  {
    return m_playlist;
  }

  bool CPlayList::LoadPlaylist(const std::string& filename)
  {
    return g_interProcess.PlayList_LoadPlaylist(m_ControlHandle, filename, m_playlist);
  }

  void CPlayList::AddItem(const std::string& url, int index)
  {
    g_interProcess.PlayList_AddItemURL(m_ControlHandle, url, index);
  }

  void CPlayList::AddItem(const V2::KodiAPI::GUI::CListItem* listitem, int index)
  {
    g_interProcess.PlayList_AddItemList(m_ControlHandle, listitem->GetListItemHandle(), index);
  }

  void CPlayList::RemoveItem(const std::string& url)
  {
    g_interProcess.PlayList_RemoveItem(m_ControlHandle, url);
  }

  void CPlayList::ClearList()
  {
    g_interProcess.PlayList_ClearList(m_ControlHandle);
  }

  int CPlayList::GetListSize()
  {
    return g_interProcess.PlayList_GetListSize(m_ControlHandle);
  }

  int CPlayList::GetListPosition()
  {
    return g_interProcess.PlayList_GetListPosition(m_ControlHandle);
  }

  void CPlayList::Shuffle(bool shuffle)
  {
    g_interProcess.PlayList_Shuffle(m_ControlHandle, shuffle);
  }

  GUI::CListItem* CPlayList::operator [](long i)
  {
    return static_cast<GUI::CListItem*>(g_interProcess.PlayList_GetItem(m_ControlHandle, i));
  }

}; /* namespace Player */

}; /* namespace KodiAPI */
}; /* namespace V2 */
