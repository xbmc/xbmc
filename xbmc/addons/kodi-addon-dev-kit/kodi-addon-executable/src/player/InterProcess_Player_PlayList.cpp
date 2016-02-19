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
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <cstring>
#include <iostream>       // std::cerr
#include <stdexcept>      // std::out_of_range

using namespace P8PLATFORM;

extern "C"
{

PLAYERHANDLE CKODIAddon_InterProcess_Player_PlayList::PlayList_New(int playlist)
{
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_Delete(PLAYERHANDLE handle)
{
}

bool CKODIAddon_InterProcess_Player_PlayList::PlayList_LoadPlaylist(PLAYERHANDLE handle, const std::string& filename, int playlist)
{
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_AddItemURL(PLAYERHANDLE handle, const std::string& url, int index)
{
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_AddItemList(PLAYERHANDLE handle, const GUIHANDLE listitem, int index)
{
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_RemoveItem(PLAYERHANDLE handle, const std::string& url)
{
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_ClearList(PLAYERHANDLE handle)
{
}

int CKODIAddon_InterProcess_Player_PlayList::PlayList_GetListSize(PLAYERHANDLE handle)
{
}

int CKODIAddon_InterProcess_Player_PlayList::PlayList_GetListPosition(PLAYERHANDLE handle)
{
}

void CKODIAddon_InterProcess_Player_PlayList::PlayList_Shuffle(PLAYERHANDLE handle, bool shuffle)
{
}

void* CKODIAddon_InterProcess_Player_PlayList::PlayList_GetItem(PLAYERHANDLE handle, long position)
{
}

}; /* extern "C" */
