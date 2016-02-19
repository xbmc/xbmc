/*
 *      Copyright (C) 2015 Team KODI
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

#include "AddonCB_Directory.h"

#include "FileItem.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/callbacks/api2/AddonCallbacksBase.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"

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

CAddOnDirectory::CAddOnDirectory()
{

}

void CAddOnDirectory::Init(V2::KodiAPI::CB_AddOnLib *callbacks)
{
  callbacks->Directory.can_open_directory    = CAddOnDirectory::can_open_directory;
  callbacks->Directory.create_directory      = CAddOnDirectory::create_directory;
  callbacks->Directory.directory_exists      = CAddOnDirectory::directory_exists;
  callbacks->Directory.remove_directory      = CAddOnDirectory::remove_directory;
  callbacks->VFS.get_vfs_directory           = CAddOnDirectory::get_vfs_directory;
  callbacks->VFS.free_vfs_directory          = CAddOnDirectory::free_vfs_directory;
}

/*\_____________________________________________________________________________
| |
| |
| |_____________________________________________________________________________
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
/*\____________________________________________________________________________/
\*/

/*\_____________________________________________________________________________
| |
| | C++ wrappers for Kodi's VFS operations
| |_____________________________________________________________________________
\*/
bool CAddOnDirectory::get_vfs_directory(
        void*                     hdl,
        const char*               strPath,
        const char*               mask,
        V2::KodiAPI::VFSDirEntry**             items,
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
      *items = (V2::KodiAPI::VFSDirEntry*)malloc(fileItems.Size()*sizeof(V2::KodiAPI::VFSDirEntry));

      CFileItemListToVFSDirEntries(*items, fileItems);
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

void CAddOnDirectory::free_vfs_directory(
        void*                     hdl,
        V2::KodiAPI::VFSDirEntry*              items,
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
        V2::KodiAPI::VFSDirEntry*              entries,
        const CFileItemList&      items)
{
  for (int i = 0; i < items.Size(); ++i)
  {
    entries[i].label      = strdup(items[i]->GetLabel().c_str());
    entries[i].path       = strdup(items[i]->GetPath().c_str());
    entries[i].size       = items[i]->m_dwSize;
    entries[i].folder     = items[i]->m_bIsFolder;
  }
}

/*\____________________________________________________________________________/
\*/

}; /* extern "C" */
}; /* namespace AddOn */

}; /* namespace KodiAPI */
}; /* namespace V2 */
