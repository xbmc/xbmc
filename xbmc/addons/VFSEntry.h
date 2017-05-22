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

#include "AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_vfs_types.h"
#include "filesystem/IFile.h"
#include "filesystem/IDirectory.h"
#include "filesystem/IFileDirectory.h"

namespace ADDON
{
  //! \brief A virtual filesystem entry add-on.
  class CVFSEntry : public CAddonDll
  {
  public:
    static std::unique_ptr<CVFSEntry> FromExtension(AddonProps props,
                                                    const cp_extension_t* ext);

    //! \brief Construct from add-on properties.
    //! \param props General addon properties
    //! \param protocols Protocols associated with add-on
    //! \param extensions File extensions associated with add-on (filedirectories)
    //! \param files If true, add-on provides files
    //! \param directories If true, add-on provides directory listings
    //! \param filedirectories If true, add-on provides filedirectories
    explicit CVFSEntry(AddonProps props,
                      const std::string& protocols,
                      const std::string& extensions,
                      bool files, bool directories, bool filedirectories);

    //! \brief Empty destructor.
    virtual ~CVFSEntry() {}

    // Things that MUST be supplied by the child classes
    bool Create();
    void* Open(const CURL& url);
    void* OpenForWrite(const CURL& url, bool bOverWrite);
    bool Exists(const CURL& url);
    int Stat(const CURL& url, struct __stat64* buffer);
    ssize_t Read(void* ctx, void* lpBuf, size_t uiBufSize);
    ssize_t Write(void* ctx, void* lpBuf, size_t uiBufSize);
    int64_t Seek(void* ctx, int64_t iFilePosition, int iWhence = SEEK_SET);
    int Truncate(void* ctx, int64_t size);
    void Close(void* ctx);
    int64_t GetPosition(void* ctx);
    int64_t GetLength(void* ctx);
    int GetChunkSize(void* ctx);
    int IoControl(void* ctx, XFILE::EIoControl request, void* param);
    bool Delete(const CURL& url);
    bool Rename(const CURL& url, const CURL& url2);

    bool GetDirectory(const CURL& url, CFileItemList& items, void* ctx);
    bool DirectoryExists(const CURL& url);
    bool RemoveDirectory(const CURL& url);
    bool CreateDirectory(const CURL& url);
    void ClearOutIdle();
    void DisconnectAll();

    bool ContainsFiles(const CURL& path, CFileItemList& items);

    const std::string& GetProtocols() const { return m_protocols; }
    const std::string& GetExtensions() const { return m_extensions; }
    bool HasFiles() const { return m_files; }
    bool HasDirectories() const { return m_directories; }
    bool HasFileDirectories() const { return m_filedirectories; }
  protected:
    std::string m_protocols; //!< Protocols for VFS entry.
    std::string m_extensions; //!< Extensions for VFS entry.
    bool m_files;             //!< Vfs entry can read files.
    bool m_directories;       //!< VFS entry can list directories.
    bool m_filedirectories;   //!< VFS entry contains file directories.
    AddonInstance_VFSEntry m_struct; //!< VFS callback table
  };

  typedef std::shared_ptr<CVFSEntry> VFSEntryPtr; //!< Convenience typedef.

  //! \brief Wrapper equpping a CVFSEntry with an IFile interface.
  //! \details Needed as CVFSEntry implements several VFS interfaces
  //!          with overlapping methods.
  class CVFSEntryIFileWrapper : public XFILE::IFile
  {
  public:
    //! \brief The constructor initializes the reference to the wrapped CVFSEntry.
    //! \param ptr The CVFSEntry to wrap.
    CVFSEntryIFileWrapper(VFSEntryPtr ptr);

    //! \brief Empty destructor.
    virtual ~CVFSEntryIFileWrapper();

    //! \brief Open a file.
    //! \param[in] url URL to open.
    //! \returns True if file was opened, false otherwise.
    virtual bool Open(const CURL& url);

    //! \brief Open a file for writing.
    //! \param[in] url URL to open.
    //! \param[in] bOverWrite If true, overwrite an existing file.
    //! \returns True if file was opened, false otherwise.
    virtual bool OpenForWrite(const CURL& url, bool bOverWrite);

    //! \brief Check for file existence.
    //! \param[in] url URL of file.
    virtual bool Exists(const CURL& url);

    //! \brief Stat a file.
    //! \param[in] url URL of file.
    //! \param[out] buffer The stat info.
    //! \details Returns 0 on success, non-zero otherwise (see fstat() return values).
    virtual int  Stat(const CURL& url, struct __stat64* buffer);

    //! \brief Read data from file:
    //! \param lpBuf Buffer to read data into.
    //! \param[in] uiBufSize Number of bytes to read.
    //! \returns Number of bytes read.
    virtual ssize_t Read(void* lpBuf, size_t uiBufSize);

    //! \brief Write data to file.
    //! \param[in] lpBuf Data to write.
    //! \param[in] uiBufSize Number of bytes to write.
    //! \returns Number of bytes written.
    virtual ssize_t Write(void* lpBuf, size_t uiBufSize);

