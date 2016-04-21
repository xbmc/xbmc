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

class CFileItemList;

namespace V2
{
namespace KodiAPI
{

struct CB_AddOnLib;
struct VFSDirEntry;

namespace AddOn
{
extern "C"
{

  class CAddOnDirectory
  {
  public:
    static void Init(struct CB_AddOnLib *interfaces);
    /*\__________________________________________________________________________
    \*/
    static bool can_open_directory(
          void*                     hdl,
          const char*               strURL);

    static bool create_directory(
          void*                     hdl,
          const char*               strPath);

    static bool directory_exists(
          void*                     hdl,
          const char*               strPath);

    static bool remove_directory(
          void*                     hdl,
          const char*               strPath);
    /*\__________________________________________________________________________
    \*/
    static bool get_directory(
          void*                     hdl,
          const char*               strPath,
          const char*               mask,
          VFSDirEntry**             items,
          unsigned int*             num_items);

    static void free_directory(
          void*                     hdl,
          VFSDirEntry*              items,
          unsigned int              num_items);

  private:
    static void CFileItemListToVFSDirEntries(
          VFSDirEntry*              entries,
          unsigned int              num_entries,
          const CFileItemList&      items);
  };

} /* extern "C" */
} /* namespace AddOn */

} /* namespace KodiAPI */
} /* namespace V2 */
