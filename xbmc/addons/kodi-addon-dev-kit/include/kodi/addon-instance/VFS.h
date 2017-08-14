#pragma once
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

#include "../AddonBase.h"
#include "../Filesystem.h"

#ifdef BUILD_KODI_ADDON
#include "../IFileTypes.h"
#else
#include "filesystem/IFileTypes.h"
#include "PlatformDefs.h"
#endif

namespace kodi { namespace addon { class CInstanceVFS; }}

extern "C"
{

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

  typedef struct VFSGetDirectoryCallbacks /* internal */
  {
    bool (__cdecl* get_keyboard_input)(void* ctx, const char* heading, char** input, bool hidden_input);
    void (__cdecl* set_error_dialog)(void* ctx, const char* heading, const char* line1, const char* line2, const char* line3);
    void (__cdecl* require_authentication)(void* ctx, const char* url);
    void* ctx;
  } VFSGetDirectoryCallbacks;

  typedef struct AddonProps_VFSEntry /* internal */
  {
    int dummy;
  } AddonProps_VFSEntry;

  typedef struct AddonToKodiFuncTable_VFSEntry /* internal */
  {
    KODI_HANDLE kodiInstance;
  } AddonToKodiFuncTable_VFSEntry;

  struct AddonInstance_VFSEntry;
  typedef struct KodiToAddonFuncTable_VFSEntry /* internal */
  {
    kodi::addon::CInstanceVFS* addonInstance;

    void* (__cdecl* open) (const AddonInstance_VFSEntry* instance, const VFSURL* url);
    void* (__cdecl* open_for_write) (const AddonInstance_VFSEntry* instance, const VFSURL* url, bool overwrite);
    ssize_t (__cdecl* read) (const AddonInstance_VFSEntry* instance, void* context, void* buffer, size_t buf_size);
    ssize_t (__cdecl* write) (const AddonInstance_VFSEntry* instance, void* context, const void* buffer, size_t buf_size);
    int64_t (__cdecl* seek) (const AddonInstance_VFSEntry* instance, void* context, int64_t position, int whence);
    int (__cdecl* truncate) (const AddonInstance_VFSEntry* instance, void* context, int64_t size);
    int64_t (__cdecl* get_length) (const AddonInstance_VFSEntry* instance, void* context);
    int64_t (__cdecl* get_position) (const AddonInstance_VFSEntry* instance, void* context);
    int (__cdecl* get_chunk_size) (const AddonInstance_VFSEntry* instance, void* context);
    int (__cdecl* io_control) (const AddonInstance_VFSEntry* instance, void* context, XFILE::EIoControl request, void* param);
    int (__cdecl* stat) (const AddonInstance_VFSEntry* instance, const VFSURL* url, struct __stat64* buffer);
    bool (__cdecl* close) (const AddonInstance_VFSEntry* instance, void* context);
    bool (__cdecl* exists) (const AddonInstance_VFSEntry* instance, const VFSURL* url);
    void (__cdecl* clear_out_idle) (const AddonInstance_VFSEntry* instance);
    void (__cdecl* disconnect_all) (const AddonInstance_VFSEntry* instance);
    bool (__cdecl* delete_it) (const AddonInstance_VFSEntry* instance, const VFSURL* url);
    bool (__cdecl* rename) (const AddonInstance_VFSEntry* instance, const VFSURL* url, const VFSURL* url2);
    bool (__cdecl* directory_exists) (const AddonInstance_VFSEntry* instance, const VFSURL* url);
    bool (__cdecl* remove_directory) (const AddonInstance_VFSEntry* instance, const VFSURL* url);
    bool (__cdecl* create_directory) (const AddonInstance_VFSEntry* instance, const VFSURL* url);
    bool (__cdecl* get_directory) (const AddonInstance_VFSEntry* instance,
                                  const VFSURL* url,
                                  VFSDirEntry** entries,
                                  int* num_entries,
                                  VFSGetDirectoryCallbacks* callbacks);
    bool (__cdecl* contains_files) (const AddonInstance_VFSEntry* instance,
                                   const VFSURL* url,
                                   VFSDirEntry** entries,
                                   int* num_entries,
                                   char* rootpath);
    void (__cdecl* free_directory) (const AddonInstance_VFSEntry* instance, VFSDirEntry* entries, int num_entries);
  } KodiToAddonFuncTable_VFSEntry;

  typedef struct AddonInstance_VFSEntry /* internal */
  {
    AddonProps_VFSEntry props;
    AddonToKodiFuncTable_VFSEntry toKodi;
    KodiToAddonFuncTable_VFSEntry toAddon;
  } AddonInstance_VFSEntry;

} /* extern "C" */

