/*
 *      Copyright (C) 2013 Arne Morten Kvarving
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <stdint.h>
#include "xbmc_addon_dll.h"
#include "kodi_vfs_types.h"

extern "C"
{
  //! \copydoc KodiToAddonFuncTable_VFSEntry::Open
  void* Open(VFSURL* url);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::OpenForWrite
  void* OpenForWrite(VFSURL* url, bool bOverWrite);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::Read
  ssize_t Read(void* context, void* buffer, size_t size);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::Write
  ssize_t Write(void* context, const void* buffer, size_t size);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::Seek
  int64_t Seek(void* context, int64_t position, int whence);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::Truncate
  int Truncate(void* context, int64_t size);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::GetLength
  int64_t GetLength(void* context);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::GetPosition
  int64_t GetPosition(void* context);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::GetChunkSize
  int GetChunkSize(void* context);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::IoControl
  int IoControl(void* context, XFILE::EIoControl request, void* param);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::Stat
  int Stat(VFSURL* url, struct __stat64* buffer);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::Close
  bool Close(void* context);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::Exists
  bool Exists(VFSURL* url);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::ContainsFiles
  void* ContainsFiles(VFSURL* url, VFSDirEntry** entries, int* num_entries,
                      char* rootpath);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::ClearOutIdle
  void ClearOutIdle();

  //! \copydoc KodiToAddonFuncTable_VFSEntry::DisconnectAll
  void DisconnectAll();

  //! \copydoc KodiToAddonFuncTable_VFSEntry::DirectoryExists
  bool DirectoryExists(VFSURL* url);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::RemoveDirectory
  bool RemoveDirectory(VFSURL* url);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::CreateDirectory
  bool CreateDirectory(VFSURL* url);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::GetDirectory
  void* GetDirectory(VFSURL* url, VFSDirEntry** entries, int* num_entries,
                     VFSCallbacks* callbacks);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::FreeDirectory
  void FreeDirectory(void* ctx);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::Delete
  bool Delete(VFSURL* url);

  //! \copydoc KodiToAddonFuncTable_VFSEntry::Rename
  bool Rename(VFSURL* url, VFSURL* url2);

  //! \brief Function to export the above structure to Kodi
  void __declspec(dllexport) get_addon(void* ptr)
  {
    AddonInstance_VFSEntry* vfs = static_cast<AddonInstance_VFSEntry*>(ptr);
    vfs->toAddon.Open = Open;
    vfs->toAddon.OpenForWrite = OpenForWrite;
    vfs->toAddon.Read = Read;
    vfs->toAddon.Write = Write;
    vfs->toAddon.Seek = Seek;
    vfs->toAddon.GetLength = GetLength;
    vfs->toAddon.GetPosition = GetPosition;
    vfs->toAddon.IoControl = IoControl;
    vfs->toAddon.Stat = Stat;
    vfs->toAddon.Close = Close;
    vfs->toAddon.Exists = Exists;
    vfs->toAddon.ClearOutIdle = ClearOutIdle;
    vfs->toAddon.DisconnectAll = DisconnectAll;
    vfs->toAddon.DirectoryExists = DirectoryExists;
    vfs->toAddon.GetDirectory = GetDirectory;
    vfs->toAddon.FreeDirectory = FreeDirectory;
    vfs->toAddon.Truncate = Truncate;
    vfs->toAddon.Delete = Delete;
    vfs->toAddon.Rename = Rename;
    vfs->toAddon.RemoveDirectory = RemoveDirectory;
    vfs->toAddon.CreateDirectory = CreateDirectory;
    vfs->toAddon.ContainsFiles = ContainsFiles;
    vfs->toAddon.GetChunkSize = GetChunkSize;
  };
};
