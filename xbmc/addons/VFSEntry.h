/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "FileItemList.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-dev-kit/include/kodi/addon-instance/VFS.h"
#include "filesystem/IDirectory.h"
#include "filesystem/IFile.h"
#include "filesystem/IFileDirectory.h"

#include <utility>

namespace ADDON
{
struct AddonEvent;

class CVFSEntry;
typedef std::shared_ptr<CVFSEntry> VFSEntryPtr;

class CVFSAddonCache : public CAddonDllInformer
{
public:
  virtual ~CVFSAddonCache();
  void Init();
  void Deinit();
  const std::vector<VFSEntryPtr> GetAddonInstances();
  VFSEntryPtr GetAddonInstance(const std::string& strId);

protected:
  void Update(const std::string& id);
  void OnEvent(const AddonEvent& event);
  bool IsInUse(const std::string& id) override;

  CCriticalSection m_critSection;
  std::vector<VFSEntryPtr> m_addonsInstances;
};

  //! \brief A virtual filesystem entry add-on.
  class CVFSEntry : public IAddonInstanceHandler
  {
  public:
    //! \brief A structure encapsulating properties of supplied protocol.
    struct ProtocolInfo
    {
      bool supportPath;      //!< Protocol has path in addition to server name
      bool supportUsername;  //!< Protocol uses logins
      bool supportPassword;  //!< Protocol supports passwords
      bool supportPort;      //!< Protocol supports port customization
      bool supportBrowsing;  //!< Protocol supports server browsing
      bool supportWrite;     //!< Protocol supports write operations
      int defaultPort;       //!< Default port to use for protocol
      std::string type;      //!< URL type for protocol
      int label;             //!< String ID to use as label in dialog

      //! \brief The constructor reads the info from an add-on info structure.
      ProtocolInfo(const AddonInfoPtr& addonInfo);
    };

    //! \brief Construct from add-on properties.
    //! \param addonInfo General addon properties
    explicit CVFSEntry(const AddonInfoPtr& addonInfo);
    ~CVFSEntry() override;

    // Things that MUST be supplied by the child classes
    void* Open(const CURL& url);
    void* OpenForWrite(const CURL& url, bool bOverWrite);
    bool Exists(const CURL& url);
    int Stat(const CURL& url, struct __stat64* buffer);
    ssize_t Read(void* ctx, void* lpBuf, size_t uiBufSize);
    ssize_t Write(void* ctx, const void* lpBuf, size_t uiBufSize);
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

    bool ContainsFiles(const CURL& url, CFileItemList& items);

    const std::string& GetProtocols() const { return m_protocols; }
    const std::string& GetExtensions() const { return m_extensions; }
    bool HasFiles() const { return m_files; }
    bool HasDirectories() const { return m_directories; }
    bool HasFileDirectories() const { return m_filedirectories; }
    const std::string& GetZeroconfType() const { return m_zeroconf; }
    const ProtocolInfo& GetProtocolInfo() const { return m_protocolInfo; }
  protected:
    std::string m_protocols;  //!< Protocols for VFS entry.
    std::string m_extensions; //!< Extensions for VFS entry.
    std::string m_zeroconf;   //!< Zero conf announce string for VFS protocol.
    bool m_files;             //!< Vfs entry can read files.
    bool m_directories;       //!< VFS entry can list directories.
    bool m_filedirectories;   //!< VFS entry contains file directories.
    ProtocolInfo m_protocolInfo; //!< Info about protocol for network dialog.
  };

  //! \brief Wrapper equipping a CVFSEntry with an IFile interface.
  //! \details Needed as CVFSEntry implements several VFS interfaces
  //!          with overlapping methods.
  class CVFSEntryIFileWrapper : public XFILE::IFile
  {
  public:
    //! \brief The constructor initializes the reference to the wrapped CVFSEntry.
    //! \param ptr The CVFSEntry to wrap.
    explicit CVFSEntryIFileWrapper(VFSEntryPtr ptr);

    //! \brief Empty destructor.
    ~CVFSEntryIFileWrapper() override;

    //! \brief Open a file.
    //! \param[in] url URL to open.
    //! \returns True if file was opened, false otherwise.
    bool Open(const CURL& url) override;

    //! \brief Open a file for writing.
    //! \param[in] url URL to open.
    //! \param[in] bOverWrite If true, overwrite an existing file.
    //! \returns True if file was opened, false otherwise.
    bool OpenForWrite(const CURL& url, bool bOverWrite) override;

    //! \brief Check for file existence.
    //! \param[in] url URL of file.
    bool Exists(const CURL& url) override;