namespace kodi
{
namespace addon
{
  class CInstanceVFS : public IAddonInstance
  {
  public:
    explicit CInstanceVFS(KODI_HANDLE instance)
      : IAddonInstance(ADDON_INSTANCE_VFS)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceVFS: Creation of multiple together with single instance way is not allowed!");

      SetAddonStruct(instance);
    }

    ~CInstanceVFS() override = default;

    /// @brief Open a file for input
    ///
    /// @param[in] url The URL of the file
    /// @return Context for the opened file
    virtual void* Open(const VFSURL& url) { return nullptr; }

    /// @brief Open a file for output
    ///
    /// @param[in] url The URL of the file
    /// @param[in] overWrite Whether or not to overwrite an existing file
    /// @return Context for the opened file
    ///
    virtual void* OpenForWrite(const VFSURL& url, bool overWrite) { return nullptr; }

    /// @brief Read from a file
    ///
    /// @param[in] context The context of the file
    /// @param[out] buffer The buffer to read data into
    /// @param[in] uiBufSize Number of bytes to read
    /// @return Number of bytes read
    ///
    virtual ssize_t Read(void* context, void* buffer, size_t uiBufSize) { return -1; }

    /// @brief Write to a file
    ///
    /// @param[in] context The context of the file
    /// @param[in] buffer The buffer to read data from
    /// @param[in] uiBufSize Number of bytes to write
    /// @return Number of bytes written
    ///
    virtual ssize_t Write(void* context, const void* buffer, size_t uiBufSize) { return -1; }

    /// @brief Seek in a file
    ///
    /// @param[in] context The context of the file
    /// @param[in] position The position to seek to
    /// @param[in] whence Position in file 'position' is relative to (SEEK_CUR, SEEK_SET, SEEK_END)
    /// @return Offset in file after seek
    ///
    virtual int64_t Seek(void* context, int64_t position, int whence) { return -1; }

    /// @brief Truncate a file
    ///
    /// @param[in] context The context of the file
    /// @param[in] size The size to truncate the file to
    /// @return 0 on success, -1 on error
    ///
    virtual int Truncate(void* context, int64_t size) { return -1; }

    /// @brief Get total size of a file
    ///
    /// @param[in] context The context of the file
    /// @return Total file size
    ///
    virtual int64_t GetLength(void* context) { return 0; }

    /// @brief Get current position in a file
    ///
    /// @param[in] context The context of the file
    /// @return Current position
    ///
    virtual int64_t GetPosition(void* context) { return 0; }

    /// @brief Get chunk size of a file
    ///
    /// @param[in] context The context of the file
    /// @return Chunk size
    ///
    virtual int GetChunkSize(void* context) { return 1; }

    /// @brief Perform an IO-control on the file
    ///
    /// @param[in] context The context of the file
    /// @param[in] request The requested IO-control
    /// @param[in] param Parameter attached to the IO-control
    /// @return -1 on error, >= 0 on success
    ///
    virtual int IoControl(void* context, XFILE::EIoControl request, void* param) { return -1; }

    /// @brief Close a file
    ///
    /// @param[in] context The context of the file
    /// @return True on success, false on failure
    ///
    virtual bool Close(void* context) { return false; }