    //! \brief Seek in file.
    //! \param[in] iFilePosition Position to seek to.
    //! \param[in] whence Origin for position.
    //! \returns New file position.
    virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);

    //! \brief Truncate a file.
    //! \param[in] size Size of new file.
    virtual int Truncate(int64_t size);

    //! \brief Close file.
    virtual void Close();

    //! \brief Obtain current file position.
    virtual int64_t GetPosition();

    //! \brief Obtain file size.
    virtual int64_t GetLength();

    //! \brief Obtain chunksize of file.
    virtual int GetChunkSize();

    //! \brief Perform I/O controls for file.
    virtual int IoControl(XFILE::EIoControl request, void* param);

    //! \brief Delete a file.
    //! \param[in] url URL of file to delete.
    virtual bool Delete(const CURL& url);

    //! \brief Rename a file.
    //! \param[in] url URL of file to rename.
    //! \param[in] url2 New URL of file.
    virtual bool Rename(const CURL& url, const CURL& url2);
  protected:
    void* m_context; //!< Opaque add-on specific context for opened file.
    VFSEntryPtr m_addon; //!< Pointer to wrapped CVFSEntry.
  };

  //! \brief Wrapper equpping a CVFSEntry with an IDirectory interface.
  //! \details Needed as CVFSEntry implements several VFS interfaces
  //!          with overlapping methods.
  class CVFSEntryIDirectoryWrapper : public XFILE::IDirectory
  {
  public:
    //! \brief The constructor initializes the reference to the wrapped CVFSEntry.
    //! \param ptr The CVFSEntry to wrap.
    CVFSEntryIDirectoryWrapper(VFSEntryPtr ptr);

    //! \brief Empty destructor.
    virtual ~CVFSEntryIDirectoryWrapper() {}

    //! \brief Return directory listing.
    //! \param[in] url URL to file to list.
    //! \param items List of items in file.
    //! \return True if listing succeeded, false otherwise.
    virtual bool GetDirectory(const CURL& strPath, CFileItemList& items);

    //! \brief Check if directory exists.
    //! \param[in] url URL to check.
    virtual bool Exists(const CURL& strPath);

    //! \brief Delete directory.
    //! \param[in] url URL to delete.
    virtual bool Remove(const CURL& strPath);

    //! \brief Create directory.
    //! \param[in] url URL to delete.
    virtual bool Create(const CURL& strPath);

    //! \brief Static helper for doing a keyboard callback.
    static bool DoGetKeyboardInput(void* context, const char* heading,
                                   char** input);

    //! \brief Get keyboard input.
    bool GetKeyboardInput2(const char* heading, char** input);

    //! \brief Static helper for displaying an error dialog.
    static void DoSetErrorDialog(void* ctx, const char* heading,
                                 const char* line1, const char* line2,
                                 const char* line3);

    //! \brief Show an error dialog.
    void SetErrorDialog2(const char* heading, const char* line1,
                         const char* line2, const char* line3);

    //! \brief Static helper for requiring authentication.
    static void DoRequireAuthentication(void* ctx, const char* url);

    //! \brief Require authentication.
    void RequireAuthentication2(const CURL& url);
  protected:
    VFSEntryPtr m_addon; //!< Pointer to wrapper CVFSEntry.
  };

  //! \brief Wrapper equpping a CVFSEntry with an IFileDirectory interface.
  //! \details Needed as CVFSEntry implements several VFS interfaces
  //!          with overlapping methods.
  class CVFSEntryIFileDirectoryWrapper : public XFILE::IFileDirectory,
                                         public CVFSEntryIDirectoryWrapper
  {
  public:
    //! \brief The constructor initializes the reference to the wrapped CVFSEntry.
    //! \param ptr The CVFSEntry to wrap.
    CVFSEntryIFileDirectoryWrapper(VFSEntryPtr ptr) : CVFSEntryIDirectoryWrapper(ptr) {}

    //! \brief Empty destructor.
    virtual ~CVFSEntryIFileDirectoryWrapper() {}

    //! \brief Check if the given file should be treated as a directory.
    //! \param[in] URL URL for file to probe.
    bool ContainsFiles(const CURL& url)
    {
      return m_addon->ContainsFiles(url, m_items);
    }

    //! \brief Return directory listing.
    //! \param[in] url URL to file to list.
    //! \param items List of items in file.
    //! \return True if listing succeeded, false otherwise.
    bool GetDirectory(const CURL& url, CFileItemList& items)
    {
      return CVFSEntryIDirectoryWrapper::GetDirectory(url, items);
    }

    //! \brief Check if directory exists.
    //! \param[in] url URL to check.
    bool Exists(const CURL& url)
    {
      return CVFSEntryIDirectoryWrapper::Exists(url);
    }

    //! \brief Delete directory.
    //! \param[in] url URL to delete.
    bool Remove(const CURL& url)
    {
      return CVFSEntryIDirectoryWrapper::Remove(url);
    }

    //! \brief Create directory.
    //! \param[in] url URL to delete.
    bool Create(const CURL& url)
    {
      return CVFSEntryIDirectoryWrapper::Create(url);
    }

    CFileItemList m_items; //!< Internal list of items, used for cache purposes.
  };

} /*namespace ADDON*/