    //! \brief Stat a file.
    //! \param[in] url URL of file.
    //! \param[out] buffer The stat info.
    //! \details Returns 0 on success, non-zero otherwise (see fstat() return values).
    int  Stat(const CURL& url, struct __stat64* buffer) override;

    //! \brief Read data from file:
    //! \param lpBuf Buffer to read data into.
    //! \param[in] uiBufSize Number of bytes to read.
    //! \returns Number of bytes read.
    ssize_t Read(void* lpBuf, size_t uiBufSize) override;

    //! \brief Write data to file.
    //! \param[in] lpBuf Data to write.
    //! \param[in] uiBufSize Number of bytes to write.
    //! \returns Number of bytes written.
    ssize_t Write(const void* lpBuf, size_t uiBufSize) override;

    //! \brief Seek in file.
    //! \param[in] iFilePosition Position to seek to.
    //! \param[in] whence Origin for position.
    //! \returns New file position.
    int64_t Seek(int64_t iFilePosition, int whence = SEEK_SET) override;

    //! \brief Truncate a file.
    //! \param[in] size Size of new file.
    int Truncate(int64_t size) override;

    //! \brief Close file.
    void Close() override;

    //! \brief Obtain current file position.
    int64_t GetPosition() override;

    //! \brief Obtain file size.
    int64_t GetLength() override;

    //! \brief Obtain chunksize of file.
    int GetChunkSize() override;

    //! \brief Perform I/O controls for file.
    int IoControl(XFILE::EIoControl request, void* param) override;

    //! \brief Delete a file.
    //! \param[in] url URL of file to delete.
    bool Delete(const CURL& url) override;

    //! \brief Rename a file.
    //! \param[in] url URL of file to rename.
    //! \param[in] url2 New URL of file.
    bool Rename(const CURL& url, const CURL& url2) override;
  protected:
    void* m_context = nullptr; //!< Opaque add-on specific context for opened file.
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
    explicit CVFSEntryIDirectoryWrapper(VFSEntryPtr ptr);

    //! \brief Empty destructor.
    ~CVFSEntryIDirectoryWrapper() override = default;

    //! \brief Return directory listing.
    //! \param[in] url URL to file to list.
    //! \param items List of items in file.
    //! \return True if listing succeeded, false otherwise.
    bool GetDirectory(const CURL& url, CFileItemList& items) override;

    //! \brief Check if directory exists.
    //! \param[in] url URL to check.
    bool Exists(const CURL& url) override;

    //! \brief Delete directory.
    //! \param[in] url URL to delete.
    bool Remove(const CURL& url) override;

    //! \brief Create directory.
    //! \param[in] url URL to delete.
    bool Create(const CURL& url) override;

    //! \brief Static helper for doing a keyboard callback.
    static bool DoGetKeyboardInput(void* context, const char* heading,
                                   char** input, bool hidden_input);

    //! \brief Get keyboard input.
    bool GetKeyboardInput2(const char* heading, char** input, bool hidden_input);

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
    explicit CVFSEntryIFileDirectoryWrapper(VFSEntryPtr ptr)
      : CVFSEntryIDirectoryWrapper(std::move(ptr))
    {
    }

    //! \brief Empty destructor.
    ~CVFSEntryIFileDirectoryWrapper() override = default;

    //! \brief Check if the given file should be treated as a directory.
    //! \param[in] url URL for file to probe.
    bool ContainsFiles(const CURL& url) override
    {
      return m_addon->ContainsFiles(url, m_items);
    }

    //! \brief Return directory listing.
    //! \param[in] url URL to file to list.
    //! \param items List of items in file.
    //! \return True if listing succeeded, false otherwise.
    bool GetDirectory(const CURL& url, CFileItemList& items) override
    {
      return CVFSEntryIDirectoryWrapper::GetDirectory(url, items);
    }

    //! \brief Check if directory exists.
    //! \param[in] url URL to check.
    bool Exists(const CURL& url) override
    {
      return CVFSEntryIDirectoryWrapper::Exists(url);
    }

    //! \brief Delete directory.
    //! \param[in] url URL to delete.
    bool Remove(const CURL& url) override
    {
      return CVFSEntryIDirectoryWrapper::Remove(url);
    }

    //! \brief Create directory.
    //! \param[in] url URL to delete.
    bool Create(const CURL& url) override
    {
      return CVFSEntryIDirectoryWrapper::Create(url);
    }

    CFileItemList m_items; //!< Internal list of items, used for cache purposes.
  };

} /*namespace ADDON*/
