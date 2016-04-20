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

#include "Addon_Directory.h"

#include "FileItem.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "filesystem/File.h"
#include "filesystem/Directory.h"

#include <algorithm>

using namespace ADDON;
using namespace XFILE;

namespace V2
{
namespace KodiAPI
{

namespace AddOn
{
extern "C"
{

void CAddOnDirectory::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->Directory.can_open_directory    = V2::KodiAPI::AddOn::CAddOnDirectory::can_open_directory;
  interfaces->Directory.create_directory      = V2::KodiAPI::AddOn::CAddOnDirectory::create_directory;
  interfaces->Directory.directory_exists      = V2::KodiAPI::AddOn::CAddOnDirectory::directory_exists;
  interfaces->Directory.remove_directory      = V2::KodiAPI::AddOn::CAddOnDirectory::remove_directory;
  interfaces->VFS.get_directory               = V2::KodiAPI::AddOn::CAddOnDirectory::get_directory;
  interfaces->VFS.free_directory              = V2::KodiAPI::AddOn::CAddOnDirectory::free_directory;
}

/*\_____________________________________________________________________________
\*/
bool CAddOnDirectory::can_open_directory(
        void*                     hdl,
        const char*               strURL)
{
  try
  {
    if (!hdl || !strURL)
      throw ADDON::WrongValueException("CAddOnDirectory - %s - invalid data (handle='%p', strURL='%p')",
                                        __FUNCTION__, hdl, strURL);
    CFileItemList items;
    return CDirectory::GetDirectory(strURL, items);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDirectory::create_directory(
        void*                     hdl,
        const char*               strPath)
{
  try
  {
    if (!hdl || !strPath)
      throw ADDON::WrongValueException("CAddOnDirectory - %s - invalid data (handle='%p', strPath='%p')",
                                        __FUNCTION__, hdl, strPath);
    return CDirectory::Create(strPath);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDirectory::directory_exists(
        void*                     hdl,
        const char*               strPath)
{
  try
  {
    if (!hdl || !strPath)
      throw ADDON::WrongValueException("CAddOnDirectory - %s - invalid data (handle='%p', strPath='%p')",
                                        __FUNCTION__, hdl, strPath);
    return CDirectory::Exists(strPath);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnDirectory::remove_directory(
        void*                     hdl,
        const char*               strPath)
{
  try
  {
    if (!hdl || !strPath)
      throw ADDON::WrongValueException("CAddOnDirectory - %s - invalid data (handle='%p', strPath='%p')",
                                        __FUNCTION__, hdl, strPath);
    // Empty directory
    CFileItemList fileItems;
    CDirectory::GetDirectory(strPath, fileItems);
    for (int i = 0; i < fileItems.Size(); ++i)
      CFile::Delete(fileItems.Get(i)->GetPath());

    return CDirectory::Remove(strPath);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

/*\_____________________________________________________________________________
\*/

bool CAddOnDirectory::get_directory(
        void*                     hdl,
        const char*               strPath,
        const char*               mask,
        VFSDirEntry**             items,
        unsigned int*             num_items)
{
  try
  {
    if (!hdl || !strPath)
      throw ADDON::WrongValueException("CAddOnDirectory - %s - invalid data (handle='%p', strPath='%p')",
                                        __FUNCTION__, hdl, strPath);
    CFileItemList fileItems;
    if (!CDirectory::GetDirectory(strPath, fileItems, mask, DIR_FLAG_NO_FILE_DIRS))
      return false;

    if (fileItems.Size() > 0)
    {
      *num_items = static_cast<unsigned int>(fileItems.Size());
      *items = (VFSDirEntry*)malloc(fileItems.Size()*sizeof(VFSDirEntry));

      CFileItemListToVFSDirEntries(*items, *num_items, fileItems);
    }
    else
    {
      *num_items = 0;
      *items = nullptr;
    }

    return true;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnDirectory::free_directory(
        void*                     hdl,
        VFSDirEntry*              items,
        unsigned int              num_items)
{
  try
  {
    if (!hdl || !items)
      throw ADDON::WrongValueException("CAddOnDirectory - %s - invalid data (handle='%p', items='%p')",
                                        __FUNCTION__, hdl, items);
    for (unsigned int i = 0; i < num_items; ++i)
    {
      free(items[i].label);
      free(items[i].path);
    }
    free(items);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnDirectory::CFileItemListToVFSDirEntries(
        VFSDirEntry*              entries,
        unsigned int              num_entries,
        const CFileItemList&      items)
{
  if (!entries)
    return;

  int toCopy = std::min(num_entries, (unsigned int)items.Size());

  for (int i = 0; i < toCopy; ++i)
  {
    entries[i].label      = strdup(items[i]->GetLabel().c_str());
    entries[i].path       = strdup(items[i]->GetPath().c_str());
    entries[i].size       = items[i]->m_dwSize;
    entries[i].folder     = items[i]->m_bIsFolder;
  }
}

} /* extern "C" */
} /* namespace AddOn */

} /* namespace KodiAPI */
} /* namespace V2 */