    /// @brief Stat a file
    ///
    /// @param[in] url The URL of the file
    /// @param[in] buffer The buffer to store results in
    /// @return -1 on error, 0 otherwise
    ///
    virtual int Stat(const VFSURL& url, struct __stat64* buffer) { return 0; }

    /// @brief Check for file existence
    ///
    /// @param[in] url The URL of the file
    /// @return True if file exists, false otherwise
    ///
    virtual bool Exists(const VFSURL& url) { return false; }

    /// @brief Clear out any idle connections
    ///
    virtual void ClearOutIdle() { }

    /// @brief Disconnect all connections
    ///
    virtual void DisconnectAll() { }

    /// @brief Delete a file
    ///
    /// @param[in] url The URL of the file
    /// @return True if deletion was successful, false otherwise
    ///
    virtual bool Delete(const VFSURL& url) { return false; }

    /// @brief Rename a file
    ///
    /// @param[in] url The URL of the source file
    /// @param[in] url2 The URL of the destination file
    /// @return True if deletion was successful, false otherwise
    ///
    virtual bool Rename(const VFSURL& url, const VFSURL& url2) { return false; }

    /// @brief Check for directory existence
    ///
    /// @param[in] url The URL of the file
    /// @return True if directory exists, false otherwise
    ///
    virtual bool DirectoryExists(const VFSURL& url) { return false; }

    /// @brief Remove a directory
    ///
    /// @param[in] url The URL of the directory
    /// @return True if removal was successful, false otherwise
    ///
    virtual bool RemoveDirectory(const VFSURL& url) { return false; }

    /// @brief Create a directory
    ///
    /// @param[in] url The URL of the file
    /// @return True if creation was successful, false otherwise
    ///
    virtual bool CreateDirectory(const VFSURL& url) { return false; }

    /// @brief Callback functions on GetDirectory()
    ///
    /// This functions becomes available during call of GetDirectory() from
    /// Kodi.
    ///
    /// If GetDirectory() returns false becomes the parts from here used on
    /// next call of the function.
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    ///
    /// #include <kodi/addon-instance/VFS.h>
    ///
    /// ...
    ///
    /// bool CMyFile::GetDirectory(const VFSURL& url, std::vector<kodi::vfs::CDirEntry>& items, CVFSCallbacks callbacks)
    /// {
    ///   std::string neededString;
    ///   callbacks.GetKeyboardInput("Test", neededString, true);
    ///   if (neededString.empty())
    ///     return false;
    ///
    ///   /* Do the work */
    ///   ...
    ///   return true;
    /// }
    /// ~~~~~~~~~~~~~
    ///
    //@{
    class CVFSCallbacks
    {
    public:
      /// @brief Require keyboard input
      ///
      /// Becomes called if GetDirectory() returns false and GetDirectory()
      /// becomes after entry called again.
      ///
      /// @param[in] heading      The heading of the keyboard dialog
      /// @param[out] input       The resulting string. Returns string after
      ///                         second call!
      /// @param[in] hiddenInput  To show input only as "*" on dialog
      /// @return                 True if input was received, false otherwise
      ///
      bool GetKeyboardInput(const std::string& heading, std::string& input, bool hiddenInput = false)
      {
        char* cInput = nullptr;
        bool ret = m_cb->get_keyboard_input(m_cb->ctx, heading.c_str(), &cInput, hiddenInput);
        if (cInput)
        {
          input = cInput;
          ::kodi::addon::CAddonBase::m_interface->toKodi->free_string(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, cInput);
        }
        return ret;
      }

      /// @brief Display an error dialog
      ///
      /// @param[in] heading      The heading of the error dialog
      /// @param[in] line1        The first line of the error dialog
      /// @param[in] line2        [opt] The second line of the error dialog
      /// @param[in] line3        [opt] The third line of the error dialog
      ///
      void SetErrorDialog(const std::string& heading, const std::string& line1, const std::string& line2 = "", const std::string& line3 = "")
      {
        m_cb->set_error_dialog(m_cb->ctx, heading.c_str(), line1.c_str(), line2.c_str(), line3.c_str());
      }

