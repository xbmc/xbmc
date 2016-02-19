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

#include "kodi/api2/.internal/AddonLib_internal.hpp"

#include <string>

extern "C"
{

  struct CKODIAddon_InterProcess_Player_PlayList
  {
    PLAYERHANDLE PlayList_New(int playlist);
    void PlayList_Delete(PLAYERHANDLE handle);
    bool PlayList_LoadPlaylist(PLAYERHANDLE handle, const std::string& filename, int playlist);
    void PlayList_AddItemURL(PLAYERHANDLE handle, const std::string& url, int index);
    void PlayList_AddItemList(PLAYERHANDLE handle, const GUIHANDLE listitem, int index);
    void PlayList_RemoveItem(PLAYERHANDLE handle, const std::string& url);
    void PlayList_ClearList(PLAYERHANDLE handle);
    int PlayList_GetListSize(PLAYERHANDLE handle);
    int PlayList_GetListPosition(PLAYERHANDLE handle);
    void PlayList_Shuffle(PLAYERHANDLE handle, bool shuffle);
    void* PlayList_GetItem(PLAYERHANDLE handle, long position);
  };

}; /* extern "C" */
