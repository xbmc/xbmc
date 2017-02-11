#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonBase.h"
#include "definitions.h"

#ifndef S_ISDIR
  #define S_ISDIR(mode)  ((((mode)) & 0170000) == (0040000))
#endif

#ifndef S_ISLNK
  #define S_ISLNK(mode)  ((((mode)) & 0170000) == (0120000))
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

} /* extern "C" */

//==============================================================================
///
typedef enum OpenFileFlags
{
  /// indicate that caller can handle truncated reads, where function returns
  /// before entire buffer has been filled
  READ_TRUNCATED = 0x01,

  /// indicate that that caller support read in the minimum defined chunk size,
  /// this disables internal cache then
  READ_CHUNKED = 0x02,

  /// use cache to access this file
  READ_CACHED = 0x04,

  /// open without caching. regardless to file type
  READ_NO_CACHE = 0x08,

  /// calcuate bitrate for file while reading
  READ_BITRATE = 0x10,

  /// indicate to the caller we will seek between multiple streams in the file
  /// frequently
  READ_MULTI_STREAM = 0x20,

  /// indicate to the caller file is audio and/or video (and e.g. may grow)
  READ_AUDIO_VIDEO = 0x40,

  /// indicate that caller will do write operations before reading
  READ_AFTER_WRITE = 0x80
} OpenFileFlags;
//------------------------------------------------------------------------------

//==============================================================================
///
typedef enum CURLOptiontype
{
  /// Set a general option
  ADDON_CURL_OPTION_OPTION,
  /// Set a protocol option
  ADDON_CURL_OPTION_PROTOCOL,
  /// Set User and password
  ADDON_CURL_OPTION_CREDENTIALS,
  /// Add a Header
  ADDON_CURL_OPTION_HEADER
} CURLOptiontype;
//------------------------------------------------------------------------------

//============================================================================
///
struct STAT_STRUCTURE
{
  uint32_t    deviceId;         // ID of device containing file
  uint64_t    size;             // Total size, in bytes
#if defined(_WIN32)
  __time64_t  accessTime;       // Time of last access
  __time64_t  modificationTime; // Time of last modification
  __time64_t  statusTime;       // Time of last status change
#else
  timespec    accessTime;       // Time of last access
  timespec    modificationTime; // Time of last modification
  timespec    statusTime;       // Time of last status change
#endif
  bool        isDirectory;      // The stat url is a directory
  bool        isSymLink;        // The stat url is a symbolic link
  bool        isHidden;         // The file is hidden
};
//------------------------------------------------------------------------------
  
namespace kodi
{
namespace vfs
{

  //============================================================================
  ///
  class CDirEntry
  {
  public:
    CDirEntry(const std::string& label = "",
                 const std::string& path = "",
                 bool bFolder = false,
                 int64_t size = -1) :
      m_label(label),
      m_path(path),
      m_bFolder(bFolder),
      m_size(size)
    {
    }

    CDirEntry(const VFSDirEntry& dirEntry) :
      m_label(dirEntry.label ? dirEntry.label : ""),
      m_path(dirEntry.path ? dirEntry.path : ""),
      m_bFolder(dirEntry.folder),
      m_size(dirEntry.size)
    {
    }

    const std::string& Label(void) const { return m_label; }
    const std::string& Path(void) const { return m_path; }
    bool IsFolder(void) const { return m_bFolder; }
    int64_t Size(void) const { return m_size; }

    void SetLabel(const std::string& label) { m_label = label; }
    void SetPath(const std::string& path) { m_path = path; }
    void SetFolder(bool bFolder) { m_bFolder = bFolder; }
    void SetSize(int64_t size) { m_size = size; }