      /// @brief Prompt the user for authentication of a URL
      ///
      /// @param[in] url The URL
      void RequireAuthentication(const std::string& url)
      {
        m_cb->require_authentication(m_cb->ctx, url.c_str());
      }

      explicit CVFSCallbacks(const VFSGetDirectoryCallbacks* cb) : m_cb(cb) { }

    private:
      const VFSGetDirectoryCallbacks* m_cb;
    };
    //@}

    /// @brief List a directory
    ///
    /// @param[in] url The URL of the directory
    /// @param[out] entries The entries in the directory
    /// @param[in] callbacks A callback structure
    /// @return Context for the directory listing
    ///
    virtual bool GetDirectory(const VFSURL& url,
                              std::vector<kodi::vfs::CDirEntry>& entries,
                              CVFSCallbacks callbacks) { return false; }

    /// @brief Check if file should be presented as a directory (multiple streams)
    ///
    /// @param[in] url The URL of the file
    /// @param[out] entries The entries in the directory
    /// @param[out] rootPath Path to root directory if multiple entries
    /// @return Context for the directory listing
    ///
    virtual bool ContainsFiles(const VFSURL& url,
                               std::vector<kodi::vfs::CDirEntry>& entries,
                               std::string& rootPath) { return false; }

  private:
    void SetAddonStruct(KODI_HANDLE instance)
    {
      if (instance == nullptr)
        throw std::logic_error("kodi::addon::CInstanceVFS: Creation with empty addon structure not allowed, table must be given from Kodi!");

      m_instanceData = static_cast<AddonInstance_VFSEntry*>(instance);
      m_instanceData->toAddon.addonInstance = this;
      m_instanceData->toAddon.open = ADDON_Open;
      m_instanceData->toAddon.open_for_write = ADDON_OpenForWrite;
      m_instanceData->toAddon.read = ADDON_Read;
      m_instanceData->toAddon.write = ADDON_Write;
      m_instanceData->toAddon.seek = ADDON_Seek;
      m_instanceData->toAddon.truncate = ADDON_Truncate;
      m_instanceData->toAddon.get_length = ADDON_GetLength;
      m_instanceData->toAddon.get_position = ADDON_GetPosition;
      m_instanceData->toAddon.get_chunk_size = ADDON_GetChunkSize;
      m_instanceData->toAddon.io_control = ADDON_IoControl;
      m_instanceData->toAddon.stat = ADDON_Stat;
      m_instanceData->toAddon.close = ADDON_Close;
      m_instanceData->toAddon.exists = ADDON_Exists;
      m_instanceData->toAddon.clear_out_idle = ADDON_ClearOutIdle;
      m_instanceData->toAddon.disconnect_all = ADDON_DisconnectAll;
      m_instanceData->toAddon.delete_it = ADDON_Delete;
      m_instanceData->toAddon.rename = ADDON_Rename;
      m_instanceData->toAddon.directory_exists = ADDON_DirectoryExists;
      m_instanceData->toAddon.remove_directory = ADDON_RemoveDirectory;
      m_instanceData->toAddon.create_directory = ADDON_CreateDirectory;
      m_instanceData->toAddon.get_directory = ADDON_GetDirectory;
      m_instanceData->toAddon.free_directory = ADDON_FreeDirectory;
      m_instanceData->toAddon.contains_files = ADDON_ContainsFiles;
    }

    inline static void* ADDON_Open(const AddonInstance_VFSEntry* instance, const VFSURL* url)
    {
      return instance->toAddon.addonInstance->Open(*url);
    }

    inline static void* ADDON_OpenForWrite(const AddonInstance_VFSEntry* instance, const VFSURL* url, bool overWrite)
    {
      return instance->toAddon.addonInstance->OpenForWrite(*url, overWrite);
    }

