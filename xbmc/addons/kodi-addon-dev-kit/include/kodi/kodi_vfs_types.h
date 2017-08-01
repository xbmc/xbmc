/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#if !defined(TARGET_WINDOWS) && !defined(TARGET_WIN10)
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __declspec
#define __declspec(X)
#endif
#endif

#include <stdint.h>
#include "xbmc_addon_types.h"
#ifdef BUILD_KODI_ADDON
#include "IFileTypes.h"
#else
#include "filesystem/IFileTypes.h"
#include "PlatformDefs.h"
#endif

extern "C"
{

  struct VFSProperty
  {
    char* name;
    char* val;
  };

  struct VFSDirEntry
  {
    char* label;             //!< item label
    char* title;             //!< item title
    char* path;              //!< item path
    int num_props;           //!< Number of properties attached to item
    VFSProperty* properties; //!< Properties
    //FILETIME mtime;          //!< Mtime for file represented by item
    bool folder;             //!< Item is a folder
    uint64_t size;           //!< Size of file represented by item
  };

  struct VFSURL
  {
    const char* url;
    const char* domain;
    const char* hostname;
    const char* filename;
    unsigned int port;
    const char* options;
    const char* username;
    const char* password;
    const char* redacted;
    const char* sharename;
  };

  struct VFSCallbacks
  {
    //! \brief Require keyboard input
    //! \param heading The heading of the keyboard dialog
    //! \param input A pointer to the resulting string. Must be free'd by caller.
    //! \return True if input was received, false otherwise
    bool (__cdecl* GetKeyboardInput)(void* ctx, const char* heading, char** input);

    //! \brief Display an error dialog
    //! \param heading The heading of the error dialog
    //! \param line1 The first line of the error dialog
    //! \param line2 The second line of the error dialog. Can be NULL
    //! \param line3 The third line of the error dialog. Can be NULL
    void (__cdecl* SetErrorDialog)(void* ctx, const char* heading, const char* line1, const char* line2, const char* line3);

    //! \brief Prompt the user for authentication of a URL
    //! \param url The URL
    void (__cdecl* RequireAuthentication)(void* ctx, const char* url);

    //! \brief The context to be passed to the callbacks
    void* ctx;
  };

  typedef struct AddonProps_VFSEntry
  {
    int dummy;
  } AddonProps_VFSEntry;

  typedef AddonProps_VFSEntry VFS_PROPS;

  typedef struct AddonToKodiFuncTable_VFSEntry
  {
    KODI_HANDLE kodiInstance;
  } AddonToKodiFuncTable_VFSEntry;

  typedef struct KodiToAddonFuncTable_VFSEntry
  {
    //! \brief Open a file for input
    //! \param url The URL of the file
    //! \return Context for the opened file
    //! \sa IFile::Open
    void* (__cdecl* Open) (VFSURL* url);

    //! \brief Open a file for output
    //! \param url The URL of the file
    //! \param bOverwrite Whether or not to overwrite an existing file
    //! \return Context for the opened file
    //! \sa IFile::OpenForWrite
    void* (__cdecl* OpenForWrite) (VFSURL* url, bool bOverWrite);

    //! \brief Read from a file
    //! \param context The context of the file
    //! \param buffer The buffer to read data into
    //! \param uiBufSize Number of bytes to read
    //! \return Number of bytes read
    //! \sa IFile::Read
    ssize_t (__cdecl* Read) (void* context, void* buffer, size_t uiBufSize);

    //! \brief Write to a file
    //! \param context The context of the file
    //! \param buffer The buffer to read data from
    //! \param uiBufSize Number of bytes to write
    //! \return Number of bytes written
    //! \sa IFile::Write
    ssize_t (__cdecl* Write) (void* context, const void* buffer, size_t uiBufSize);

    //! \brief Seek in a file
    //! \param context The context of the file
    //! \param position The position to seek to
    //! \param whence Position in file 'position' is relative to (SEEK_CUR, SEEK_SET, SEEK_END)
    //! \return Offset in file after seek
    //! \sa IFile::Seek
    int64_t  (__cdecl* Seek) (void* context, int64_t position, int whence);

    //! \brief Truncate a file
    //! \param context The context of the file
    //! \param size The size to truncate the file to
    //! \return 0 on success, -1 on error
    //! \sa IFile::Truncate
    int      (__cdecl* Truncate) (void* context, int64_t size);

    //! \brief Get total size of a file
    //! \param context The context of the file
    //! \return Total file size
    //! \sa IFile::GetLength
    int64_t  (__cdecl* GetLength) (void* context);

    //! \brief Get current position in a file
    //! \param context The context of the file
    //! \return Current position
    //! \sa IFile::GetPosition
    int64_t  (__cdecl* GetPosition) (void* context);

    //! \brief Get chunk size of a file
    //! \param context The context of the file
    //! \return Chunk size
    //! \sa IFile::GetChunkSize()
    int      (__cdecl* GetChunkSize)(void* context);

    //! \brief Perform an IO-control on the file
    //! \param context The context of the file
    //! \param request The requested IO-control
    //! \param param Parameter attached to the IO-control
    //! \return -1 on error, >= 0 on success
    //! \sa IFile::IoControl
    int  (__cdecl* IoControl) (void* context, XFILE::EIoControl request, void* param);

    //! \brief Stat a file
    //! \param url The URL of the file
    //! \param buffer The buffer to store results in
    //! \return -1 on error, 0 otherwise
    //! \sa IFile::Stat
    int  (__cdecl* Stat) (VFSURL* url, struct __stat64* buffer);
    //! \brief Close a file
    //! \param context The context of the file
    //! \return True on success, false on failure
    //! \sa IFile::Close

    bool (__cdecl* Close) (void* context);

    //! \brief Check for file existence
    //! \param url The URL of the file
    //! \return True if file exists, false otherwise
    //! \sa IFile::Exists
    bool (__cdecl* Exists) (VFSURL* url);

    //! \brief Clear out any idle connections
    void (__cdecl* ClearOutIdle) ();

    //! \brief Disconnect all connections
    void (__cdecl* DisconnectAll) ();

    //! \brief Delete a file
    //! \param url The URL of the file
    //! \return True if deletion was successful, false otherwise
    //! \sa IFile::Delete
    bool (__cdecl* Delete) (VFSURL* url);

    //! \brief Rename a file
    //! \param url The URL of the source file
    //! \param url2 The URL of the destination file
    //! \return True if deletion was successful, false otherwise
    //! \sa IFile::Rename
    bool (__cdecl* Rename) (VFSURL* url, VFSURL* url2);

    //! \brief Check for directory existence
    //! \param url The URL of the file
    //! \return True if directory exists, false otherwise
    //! \sa IDirectory::Exists
    bool (__cdecl* DirectoryExists) (VFSURL* url);

    //! \brief Remove a directory
    //! \param url The URL of the directory
    //! \return True if removal was successful, false otherwise
    //! \sa IDirectory::Remove
    bool (__cdecl* RemoveDirectory) (VFSURL* url);

    //! \brief Create a directory
    //! \param url The URL of the file
    //! \return True if creation was successful, false otherwise
    //! \sa IDirectory::Create
    bool (__cdecl* CreateDirectory) (VFSURL* url);

    //! \brief List a directory
    //! \param url The URL of the directory
    //! \param entries The entries in the directory
    //! \param num_entries Number of entries in the directory
    //! \param callbacks A callback structure
    //! \return Context for the directory listing
    //! \sa IDirectory::GetDirectory
    void* (__cdecl* GetDirectory) (VFSURL* url,
                                   VFSDirEntry** entries,
                                   int* num_entries,
                                   VFSCallbacks* callbacks);

    //! \brief Free up resources after listing a directory
    void (__cdecl* FreeDirectory) (void* ctx);

    //! \brief Check if file should be presented as a directory (multiple streams)
    //! \param url The URL of the file
    //! \param entries The entries in the directory
    //! \param num_entries Number of entries in the directory
    //! \param rootpath Path to root directory if multiple entries
    //! \return Context for the directory listing
    //! \sa IFileDirectory::ContainsFiles, FreeDirectory
    void* (__cdecl* ContainsFiles) (VFSURL* url,
                                    VFSDirEntry** entries,
                                    int* num_entries,
                                    char* rootpath);
  } KodiToAddonFuncTable_VFSEntry;

  typedef struct AddonInstance_VFSEntry
  {
    AddonProps_VFSEntry props;
    AddonToKodiFuncTable_VFSEntry toKodi;
    KodiToAddonFuncTable_VFSEntry toAddon;
  } AddonInstance_VFSEntry;

}