  private:
    std::string m_label;
    std::string m_path;
    bool m_bFolder;
    int64_t m_size;
  };
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline bool CreateDirectory(const std::string& path)
  {
    return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.create_directory(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, path.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline bool DirectoryExists(const std::string& path)
  {
    return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.directory_exists(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, path.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline bool RemoveDirectory(const std::string& path)
  {
    return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.remove_directory(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, path.c_str());
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline bool GetDirectory(const std::string& path, const std::string& mask, std::vector<CDirEntry>& items)
  {
    VFSDirEntry* dir_list = nullptr;
    unsigned int num_items = 0;
    if (::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.get_directory(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, path.c_str(), mask.c_str(), &dir_list, &num_items))
    {
      if (dir_list)
      {
        for (unsigned int i = 0; i < num_items; ++i)
          items.push_back(CDirEntry(dir_list[i]));

        ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.free_directory(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, dir_list, num_items);
      }

      return true;
    }
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline std::string GetFileMD5(const std::string& path)
  {
    std::string strReturn;
    char* strMd5 = ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.get_file_md5(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, path.c_str());
    if (strMd5 != nullptr)
    {
      if (std::strlen(strMd5))
        strReturn = strMd5;
      ::kodi::addon::CAddonBase::m_interface->toKodi.free_string(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, strMd5);
    }
    return strReturn;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline std::string GetCacheThumbName(const std::string& filename)
  {
    std::string strReturn;
    char* strThumbName = ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.get_cache_thumb_name(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, filename.c_str());
    if (strThumbName != nullptr)
    {
      if (std::strlen(strThumbName))
        strReturn = strThumbName;
      ::kodi::addon::CAddonBase::m_interface->toKodi.free_string(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, strThumbName);
    }
    return strReturn;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline std::string MakeLegalFileName(const std::string& filename)
  {
    std::string strReturn;
    char* strLegalFileName = ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.make_legal_filename(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, filename.c_str());
    if (strLegalFileName != nullptr)
    {
      if (std::strlen(strLegalFileName))
        strReturn = strLegalFileName;
      ::kodi::addon::CAddonBase::m_interface->toKodi.free_string(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, strLegalFileName);
    }
    return strReturn;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline std::string MakeLegalPath(const std::string& path)
  {
    std::string strReturn;
    char* strLegalPath = ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.make_legal_path(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, path.c_str());
    if (strLegalPath != nullptr)
    {
      if (std::strlen(strLegalPath))
        strReturn = strLegalPath;
      ::kodi::addon::CAddonBase::m_interface->toKodi.free_string(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, strLegalPath);
    }
    return strReturn;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline std::string TranslateSpecialProtocol(const std::string& source)
  {
    std::string strReturn;
    char* protocol = ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.translate_special_protocol(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, source.c_str());
    if (protocol != nullptr)
    {
      if (std::strlen(protocol))
        strReturn = protocol;
      ::kodi::addon::CAddonBase::m_interface->toKodi.free_string(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, protocol);
    }
    return strReturn;
  }
  //----------------------------------------------------------------------------
  
  //============================================================================
  ///
  inline std::string GetFileName(const std::string& path)
  {
    /* find the last slash */
    const size_t slash = path.find_last_of("/\\");
    return path.substr(slash+1);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  inline std::string GetDirectoryName(const std::string& path)
  {
    // Will from a full filename return the directory the file resides in.
    // Keeps the final slash at end and possible |option=foo options.

    size_t iPosSlash = path.find_last_of("/\\");
    if (iPosSlash == std::string::npos)
      return ""; // No slash, so no path (ignore any options)

    size_t iPosBar = path.rfind('|');
    if (iPosBar == std::string::npos)
      return path.substr(0, iPosSlash + 1); // Only path

    return path.substr(0, iPosSlash + 1) + path.substr(iPosBar); // Path + options
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  class CFile
  {
  public:
    //==========================================================================
    ///
    CFile() : m_file(nullptr) { }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    virtual ~CFile() { Close(); }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    bool OpenFile(const std::string& filename, unsigned int flags = 0)
    {
      Close();
      m_file = ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.open_file(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, filename.c_str(), flags);
      return m_file != nullptr;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    bool OpenFileForWrite(const std::string& filename, bool overwrite = false)
    {
      Close();

      // Try to open the file. If it fails, check if we need to create the directory first
      // This way we avoid checking if the directory exists every time
      m_file = ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.open_file_for_write(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, filename.c_str(), overwrite);
      if (!m_file)
      {
        std::string cacheDirectory = kodi::vfs::GetDirectoryName(filename);
        if (::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.directory_exists(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, cacheDirectory.c_str()) ||
            ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.create_directory(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, cacheDirectory.c_str()))
          m_file = ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.open_file_for_write(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, filename.c_str(), overwrite);
      }
      return m_file != nullptr;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    bool CURLCreate(const std::string& url)
    {
      m_file = ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.curl_create(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, url.c_str());
      return m_file != nullptr;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    bool CURLAddOption(CURLOptiontype type, const std::string& name, const std::string& value)
    {
      if (!m_file)
      {
        kodi::Log(ADDON_LOG_ERROR, "kodi::vfs::CURLCreate(...) needed to call before!");
        return false;
      }
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.curl_add_option(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file, type, name.c_str(), value.c_str());
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    bool CURLOpen(unsigned int flags = 0)
    {
      if (!m_file)
      {
        kodi::Log(ADDON_LOG_ERROR, "kodi::vfs::CURLCreate(...) needed to call before!");
        return false;
      }
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.curl_open(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file, flags);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    ssize_t Read(void* ptr, size_t size)
    {
      if (!m_file)
        return 0;
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.read_file(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file, ptr, size);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    bool ReadLine(std::string &line)
    {
      line.clear();
      if (!m_file)
        return false;
      // TODO: Read 1024 chars into buffer. If file position advanced that many
      // chars, we didn't hit a newline. Otherwise, if file position is 1 or 2
      // past the number of bytes read, we read (and skipped) a newline sequence.
      char buffer[1025];
      if (::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.read_file_string(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file, buffer, sizeof(buffer)))
      {
        line = buffer;
        return !line.empty();
      }
      return false;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    ssize_t Write(const void* ptr, size_t size)
    {
      if (!m_file)
        return 0;
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.write_file(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file, ptr, size);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    void Flush()
    {
      if (!m_file)
        return;
      ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.flush_file(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    int64_t Seek(int64_t position, int whence = SEEK_SET)
    {
      if (!m_file)
        return 0;
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.seek_file(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file, position, whence);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    int Truncate(int64_t size)
    {
      if (!m_file)
        return -1;
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.truncate_file(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file, size);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    int64_t GetPosition()
    {
      if (!m_file)
        return -1;
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.get_file_position(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    int64_t GetLength()
    {
      if (!m_file)
        return 0;
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.get_file_length(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file);
    }

    //==========================================================================
    ///
    void Close()
    {
      if (!m_file)
        return;
      ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.close_file(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file);
      m_file = nullptr;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    int GetChunkSize()
    {
      if (!m_file)
        return 0;
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.get_file_chunk_size(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    double GetFileDownloadSpeed()
    {
      if (!m_file)
        return 0.0;
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.get_file_download_speed(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, m_file);
    }
    //--------------------------------------------------------------------------

    //============================================================================
    ///
    static inline bool Exists(const std::string& filename, bool usecache = false)
    {
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.file_exists(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, filename.c_str(), usecache);
    }
    //----------------------------------------------------------------------------

    //============================================================================
    ///
    static inline int Stat(const std::string& filename, STAT_STRUCTURE& buffer)
    {
      struct __stat64 frontendBuffer = { };
      if (::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.stat_file(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, filename.c_str(), &frontendBuffer))
      {
        buffer.deviceId         = frontendBuffer.st_dev;
        buffer.size             = frontendBuffer.st_size;
#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
        buffer.accessTime       = frontendBuffer.st_atimespec;
        buffer.modificationTime = frontendBuffer.st_mtimespec;
        buffer.statusTime       = frontendBuffer.st_ctimespec;
#elif defined(TARGET_WINDOWS)
        buffer.accessTime       = frontendBuffer.st_atime;
        buffer.modificationTime = frontendBuffer.st_mtime;
        buffer.statusTime       = frontendBuffer.st_ctime;
#elif defined(TARGET_ANDROID)
        buffer.accessTime.tv_sec = frontendBuffer.st_atime;
        buffer.accessTime.tv_nsec = frontendBuffer.st_atime_nsec;
        buffer.modificationTime.tv_sec = frontendBuffer.st_mtime;
        buffer.modificationTime.tv_nsec = frontendBuffer.st_mtime_nsec;
        buffer.statusTime.tv_sec = frontendBuffer.st_ctime;
        buffer.statusTime.tv_nsec = frontendBuffer.st_ctime_nsec;
#else
        buffer.accessTime       = frontendBuffer.st_atim;
        buffer.modificationTime = frontendBuffer.st_mtim;
        buffer.statusTime       = frontendBuffer.st_ctim;
#endif
        buffer.isDirectory      = S_ISDIR(frontendBuffer.st_mode);
        buffer.isSymLink        = S_ISLNK(frontendBuffer.st_mode);
        buffer.isHidden         = false; // TODO
        return true;
      }
      return false;
    }
    //----------------------------------------------------------------------------

    //============================================================================
    ///
    static inline bool Delete(const std::string& filename)
    {
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.delete_file(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, filename.c_str());
    }
    //----------------------------------------------------------------------------

    //============================================================================
    ///
    static inline bool Rename(const std::string& filename, const std::string& newFileName)
    {
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.rename_file(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, filename.c_str(), newFileName.c_str());
    }
    //----------------------------------------------------------------------------

    //============================================================================
    ///
    static inline bool Copy(const std::string& filename, const std::string& destination)
    {
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.copy_file(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, filename.c_str(), destination.c_str());
    }
    //----------------------------------------------------------------------------

    //============================================================================
    ///
    static inline bool SetHidden(const std::string& filename, bool hidden)
    {
      return ::kodi::addon::CAddonBase::m_interface->toKodi.kodi->filesystem.file_set_hidden(::kodi::addon::CAddonBase::m_interface->toKodi.kodiBase, filename.c_str(), hidden);
    }
    //----------------------------------------------------------------------------

    //============================================================================
    ///
    static inline unsigned int GetChunkSize(unsigned int chunk, unsigned int minimum)
    {
      if (chunk)
        return chunk * ((minimum + chunk - 1) / chunk);
      else
        return minimum;
    }
    //----------------------------------------------------------------------------

  private:
    void* m_file;
  };
  //----------------------------------------------------------------------------

} /* namespace vfs */
} /* namespace kodi */