    inline static ssize_t ADDON_Read(const AddonInstance_VFSEntry* instance, void* context, void* buffer, size_t uiBufSize)
    {
      return instance->toAddon.addonInstance->Read(context, buffer, uiBufSize);
    }

    inline static ssize_t ADDON_Write(const AddonInstance_VFSEntry* instance, void* context, const void* buffer, size_t uiBufSize)
    {
      return instance->toAddon.addonInstance->Write(context, buffer, uiBufSize);
    }

    inline static int64_t ADDON_Seek(const AddonInstance_VFSEntry* instance, void* context, int64_t position, int whence)
    {
      return instance->toAddon.addonInstance->Seek(context, position, whence);
    }

    inline static int ADDON_Truncate(const AddonInstance_VFSEntry* instance, void* context, int64_t size)
    {
      return instance->toAddon.addonInstance->Truncate(context, size);
    }

    inline static int64_t ADDON_GetLength(const AddonInstance_VFSEntry* instance, void* context)
    {
      return instance->toAddon.addonInstance->GetLength(context);
    }

    inline static int64_t ADDON_GetPosition(const AddonInstance_VFSEntry* instance, void* context)
    {
      return instance->toAddon.addonInstance->GetPosition(context);
    }

    inline static int ADDON_GetChunkSize(const AddonInstance_VFSEntry* instance, void* context)
    {
      return instance->toAddon.addonInstance->GetChunkSize(context);
    }

    inline static int ADDON_IoControl(const AddonInstance_VFSEntry* instance, void* context, XFILE::EIoControl request, void* param)
    {
      return instance->toAddon.addonInstance->IoControl(context, request, param);
    }

    inline static int ADDON_Stat(const AddonInstance_VFSEntry* instance, const VFSURL* url, struct __stat64* buffer)
    {
      return instance->toAddon.addonInstance->Stat(*url, buffer);
    }

    inline static bool ADDON_Close(const AddonInstance_VFSEntry* instance, void* context)
    {
      return instance->toAddon.addonInstance->Close(context);
    }

    inline static bool ADDON_Exists(const AddonInstance_VFSEntry* instance, const VFSURL* url)
    {
      return instance->toAddon.addonInstance->Exists(*url);
    }

    inline static void ADDON_ClearOutIdle(const AddonInstance_VFSEntry* instance)
    {
      return instance->toAddon.addonInstance->ClearOutIdle();
    }

    inline static void ADDON_DisconnectAll(const AddonInstance_VFSEntry* instance)
    {
      return instance->toAddon.addonInstance->DisconnectAll();
    }

    inline static bool ADDON_Delete(const AddonInstance_VFSEntry* instance, const VFSURL* url)
    {
      return instance->toAddon.addonInstance->Delete(*url);
    }

    inline static bool ADDON_Rename(const AddonInstance_VFSEntry* instance, const VFSURL* url, const VFSURL* url2)
    {
      return instance->toAddon.addonInstance->Rename(*url, *url2);
    }

    inline static bool ADDON_DirectoryExists(const AddonInstance_VFSEntry* instance, const VFSURL* url)
    {
      return instance->toAddon.addonInstance->DirectoryExists(*url);
    }

    inline static bool ADDON_RemoveDirectory(const AddonInstance_VFSEntry* instance, const VFSURL* url)
    {
      return instance->toAddon.addonInstance->RemoveDirectory(*url);
    }

    inline static bool ADDON_CreateDirectory(const AddonInstance_VFSEntry* instance, const VFSURL* url)
    {
      return instance->toAddon.addonInstance->CreateDirectory(*url);
    }

