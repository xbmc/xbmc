#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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

class CFileItem;

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;

namespace Player
{
extern "C"
{

  class CAddOnPlayList
  {
  public:
    static void Init(struct CB_AddOnLib *interfaces);

    static void* New(void* hdl, int playList);
    static void Delete(void* hdl, void* handle);

    static bool LoadPlaylist(void* hdl, void* handle, const char* filename, int playList);
    static void AddItemURL(void* hdl, void* handle, const char* url, int index);
    static void AddItemList(void* hdl, void* handle, const void* listitem, int index);
    static void RemoveItem(void* hdl, void* handle, const char* url);
    static void ClearList(void* hdl, void* handle);
    static int GetListSize(void* hdl, void* handle);
    static int GetListPosition(void* hdl, void* handle);
    static void Shuffle(void* hdl, void* handle, bool shuffle);
  };

} /* extern "C" */
} /* namespace Player */

} /* namespace KodiAPI */
} /* namespace V2 */