    inline static bool ADDON_GetDirectory(const AddonInstance_VFSEntry* instance,
                                          const VFSURL* url,
                                          VFSDirEntry** retEntries,
                                          int* num_entries,
                                          VFSGetDirectoryCallbacks* callbacks)
    {
      std::vector<kodi::vfs::CDirEntry> addonEntries;
      bool ret = instance->toAddon.addonInstance->GetDirectory(*url, addonEntries, CVFSCallbacks(callbacks));
      if (ret)
      {
        VFSDirEntry* entries = static_cast<VFSDirEntry*>(malloc(sizeof(VFSDirEntry) * addonEntries.size()));
        for (unsigned int i = 0; i < addonEntries.size(); ++i)
        {
          entries[i].label = strdup(addonEntries[i].Label().c_str());
          entries[i].title = strdup(addonEntries[i].Title().c_str());
          entries[i].path = strdup(addonEntries[i].Path().c_str());
          entries[i].folder = addonEntries[i].IsFolder();
          entries[i].size = addonEntries[i].Size();

          entries[i].num_props = 0;
          const std::map<std::string, std::string>& props = addonEntries[i].GetProperties();
          if (!props.empty())
          {
            entries[i].properties = static_cast<VFSProperty*>(malloc(sizeof(VFSProperty)*props.size()));
            for (const auto& prop : props)
            {
              entries[i].properties[entries[i].num_props].name = strdup(prop.first.c_str());
              entries[i].properties[entries[i].num_props].val = strdup(prop.second.c_str());
              ++entries[i].num_props;
            }
          }
          else
            entries[i].properties = nullptr;
        }
        *retEntries = entries;
        *num_entries = addonEntries.size();
      }
      return ret;
    }

    inline static void ADDON_FreeDirectory(const AddonInstance_VFSEntry* instance, VFSDirEntry* entries, int num_entries)
    {
      for (int i = 0; i < num_entries; ++i)
      {
        if (entries[i].properties)
        {
          for (unsigned int j = 0; j < entries[i].num_props; ++j)
          {
            free(entries[i].properties[j].name);
            free(entries[i].properties[j].val);
          }
          free(entries[i].properties);
        }
        free(entries[i].label);
        free(entries[i].title);
        free(entries[i].path);
      }
      free(entries);
    }

    inline static bool ADDON_ContainsFiles(const AddonInstance_VFSEntry* instance,
                                           const VFSURL* url,
                                           VFSDirEntry** retEntries,
                                           int* num_entries,
                                           char* rootpath)
    {
      std::string cppRootPath; 
      std::vector<kodi::vfs::CDirEntry> addonEntries;
      bool ret = instance->toAddon.addonInstance->ContainsFiles(*url, addonEntries, cppRootPath);
      if (ret)
      {
        strncpy(rootpath, cppRootPath.c_str(), ADDON_STANDARD_STRING_LENGTH);

        VFSDirEntry* entries = static_cast<VFSDirEntry*>(malloc(sizeof(VFSDirEntry) * addonEntries.size()));
        for (unsigned int i = 0; i < addonEntries.size(); ++i)
        {
          entries[i].label = strdup(addonEntries[i].Label().c_str());
          entries[i].title = strdup(addonEntries[i].Title().c_str());
          entries[i].path = strdup(addonEntries[i].Path().c_str());
          entries[i].folder = addonEntries[i].IsFolder();
          entries[i].size = addonEntries[i].Size();

          entries[i].num_props = 0;
          const std::map<std::string, std::string>& props = addonEntries[i].GetProperties();
          if (!props.empty())
          {
            entries[i].properties = static_cast<VFSProperty*>(malloc(sizeof(VFSProperty)*props.size()));
            for (const auto& prop : props)
            {
              entries[i].properties[entries[i].num_props].name = strdup(prop.first.c_str());
              entries[i].properties[entries[i].num_props].val = strdup(prop.second.c_str());
              ++entries[i].num_props;
            }
          }
          else
            entries[i].properties = nullptr;
        }
        *retEntries = entries;
        *num_entries = addonEntries.size();
      }
      return ret;
    }

    AddonInstance_VFSEntry* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
