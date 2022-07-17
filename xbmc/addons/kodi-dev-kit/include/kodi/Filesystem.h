/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonBase.h"
#include "c-api/filesystem.h"

#ifdef __cplusplus

#include <cstring>
#include <map>
#include <vector>

namespace kodi
{
namespace vfs
{

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// Main page text for filesystem group by Doxygen.
//{{{

//==============================================================================
///
/// @defgroup cpp_kodi_vfs  Interface - kodi::vfs
/// @ingroup cpp
/// @brief **Virtual filesystem functions**\n
/// Offers classes and functions for access to the Virtual File Server (VFS)
/// which you can use to manipulate files and folders.
///
/// This system allow the use of ["Special Protocol"](https://kodi.wiki/view/Special_protocol)
/// where is Kodi's solution to platform dependent directories. Common directory
/// names are assigned a <b>`special://[name]`</b> path which is passed around
/// inside Kodi and then translated to the platform specific path before the
/// operating system sees it. This helps keep most of the platform mess
/// centralized in the code.\n
/// To become a correct path back can be @ref TranslateSpecialProtocol() used.
///
/// It has the header @ref Filesystem.h "#include <kodi/Filesystem.h>" be
/// included to enjoy it.
///
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_vfs_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_vfs
/// @brief **Virtual file Server definition values**\n
/// All to VFS system functions associated data structures.
///
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_vfs_Directory 1. Directory functions
/// @ingroup cpp_kodi_vfs
/// @brief **Globally available directories related functions**\n
/// Used to perform typical operations with it.
///
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_vfs_File 2. File functions
/// @ingroup cpp_kodi_vfs
/// @brief **Globally available file related functions**\n
/// Used to perform typical operations with it.
///
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_vfs_General 3. General functions
/// @ingroup cpp_kodi_vfs
/// @brief **Other globally available functions**\n
/// Used to perform typical operations with it.
///
//------------------------------------------------------------------------------

//}}}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" related filesystem definitions
//{{{

//==============================================================================
/// @defgroup cpp_kodi_vfs_Defs_FileStatus class FileStatus
/// @ingroup cpp_kodi_vfs_Defs
/// @brief **File information status**\n
/// Used on kodi::vfs::StatFile() to get detailed information about a file.
///
///@{
class ATTR_DLL_LOCAL FileStatus : public kodi::addon::CStructHdl<FileStatus, STAT_STRUCTURE>
{
public:
  /*! \cond PRIVATE */
  FileStatus() { memset(m_cStructure, 0, sizeof(STAT_STRUCTURE)); }
  FileStatus(const FileStatus& channel) : CStructHdl(channel) {}
  FileStatus(const STAT_STRUCTURE* channel) : CStructHdl(channel) {}
  FileStatus(STAT_STRUCTURE* channel) : CStructHdl(channel) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_vfs_Defs_FileStatus_Help Value Help
  /// @ingroup cpp_kodi_vfs_Defs_FileStatus
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_vfs_Defs_FileStatus :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **ID of device containing file** | `uint32_t` | @ref FileStatus::SetDeviceId "SetDeviceId" | @ref FileStatus::GetDeviceId "GetDeviceId"
  /// | **Represent file serial numbers** | `uint64_t` | @ref FileStatus::SetFileSerialNumber "SetFileSerialNumber" | @ref FileStatus::GetFileSerialNumber "GetFileSerialNumber"
  /// | **Total size, in bytes** | `uint64_t` | @ref FileStatus::SetSize "SetSize" | @ref FileStatus::GetSize "GetSize"
  /// | **Time of last access** | `time_t` | @ref FileStatus::SetAccessTime "SetAccessTime" | @ref FileStatus::GetAccessTime "GetAccessTime"
  /// | **Time of last modification** | `time_t` | @ref FileStatus::SetModificationTime "SetModificationTime" | @ref FileStatus::GetModificationTime "GetModificationTime"
  /// | **Time of last status change** | `time_t` | @ref FileStatus::SetStatusTime "SetStatusTime" | @ref FileStatus::GetStatusTime "GetStatusTime"
  /// | **Stat url is a directory** | `bool` | @ref FileStatus::SetIsDirectory "SetIsDirectory" | @ref FileStatus::GetIsDirectory "GetIsDirectory"
  /// | **Stat url as a symbolic link** | `bool` | @ref FileStatus::SetIsSymLink "SetIsSymLink" | @ref FileStatus::GetIsSymLink "GetIsSymLink"
  /// | **Stat url as a block special** | `bool` | @ref FileStatus::SetIsBlock "SetIsBlock" | @ref FileStatus::GetIsBlock "GetIsBlock"
  /// | **Stat url as a character special** | `bool` | @ref FileStatus::SetIsCharacter "SetIsCharacter" | @ref FileStatus::GetIsCharacter "GetIsCharacter"
  /// | **Stat url as a FIFO special** | `bool` | @ref FileStatus::SetIsFifo "SetIsFifo" | @ref FileStatus::GetIsFifo "GetIsFifo"
  /// | **Stat url as a regular** | `bool` | @ref FileStatus::SetIsRegular "SetIsRegular" | @ref FileStatus::GetIsRegular "GetIsRegular"
  /// | **Stat url as a socket** | `bool` | @ref FileStatus::SetIsSocket "SetIsSocket" | @ref FileStatus::GetIsSocket "GetIsSocket"
  ///

  /// @addtogroup cpp_kodi_vfs_Defs_FileStatus
  /// @copydetails cpp_kodi_vfs_Defs_FileStatus_Help
  ///@{

  /// @brief Set ID of device containing file.
  void SetDeviceId(uint32_t deviceId) { m_cStructure->deviceId = deviceId; }

  /// @brief Get ID of device containing file.
  uint32_t GetDeviceId() const { return m_cStructure->deviceId; }

  /// @brief Set the file serial number, which distinguishes this file from all other files on the same device.
  void SetFileSerialNumber(uint64_t fileSerialNumber)
  {
    m_cStructure->fileSerialNumber = fileSerialNumber;
  }

  /// @brief Get the file serial number, which distinguishes this file from all other files on the same device.
  uint64_t GetFileSerialNumber() const { return m_cStructure->fileSerialNumber; }

  /// @brief Set total size, in bytes.
  void SetSize(uint64_t size) { m_cStructure->size = size; }

  /// @brief Get total size, in bytes.
  uint64_t GetSize() const { return m_cStructure->size; }

  /// @brief Set time of last access.
  void SetAccessTime(time_t accessTime) { m_cStructure->accessTime = accessTime; }

  /// @brief Get time of last access.
  time_t GetAccessTime() const { return m_cStructure->accessTime; }

  /// @brief Set time of last modification.
  void SetModificationTime(time_t modificationTime)
  {
    m_cStructure->modificationTime = modificationTime;
  }

  /// @brief Get time of last modification.
  time_t GetModificationTime() const { return m_cStructure->modificationTime; }

  /// @brief Set time of last status change.
  void SetStatusTime(time_t statusTime) { m_cStructure->statusTime = statusTime; }

  /// @brief Get time of last status change.
  time_t GetStatusTime() const { return m_cStructure->statusTime; }

  /// @brief Set the stat url is a directory.
  void SetIsDirectory(bool isDirectory) { m_cStructure->isDirectory = isDirectory; }

  /// @brief The stat url is a directory if returns true.
  bool GetIsDirectory() const { return m_cStructure->isDirectory; }

  /// @brief Set stat url as a symbolic link.
  void SetIsSymLink(bool isSymLink) { m_cStructure->isSymLink = isSymLink; }

  /// @brief Get stat url is a symbolic link.
  bool GetIsSymLink() const { return m_cStructure->isSymLink; }

  /// @brief Set stat url as a block special.
  void SetIsBlock(bool isBlock) { m_cStructure->isBlock = isBlock; }

  /// @brief Get stat url is a block special.
  bool GetIsBlock() const { return m_cStructure->isBlock; }

  /// @brief Set stat url as a character special.
  void SetIsCharacter(bool isCharacter) { m_cStructure->isCharacter = isCharacter; }

  /// @brief Get stat url is a character special.
  bool GetIsCharacter() const { return m_cStructure->isCharacter; }

  /// @brief Set stat url as a FIFO special.
  void SetIsFifo(bool isFifo) { m_cStructure->isFifo = isFifo; }

  /// @brief Get stat url is a FIFO special.
  bool GetIsFifo() const { return m_cStructure->isFifo; }

  /// @brief Set stat url as a regular.
  void SetIsRegular(bool isRegular) { m_cStructure->isRegular = isRegular; }

  /// @brief Get stat url is a regular.
  bool GetIsRegular() const { return m_cStructure->isRegular; }

  /// @brief Set stat url is a socket.
  void SetIsSocket(bool isSocket) { m_cStructure->isSocket = isSocket; }

  /// @brief Get stat url is a regular.
  bool GetIsSocket() const { return m_cStructure->isSocket; }
  ///@}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_vfs_Defs_CacheStatus class CacheStatus
/// @ingroup cpp_kodi_vfs_Defs
/// @brief **Cache information status**\n
/// Used on kodi::vfs::CFile::IoControlGetCacheStatus() to get running cache
/// status of processed stream.
///
///@{
class ATTR_DLL_LOCAL CacheStatus
  : public kodi::addon::CStructHdl<CacheStatus, VFS_CACHE_STATUS_DATA>
{
public:
  /*! \cond PRIVATE */
  CacheStatus() { memset(m_cStructure, 0, sizeof(VFS_CACHE_STATUS_DATA)); }
  CacheStatus(const CacheStatus& channel) : CStructHdl(channel) {}
  CacheStatus(const VFS_CACHE_STATUS_DATA* channel) : CStructHdl(channel) {}
  CacheStatus(VFS_CACHE_STATUS_DATA* channel) : CStructHdl(channel) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_vfs_Defs_CacheStatus_Help Value Help
  /// @ingroup cpp_kodi_vfs_Defs_CacheStatus
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_vfs_Defs_CacheStatus :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Number of bytes cached** | `uint64_t` | @ref CacheStatus::SetForward "SetForward" | @ref CacheStatus::GetForward "GetForward"
  /// | **Maximum number of bytes per second** | `uint32_t` | @ref CacheStatus::SetMaxRate "SetMaxRate" | @ref CacheStatus::GetMaxRate "GetMaxRate"
  /// | **Average read rate from source file** | `uint32_t` | @ref CacheStatus::SetCurrentRate "SetCurrentRate" | @ref CacheStatus::GetCurrentRate "GetCurrentRate"
  /// | **Cache low speed rate detected** | `uint32_t` | @ref CacheStatus::SetLowRate "SetLowRate" | @ref CacheStatus::GetLowRate "GetLowRate"
  ///

  /// @addtogroup cpp_kodi_vfs_Defs_CacheStatus
  /// @copydetails cpp_kodi_vfs_Defs_CacheStatus_Help
  ///@{

  /// @brief Set number of bytes cached forward of current position.
  void SetForward(uint64_t forward) { m_cStructure->forward = forward; }

  /// @brief Get number of bytes cached forward of current position.
  uint64_t GetForward() { return m_cStructure->forward; }

  /// @brief Set maximum number of bytes per second cache is allowed to fill.
  void SetMaxRate(uint32_t maxrate) { m_cStructure->maxrate = maxrate; }

  /// @brief Set maximum number of bytes per second cache is allowed to fill.
  uint32_t GetMaxRate() { return m_cStructure->maxrate; }

  /// @brief Set number of bytes per second for average read rate from source file since last position change.
  void SetCurrentRate(uint32_t currate) { m_cStructure->currate = currate; }

  /// @brief Get number of bytes per second for average read rate from source file since last position change.
  uint32_t GetCurrentRate() { return m_cStructure->currate; }

  /// @brief Set number of bytes per second for low speed rate.
  void SetLowRate(uint32_t lowrate) { m_cStructure->lowrate = lowrate; }

  /// @brief Get number of bytes per second for low speed rate.
  uint32_t GetLowRate() { return m_cStructure->lowrate; }

  ///@}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_vfs_Defs_HttpHeader class HttpHeader
/// @ingroup cpp_kodi_vfs_Defs
/// @brief **HTTP header information**\n
/// The class used to access HTTP header information and get his information.
///
/// Used on @ref kodi::vfs::GetHttpHeader().
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_vfs_Defs_HttpHeader_Help
///
///@{
class ATTR_DLL_LOCAL HttpHeader
{
public:
  //==========================================================================
  /// @brief Http header parser class constructor.
  ///
  HttpHeader()
  {
    using namespace ::kodi::addon;

    CPrivateBase::m_interface->toKodi->kodi_filesystem->http_header_create(
        CPrivateBase::m_interface->toKodi->kodiBase, &m_handle);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @brief Class destructor.
  ///
  ~HttpHeader()
  {
    using namespace ::kodi::addon;

    CPrivateBase::m_interface->toKodi->kodi_filesystem->http_header_free(
        CPrivateBase::m_interface->toKodi->kodiBase, &m_handle);
  }
  //--------------------------------------------------------------------------

  /// @defgroup cpp_kodi_vfs_Defs_HttpHeader_Help Value Help
  /// @ingroup cpp_kodi_vfs_Defs_HttpHeader
  ///
  /// <b>The following table contains values that can be get with @ref cpp_kodi_vfs_Defs_HttpHeader :</b>
  /// | Description | Type | Get call
  /// |-------------|------|------------
  /// | **Get the value associated with this parameter of these HTTP headers** | `std::string` | @ref HttpHeader::GetValue "GetValue"
  /// | **Get the values as list associated with this parameter of these HTTP headers** | `std::vector<std::string>` | @ref HttpHeader::GetValues "GetValues"
  /// | **Get the full header string associated with these HTTP headers** | `std::string` | @ref HttpHeader::GetHeader "GetHeader"
  /// | **Get the mime type associated with these HTTP headers** | `std::string` | @ref HttpHeader::GetMimeType "GetMimeType"
  /// | **Get the charset associated with these HTTP headers** | `std::string` | @ref HttpHeader::GetCharset "GetCharset"
  /// | **The protocol line associated with these HTTP headers** | `std::string` | @ref HttpHeader::GetProtoLine "GetProtoLine"
  ///

  /// @addtogroup cpp_kodi_vfs_Defs_HttpHeader
  ///@{

  //==========================================================================
  /// @brief Get the value associated with this parameter of these HTTP
  /// headers.
  ///
  /// @param[in] param The name of the parameter a value is required for
  /// @return The value found
  ///
  std::string GetValue(const std::string& param) const
  {
    using namespace ::kodi::addon;

    if (!m_handle.handle)
      return "";

    std::string protoLine;
    char* string = m_handle.get_value(CPrivateBase::m_interface->toKodi->kodiBase, m_handle.handle,
                                      param.c_str());
    if (string != nullptr)
    {
      protoLine = string;
      CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                     string);
    }
    return protoLine;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @brief Get the values as list associated with this parameter of these
  /// HTTP headers.
  ///
  /// @param[in] param The name of the parameter values are required for
  /// @return The values found
  ///
  std::vector<std::string> GetValues(const std::string& param) const
  {
    using namespace kodi::addon;

    if (!m_handle.handle)
      return std::vector<std::string>();

    int numValues = 0;
    char** res(m_handle.get_values(CPrivateBase::m_interface->toKodi->kodiBase, m_handle.handle,
                                   param.c_str(), &numValues));
    if (res)
    {
      std::vector<std::string> vecReturn;
      vecReturn.reserve(numValues);
      for (int i = 0; i < numValues; ++i)
      {
        vecReturn.emplace_back(res[i]);
      }
      CPrivateBase::m_interface->toKodi->free_string_array(
          CPrivateBase::m_interface->toKodi->kodiBase, res, numValues);
      return vecReturn;
    }
    return std::vector<std::string>();
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @brief Get the full header string associated with these HTTP headers.
  ///
  /// @return The header as a string
  ///
  std::string GetHeader() const
  {
    using namespace ::kodi::addon;

    if (!m_handle.handle)
      return "";

    std::string header;
    char* string =
        m_handle.get_header(CPrivateBase::m_interface->toKodi->kodiBase, m_handle.handle);
    if (string != nullptr)
    {
      header = string;
      CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                     string);
    }
    return header;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @brief Get the mime type associated with these HTTP headers.
  ///
  /// @return The mime type
  ///
  std::string GetMimeType() const
  {
    using namespace ::kodi::addon;

    if (!m_handle.handle)
      return "";

    std::string protoLine;
    char* string =
        m_handle.get_mime_type(CPrivateBase::m_interface->toKodi->kodiBase, m_handle.handle);
    if (string != nullptr)
    {
      protoLine = string;
      CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                     string);
    }
    return protoLine;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @brief Get the charset associated with these HTTP headers.
  ///
  /// @return The charset
  ///
  std::string GetCharset() const
  {
    using namespace ::kodi::addon;

    if (!m_handle.handle)
      return "";

    std::string protoLine;
    char* string =
        m_handle.get_charset(CPrivateBase::m_interface->toKodi->kodiBase, m_handle.handle);
    if (string != nullptr)
    {
      protoLine = string;
      CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                     string);
    }
    return protoLine;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @brief The protocol line associated with these HTTP headers.
  ///
  /// @return The protocol line
  ///
  std::string GetProtoLine() const
  {
    using namespace ::kodi::addon;

    if (!m_handle.handle)
      return "";

    std::string protoLine;
    char* string =
        m_handle.get_proto_line(CPrivateBase::m_interface->toKodi->kodiBase, m_handle.handle);
    if (string != nullptr)
    {
      protoLine = string;
      CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                     string);
    }
    return protoLine;
  }
  //--------------------------------------------------------------------------

  ///@}

  KODI_HTTP_HEADER m_handle;
};
///@}
//----------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_vfs_CDirEntry class CDirEntry
/// @ingroup cpp_kodi_vfs_Defs
///
/// @brief **Virtual file server directory entry**\n
/// This class is used as an entry for files and folders in
/// kodi::vfs::GetDirectory().
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
///
/// ...
///
/// std::vector<kodi::vfs::CDirEntry> items;
/// kodi::vfs::GetDirectory("special://temp", "", items);
///
/// fprintf(stderr, "Directory have %lu entries\n", items.size());
/// for (unsigned long i = 0; i < items.size(); i++)
/// {
///   char buff[20];
///   time_t now = items[i].DateTime();
///   strftime(buff, 20, "%Y-%m-%d %H:%M:%S", gmtime(&now));
///   fprintf(stderr, " - %04lu -- Folder: %s -- Name: %s -- Path: %s -- Time: %s\n",
///             i+1,
///             items[i].IsFolder() ? "yes" : "no ",
///             items[i].Label().c_str(),
///             items[i].Path().c_str(),
///             buff);
/// }
/// ~~~~~~~~~~~~~
///
/// It has the header @ref Filesystem.h "#include <kodi/Filesystem.h>" be included
/// to enjoy it.
///
///@{
class ATTR_DLL_LOCAL CDirEntry
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_vfs_CDirEntry
  /// @brief Constructor for VFS directory entry
  ///
  /// @param[in] label    [opt] Name to use for entry
  /// @param[in] path     [opt] Used path of the entry
  /// @param[in] folder   [opt] If set entry used as folder
  /// @param[in] size     [opt] If used as file, his size defined there
  /// @param[in] dateTime [opt] Date time of the entry
  ///
  CDirEntry(const std::string& label = "",
            const std::string& path = "",
            bool folder = false,
            int64_t size = -1,
            time_t dateTime = 0)
    : m_label(label), m_path(path), m_folder(folder), m_size(size), m_dateTime(dateTime)
  {
  }
  //----------------------------------------------------------------------------

  //============================================================================
  // @note Not for addon development itself needed, thats why below is
  // disabled for doxygen!
  //
  // @ingroup cpp_kodi_vfs_CDirEntry
  // @brief Constructor to create own copy
  //
  // @param[in] dirEntry pointer to own class type
  //
  explicit CDirEntry(const VFSDirEntry& dirEntry)
    : m_label(dirEntry.label ? dirEntry.label : ""),
      m_path(dirEntry.path ? dirEntry.path : ""),
      m_folder(dirEntry.folder),
      m_size(dirEntry.size),
      m_dateTime(dirEntry.date_time)
  {
  }
  //----------------------------------------------------------------------------

  /// @defgroup cpp_kodi_vfs_CDirEntry_Help Value Help
  /// @ingroup cpp_kodi_vfs_CDirEntry
  /// --------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_vfs_CDirEntry :</b>
  /// | Name | Type | Set call | Get call | Clear call |
  /// |------|------|----------|----------|------------|
  /// | **Directory entry name** | `std::string` | @ref CDirEntry::SetLabel "SetLabel" | @ref CDirEntry::Label "Label" | |
  /// | **Title of entry** | `std::string` | @ref CDirEntry::SetTitle "SetTitle" | @ref CDirEntry::Title "Title" | |
  /// | **Path of the entry** | `std::string` | @ref CDirEntry::SetPath "SetPath" | @ref CDirEntry::Path "Path" | |
  /// | **Entry is folder** | `bool` | @ref CDirEntry::SetFolder "SetFolder" | @ref CDirEntry::IsFolder "IsFolder" | |
  /// | **The size of the file** | `int64_t` | @ref CDirEntry::SetSize "SetSize" | @ref CDirEntry::Size "Size" | |
  /// | **File time and date** | `time_t` | @ref CDirEntry::SetDateTime "SetDateTime" | @ref CDirEntry::DateTime "DateTime" | |
  /// | **Property entries** | `std::string, std::string` | @ref CDirEntry::AddProperty "AddProperty" | @ref CDirEntry::GetProperties "GetProperties" | @ref CDirEntry::ClearProperties "ClearProperties"
  ///

  /// @addtogroup cpp_kodi_vfs_CDirEntry
  /// @copydetails cpp_kodi_vfs_CDirEntry_Help
  ///@{

  //============================================================================
  /// @brief Get the directory entry name.
  ///
  /// @return Name of the entry
  ///
  const std::string& Label(void) const { return m_label; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the optional title of entry.
  ///
  /// @return Title of the entry, if exists
  ///
  const std::string& Title(void) const { return m_title; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the path of the entry.
  ///
  /// @return File system path of the entry
  ///
  const std::string& Path(void) const { return m_path; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Used to check entry is folder.
  ///
  /// @return true if entry is a folder
  ///
  bool IsFolder(void) const { return m_folder; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief If file, the size of the file.
  ///
  /// @return Defined file size
  ///
  int64_t Size(void) const { return m_size; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get file time and date for a new entry.
  ///
  /// @return The with time_t defined date and time of file
  ///
  time_t DateTime() { return m_dateTime; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set the label name.
  ///
  /// @param[in] label name of entry
  ///
  void SetLabel(const std::string& label) { m_label = label; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set the title name.
  ///
  /// @param[in] title title name of entry
  ///
  void SetTitle(const std::string& title) { m_title = title; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set the path of the entry.
  ///
  /// @param[in] path path of entry
  ///
  void SetPath(const std::string& path) { m_path = path; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set the entry defined as folder.
  ///
  /// @param[in] folder If true becomes entry defined as folder
  ///
  void SetFolder(bool folder) { m_folder = folder; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set a file size for a new entry.
  ///
  /// @param[in] size Size to set for dir entry
  ///
  void SetSize(int64_t size) { m_size = size; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set file time and date for a new entry.
  ///
  /// @param[in] dateTime The with time_t defined date and time of file
  ///
  void SetDateTime(time_t dateTime) { m_dateTime = dateTime; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Add a by string defined property entry to directory entry.
  ///
  /// @note A property can be used to add some special information about a file
  /// or directory entry, this can be used on other places to do the right work
  /// of them.
  ///
  /// @param[in] id     Identification name of property
  /// @param[in] value  The property value to add by given id
  ///
  void AddProperty(const std::string& id, const std::string& value) { m_properties[id] = value; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Clear all present properties.
  ///
  void ClearProperties() { m_properties.clear(); }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the present properties list on directory entry.
  ///
  /// @return map with all present properties
  ///
  const std::map<std::string, std::string>& GetProperties() const { return m_properties; }
  //----------------------------------------------------------------------------

  ///@}

private:
  std::string m_label;
  std::string m_title;
  std::string m_path;
  std::map<std::string, std::string> m_properties;
  bool m_folder;
  int64_t m_size;
  time_t m_dateTime;
};
///@}
//------------------------------------------------------------------------------

//}}}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Directory related functions
//{{{

//==============================================================================
/// @ingroup cpp_kodi_vfs_Directory
/// @brief Make a directory.
///
/// The kodi::vfs::CreateDirectory() function shall create a
/// new directory with name path.
///
/// The newly created directory shall be an empty directory.
///
/// @param[in] path Path to the directory.
/// @return Upon successful completion, CreateDirectory() shall return true.
///         Otherwise false shall be returned, no directory shall be created.
///
///
/// -------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// std::string directory = "C:\\my_dir";
/// bool ret = kodi::vfs::CreateDirectory(directory);
/// fprintf(stderr, "Directory '%s' successfully created: %s\n", directory.c_str(), ret ? "yes" : "no");
/// ...
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL CreateDirectory(const std::string& path)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->create_directory(
      CPrivateBase::m_interface->toKodi->kodiBase, path.c_str());
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_Directory
/// @brief Verifying the Existence of a Directory.
///
/// The kodi::vfs::DirectoryExists() method determines whether
/// a specified folder exists.
///
/// @param[in] path Path to the directory.
/// @return True when it exists, false otherwise.
///
///
/// -------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// std::string directory = "C:\\my_dir";
/// bool ret = kodi::vfs::DirectoryExists(directory);
/// fprintf(stderr, "Directory '%s' present: %s\n", directory.c_str(), ret ? "yes" : "no");
/// ...
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL DirectoryExists(const std::string& path)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->directory_exists(
      CPrivateBase::m_interface->toKodi->kodiBase, path.c_str());
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_Directory
/// @brief Removes a directory.
///
/// The kodi::vfs::RemoveDirectory() function shall remove a
/// directory whose name is given by path.
///
/// @param[in] path Path to the directory.
/// @param[in] recursive [opt] Remove directory recursive (default is false)
/// @return Upon successful completion, the function RemoveDirectory() shall
///         return true. Otherwise, false shall be returned, and errno set
///         to indicate the error. If false is returned, the named directory
///         shall not be changed.
///
///
/// -------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// bool ret = kodi::vfs::RemoveDirectory("C:\\my_dir");
/// ...
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL RemoveDirectory(const std::string& path, bool recursive = false)
{
  using namespace kodi::addon;

  if (!recursive)
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->remove_directory(
        CPrivateBase::m_interface->toKodi->kodiBase, path.c_str());
  else
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->remove_directory_recursive(
        CPrivateBase::m_interface->toKodi->kodiBase, path.c_str());
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_Directory
/// @brief Lists a directory.
///
/// Return the list of files and directories which have been found in the
/// specified directory and which respect the given constraint.
///
/// It can handle the normal OS dependent paths and also the special virtual
/// filesystem from Kodi what starts with \b special://.
///
/// @param[in]  path  The path in which the files and directories are located.
/// @param[in]  mask  Mask to filter out requested files, e.g. "*.avi|*.mpg" to
///                   files with this ending.
/// @param[out] items The returned list directory entries.
/// @return           True if listing was successful, false otherwise.
///
///
/// -------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
///
/// std::vector<kodi::vfs::CDirEntry> items;
/// kodi::vfs::GetDirectory("special://temp", "", items);
///
/// fprintf(stderr, "Directory have %lu entries\n", items.size());
/// for (unsigned long i = 0; i < items.size(); i++)
/// {
///   fprintf(stderr, " - %04lu -- Folder: %s -- Name: %s -- Path: %s\n",
///             i+1,
///             items[i].IsFolder() ? "yes" : "no ",
///             items[i].Label().c_str(),
///             items[i].Path().c_str());
/// }
/// ~~~~~~~~~~~~~
inline bool ATTR_DLL_LOCAL GetDirectory(const std::string& path,
                                        const std::string& mask,
                                        std::vector<kodi::vfs::CDirEntry>& items)
{
  using namespace kodi::addon;

  VFSDirEntry* dir_list = nullptr;
  unsigned int num_items = 0;
  if (CPrivateBase::m_interface->toKodi->kodi_filesystem->get_directory(
          CPrivateBase::m_interface->toKodi->kodiBase, path.c_str(), mask.c_str(), &dir_list,
          &num_items))
  {
    if (dir_list)
    {
      for (unsigned int i = 0; i < num_items; ++i)
        items.emplace_back(dir_list[i]);

      CPrivateBase::m_interface->toKodi->kodi_filesystem->free_directory(
          CPrivateBase::m_interface->toKodi->kodiBase, dir_list, num_items);
    }

    return true;
  }
  return false;
}
//------------------------------------------------------------------------------

//}}}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" File related functions
//{{{

//==============================================================================
/// @ingroup cpp_kodi_vfs_File
/// @brief Check if a file exists.
///
/// @param[in] filename The filename to check.
/// @param[in] usecache Check in file cache.
/// @return true if the file exists false otherwise.
///
///
/// -------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// bool exists = kodi::vfs::FileExists("special://temp/kodi.log");
/// fprintf(stderr, "Log file should be always present, is it present? %s\n", exists ? "yes" : "no");
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL FileExists(const std::string& filename, bool usecache = false)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->file_exists(
      CPrivateBase::m_interface->toKodi->kodiBase, filename.c_str(), usecache);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_File
/// @brief Get file status.
///
/// These function return information about a file. Execute (search)
/// permission is required on all of the directories in path that
/// lead to the file.
///
/// The call return a stat structure, which contains the on
/// @ref cpp_kodi_vfs_Defs_FileStatus defined values.
///
/// @warning Not all of the OS file systems implement all of the time fields.
///
/// @param[in] filename The filename to read the status from.
/// @param[out] buffer The file status is written into this buffer.
/// @return On success, trur is returned. On error, false is returned
///
///
/// @copydetails cpp_kodi_vfs_Defs_FileStatus_Help
///
/// -------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// kodi::vfs::FileStatus statFile;
/// int ret = kodi::vfs::StatFile("special://temp/kodi.log", statFile);
/// fprintf(stderr, "deviceId (ID of device containing file)       = %u\n"
///                 "size (total size, in bytes)                   = %lu\n"
///                 "accessTime (time of last access)              = %lu\n"
///                 "modificationTime (time of last modification)  = %lu\n"
///                 "statusTime (time of last status change)       = %lu\n"
///                 "isDirectory (The stat url is a directory)     = %s\n"
///                 "isSymLink (The stat url is a symbolic link)   = %s\n"
///                 "Return value                                  = %i\n",
///                      statFile.GetDeviceId(),
///                      statFile.GetSize(),
///                      statFile.GetAccessTime(),
///                      statFile.GetModificationTime(),
///                      statFile.GetStatusTime(),
///                      statFile.GetIsDirectory() ? "true" : "false",
///                      statFile.GetIsSymLink() ? "true" : "false",
///                      ret);
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL StatFile(const std::string& filename, kodi::vfs::FileStatus& buffer)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->stat_file(
      CPrivateBase::m_interface->toKodi->kodiBase, filename.c_str(), buffer);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_File
/// @brief Deletes a file.
///
/// @param[in] filename The filename to delete.
/// @return The file was successfully deleted.
///
///
/// -------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// #include <kodi/gui/DialogFileBrowser.h>
/// #include <kodi/gui/DialogOK.h>
/// ...
/// std::string filename;
/// if (kodi::gui::DialogFileBrowser::ShowAndGetFile("local", "",
///                                                  "Test File selection and delete of them!",
///                                                  filename))
/// {
///   bool successed = kodi::vfs::DeleteFile(filename);
///   if (!successed)
///     kodi::gui::DialogOK::ShowAndGetInput("Error", "Delete of File", filename, "failed!");
///   else
///     kodi::gui::DialogOK::ShowAndGetInput("Information", "Delete of File", filename, "successfully done.");
/// }
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL DeleteFile(const std::string& filename)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->delete_file(
      CPrivateBase::m_interface->toKodi->kodiBase, filename.c_str());
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_File
/// @brief Rename a file name.
///
/// @param[in] filename The filename to copy.
/// @param[in] newFileName The new filename
/// @return true if successfully renamed
///
///
inline bool ATTR_DLL_LOCAL RenameFile(const std::string& filename, const std::string& newFileName)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->rename_file(
      CPrivateBase::m_interface->toKodi->kodiBase, filename.c_str(), newFileName.c_str());
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_File
/// @brief Copy a file from source to destination.
///
/// @param[in] filename The filename to copy.
/// @param[in] destination The destination to copy file to
/// @return true if successfully copied
///
///
inline bool ATTR_DLL_LOCAL CopyFile(const std::string& filename, const std::string& destination)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->copy_file(
      CPrivateBase::m_interface->toKodi->kodiBase, filename.c_str(), destination.c_str());
}
//------------------------------------------------------------------------------

//}}}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" General filesystem functions
//{{{

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Retrieve MD5sum of a file.
///
/// @param[in] path Path to the file to MD5sum
/// @return MD5 sum of the file
///
///
/// -------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// #include <kodi/gui/DialogFileBrowser.h>
/// ...
/// std::string md5;
/// std::string filename;
/// if (kodi::gui::DialogFileBrowser::ShowAndGetFile("local", "*.avi|*.mpg|*.mp4",
///                                                "Test File selection to get MD5",
///                                                filename))
/// {
///   md5 = kodi::vfs::GetFileMD5(filename);
///   fprintf(stderr, "MD5 of file '%s' is %s\n", md5.c_str(), filename.c_str());
/// }
/// ~~~~~~~~~~~~~
///
inline std::string ATTR_DLL_LOCAL GetFileMD5(const std::string& path)
{
  using namespace kodi::addon;

  std::string strReturn;
  char* strMd5 = CPrivateBase::m_interface->toKodi->kodi_filesystem->get_file_md5(
      CPrivateBase::m_interface->toKodi->kodiBase, path.c_str());
  if (strMd5 != nullptr)
  {
    if (std::strlen(strMd5))
      strReturn = strMd5;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   strMd5);
  }
  return strReturn;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Returns a thumb cache filename.
///
/// @param[in] filename Path to file
/// @return Cache filename
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// #include <kodi/gui/DialogFileBrowser.h>
/// ...
/// std::string thumb;
/// std::string filename;
/// if (kodi::gui::DialogFileBrowser::ShowAndGetFile("local", "*.avi|*.mpg|*.mp4",
///                                                "Test File selection to get Thumnail",
///                                                filename))
/// {
///   thumb = kodi::vfs::GetCacheThumbName(filename);
///   fprintf(stderr, "Thumb name of file '%s' is %s\n", thumb.c_str(), filename.c_str());
/// }
/// ~~~~~~~~~~~~~
///
inline std::string ATTR_DLL_LOCAL GetCacheThumbName(const std::string& filename)
{
  using namespace kodi::addon;

  std::string strReturn;
  char* strThumbName = CPrivateBase::m_interface->toKodi->kodi_filesystem->get_cache_thumb_name(
      CPrivateBase::m_interface->toKodi->kodiBase, filename.c_str());
  if (strThumbName != nullptr)
  {
    if (std::strlen(strThumbName))
      strReturn = strThumbName;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   strThumbName);
  }
  return strReturn;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Make filename valid.
///
/// Function to replace not valid characters with '_'. It can be also
/// compared with original before in a own loop until it is equal
/// (no invalid characters).
///
/// @param[in] filename Filename to check and fix
/// @return The legal filename
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// std::string fileName = "///\\jk???lj????.mpg";
/// std::string legalName = kodi::vfs::MakeLegalFileName(fileName);
/// fprintf(stderr, "Legal name of '%s' is '%s'\n", fileName.c_str(), legalName.c_str());
///
/// /* Returns as legal: 'jk___lj____.mpg' */
/// ~~~~~~~~~~~~~
///
inline std::string ATTR_DLL_LOCAL MakeLegalFileName(const std::string& filename)
{
  using namespace kodi::addon;

  std::string strReturn;
  char* strLegalFileName = CPrivateBase::m_interface->toKodi->kodi_filesystem->make_legal_filename(
      CPrivateBase::m_interface->toKodi->kodiBase, filename.c_str());
  if (strLegalFileName != nullptr)
  {
    if (std::strlen(strLegalFileName))
      strReturn = strLegalFileName;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   strLegalFileName);
  }
  return strReturn;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Make directory name valid.
///
/// Function to replace not valid characters with '_'. It can be also
/// compared with original before in a own loop until it is equal
/// (no invalid characters).
///
/// @param[in] path Directory name to check and fix
/// @return The legal directory name
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// std::string path = "///\\jk???lj????\\hgjkg";
/// std::string legalPath = kodi::vfs::MakeLegalPath(path);
/// fprintf(stderr, "Legal name of '%s' is '%s'\n", path.c_str(), legalPath.c_str());
///
/// /* Returns as legal: '/jk___lj____/hgjkg' */
/// ~~~~~~~~~~~~~
///
inline std::string ATTR_DLL_LOCAL MakeLegalPath(const std::string& path)
{
  using namespace kodi::addon;

  std::string strReturn;
  char* strLegalPath = CPrivateBase::m_interface->toKodi->kodi_filesystem->make_legal_path(
      CPrivateBase::m_interface->toKodi->kodiBase, path.c_str());
  if (strLegalPath != nullptr)
  {
    if (std::strlen(strLegalPath))
      strReturn = strLegalPath;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   strLegalPath);
  }
  return strReturn;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Returns the translated path.
///
/// @param[in] source String or unicode - Path to format
/// @return A human-readable string suitable for logging
///
/// @note Only useful if you are coding for both Linux and Windows. e.g.
/// Converts 'special://masterprofile/script_data' ->
/// '/home/user/.kodi/UserData/script_data' on Linux.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// std::string path = kodi::vfs::TranslateSpecialProtocol("special://masterprofile/script_data");
/// fprintf(stderr, "Translated path is: %s\n", path.c_str());
/// ...
/// ~~~~~~~~~~~~~
/// or
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// fprintf(stderr, "Directory 'special://temp' is '%s'\n", kodi::vfs::TranslateSpecialProtocol("special://temp").c_str());
/// ...
/// ~~~~~~~~~~~~~
///
inline std::string ATTR_DLL_LOCAL TranslateSpecialProtocol(const std::string& source)
{
  using namespace kodi::addon;

  std::string strReturn;
  char* protocol = CPrivateBase::m_interface->toKodi->kodi_filesystem->translate_special_protocol(
      CPrivateBase::m_interface->toKodi->kodiBase, source.c_str());
  if (protocol != nullptr)
  {
    if (std::strlen(protocol))
      strReturn = protocol;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   protocol);
  }
  return strReturn;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Retrieves information about the amount of space that is available on
/// a disk volume.
///
/// Path can be also with Kodi's special protocol.
///
/// @param[in] path Path for where to check
/// @param[out] capacity The total number of bytes in the file system
/// @param[out] free The total number of free bytes in the file system
/// @param[out] available The total number of free bytes available to a
///                       non-privileged process
/// @return true if successfully done and set
///
/// @warning This only works with paths belonging to OS. If <b>"special://"</b>
/// is used, it must point to a place on your own OS.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <climits> // for ULLONG_MAX
/// #include <kodi/Filesystem.h>
/// ...
/// std::string path = "special://temp";
/// uint64_t capacity = ULLONG_MAX;
/// uint64_t free = ULLONG_MAX;
/// uint64_t available = ULLONG_MAX;
/// kodi::vfs::GetDiskSpace(path, capacity, free, available);
/// fprintf(stderr, "Path '%s' sizes:\n", path.c_str());
/// fprintf(stderr, " - capacity:  %lu MByte\n", capacity / 1024 / 1024);
/// fprintf(stderr, " - free:      %lu MByte\n", free / 1024 / 1024);
/// fprintf(stderr, " - available: %lu MByte\n", available / 1024 / 1024);
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL GetDiskSpace(const std::string& path,
                                        uint64_t& capacity,
                                        uint64_t& free,
                                        uint64_t& available)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->get_disk_space(
      CPrivateBase::m_interface->toKodi->kodiBase, path.c_str(), &capacity, &free, &available);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Return the file name from given complete path string.
///
/// @param[in] path The complete path include file and directory
/// @return Filename from path
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// std::string fileName = kodi::vfs::GetFileName("special://temp/kodi.log");
/// fprintf(stderr, "File name is '%s'\n", fileName.c_str());
/// ~~~~~~~~~~~~~
///
inline std::string ATTR_DLL_LOCAL GetFileName(const std::string& path)
{
  /* find the last slash */
  const size_t slash = path.find_last_of("/\\");
  return path.substr(slash + 1);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Return the directory name from given complete path string.
///
/// @param[in] path The complete path include file and directory
/// @return Directory name from path
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// std::string dirName = kodi::vfs::GetDirectoryName("special://temp/kodi.log");
/// fprintf(stderr, "Directory name is '%s'\n", dirName.c_str());
/// ~~~~~~~~~~~~~
///
inline std::string ATTR_DLL_LOCAL GetDirectoryName(const std::string& path)
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
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Remove the slash on given path name.
///
/// @param[in,out] path The complete path
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// std::string dirName = "special://temp/";
/// kodi::vfs::RemoveSlashAtEnd(dirName);
/// fprintf(stderr, "Directory name is '%s'\n", dirName.c_str());
/// ~~~~~~~~~~~~~
///
inline void ATTR_DLL_LOCAL RemoveSlashAtEnd(std::string& path)
{
  if (!path.empty())
  {
    char last = path[path.size() - 1];
    if (last == '/' || last == '\\')
      path.erase(path.size() - 1);
  }
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Return a size aligned to the chunk size at least as large as the
/// chunk size.
///
/// @param[in] chunk The chunk size
/// @param[in] minimum The minimum size (or maybe the minimum number of chunks?)
/// @return The aligned size
///
inline unsigned int ATTR_DLL_LOCAL GetChunkSize(unsigned int chunk, unsigned int minimum)
{
  if (chunk)
    return chunk * ((minimum + chunk - 1) / chunk);
  else
    return minimum;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Checks the given path contains a known internet protocol.
///
/// About following protocols are the path checked:
/// | Protocol | Return true condition | Protocol | Return true condition
/// |----------|-----------------------|----------|-----------------------
/// | **dav**  | strictCheck = true    | **rtmps**    | always
/// | **davs** | strictCheck = true    | **rtmpt**    | always
/// | **ftp**  | strictCheck = true    | **rtmpte**   | always
/// | **ftps** | strictCheck = true    | **rtp**      | always
/// | **http** | always                | **rtsp**     | always
/// | **https**| always                | **sdp**      | always
/// | **mms**  | always                | **sftp**     | strictCheck = true
/// | **mmsh** | always                | **stack**    | always
/// | **mmst** | always                | **tcp**      | always
/// | **rtmp** | always                | **udp**      | always
/// | **rtmpe**| always                |              | |
///
/// @param[in] path To checked path/URL
/// @param[in] strictCheck [opt] If True the set of protocols used will be
///                        extended to include ftp, ftps, dav, davs and sftp.
/// @return True if path is to a internet stream, false otherwise
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// // Check should return false
/// fprintf(stderr, "File name 1 is internet stream '%s' (should no)\n",
///                     kodi::vfs::IsInternetStream("D:/my-file.mkv") ? "yes" : "no");
///
/// // Check should return true
/// fprintf(stderr, "File name 2 is internet stream '%s' (should yes)\n",
///                     kodi::vfs::IsInternetStream("http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4") ? "yes" : "no");
///
/// // Check should return false
/// fprintf(stderr, "File name 1 is internet stream '%s' (should no)\n",
///                     kodi::vfs::IsInternetStream("ftp://do-somewhere.com/the-file.mkv") ? "yes" : "no", false);
///
/// // Check should return true
/// fprintf(stderr, "File name 1 is internet stream '%s' (should yes)\n",
///                     kodi::vfs::IsInternetStream("ftp://do-somewhere.com/the-file.mkv") ? "yes" : "no", true);
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL IsInternetStream(const std::string& path, bool strictCheck = false)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->is_internet_stream(
      CPrivateBase::m_interface->toKodi->kodiBase, path.c_str(), strictCheck);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Checks whether the specified path refers to a local network.
///
/// In difference to @ref IsHostOnLAN() include this more deeper checks where
/// also handle Kodi's special protocol and stacks.
///
/// @param[in] path To checked path
/// @return True if path is on LAN, false otherwise
///
/// @note Check includes @ref IsHostOnLAN() too.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// // Check should return true
/// bool lan = kodi::vfs::IsOnLAN("smb://path/to/file");
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL IsOnLAN(const std::string& path)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->is_on_lan(
      CPrivateBase::m_interface->toKodi->kodiBase, path.c_str());
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Checks specified path for external network.
///
/// @param[in] path To checked path
/// @return True if path is remote, false otherwise
///
/// @note This does not apply to the local network.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// // Check should return true
/// bool remote = kodi::vfs::IsRemote("http://path/to/file");
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL IsRemote(const std::string& path)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->is_remote(
      CPrivateBase::m_interface->toKodi->kodiBase, path.c_str());
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Checks whether the given path refers to the own system.
///
/// @param[in] path To checked path
/// @return True if path is local, false otherwise
///
inline bool ATTR_DLL_LOCAL IsLocal(const std::string& path)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->is_local(
      CPrivateBase::m_interface->toKodi->kodiBase, path.c_str());
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Checks specified path is a regular URL, e.g. "someprotocol://path/to/file"
///
/// @return True if file item is URL, false otherwise
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
///
/// bool isURL;
/// // Check should return true
/// isURL = kodi::vfs::IsURL("someprotocol://path/to/file");
///
/// // Check should return false
/// isURL = kodi::vfs::IsURL("/path/to/file");
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL IsURL(const std::string& path)
{
  using namespace kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->is_url(
      CPrivateBase::m_interface->toKodi->kodiBase, path.c_str());
}
//--------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief To get HTTP header information.
///
/// @param[in] url URL source of the data
/// @param[out] header The @ref cpp_kodi_vfs_Defs_HttpHeader
/// @return true if successfully done, otherwise false
///
///
/// ------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_vfs_Defs_HttpHeader_Help
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// kodi::vfs::HttpHeader header;
/// bool ret = kodi::vfs::GetHttpHeader(url, header);
/// ...
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL GetHttpHeader(const std::string& url, HttpHeader& header)
{
  using namespace ::kodi::addon;

  return CPrivateBase::m_interface->toKodi->kodi_filesystem->get_http_header(
      CPrivateBase::m_interface->toKodi->kodiBase, url.c_str(), &header.m_handle);
}
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Get file mime type.
///
/// @param[in] url URL source of the data
/// @param[out] mimeType the mime type of the URL
/// @param[in] useragent to be used when retrieving the MimeType [opt]
/// @return true if successfully done, otherwise false
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// std::string mimeType;.
/// if (kodi::vfs::GetMimeType(url, mimeType))
///   fprintf(stderr, "The mime type is '%s'\n", mimeType.c_str());
/// ...
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL GetMimeType(const std::string& url,
                                       std::string& mimeType,
                                       const std::string& useragent = "")
{
  using namespace ::kodi::addon;

  char* cMimeType = nullptr;
  bool ret = CPrivateBase::m_interface->toKodi->kodi_filesystem->get_mime_type(
      CPrivateBase::m_interface->toKodi->kodiBase, url.c_str(), &cMimeType, useragent.c_str());
  if (cMimeType != nullptr)
  {
    mimeType = cMimeType;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   cMimeType);
  }
  return ret;
}
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Get file content-type.
///
/// @param[in] url URL source of the data
/// @param[out] content The returned type
/// @param[in] useragent to be used when retrieving the MimeType [opt]
/// @return true if successfully done, otherwise false
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// std::string content;.
/// if (kodi::vfs::GetContentType(url, content))
///   fprintf(stderr, "The content type is '%s'\n", content.c_str());
/// ...
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL GetContentType(const std::string& url,
                                          std::string& content,
                                          const std::string& useragent = "")
{
  using namespace ::kodi::addon;

  char* cContent = nullptr;
  bool ret = CPrivateBase::m_interface->toKodi->kodi_filesystem->get_content_type(
      CPrivateBase::m_interface->toKodi->kodiBase, url.c_str(), &cContent, useragent.c_str());
  if (cContent != nullptr)
  {
    content = cContent;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   cContent);
  }
  return ret;
}
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_vfs_General
/// @brief Get cookies stored by CURL in RFC 2109 format.
///
/// @param[in] url URL source of the data
/// @param[out] cookies The text list of available cookies
/// @return true if successfully done, otherwise false
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
/// ...
/// std::string url = "";
/// std::string cookies;
/// bool ret = kodi::vfs::GetCookies(url, cookies);
/// fprintf(stderr, "Cookies from URL '%s' are '%s' (return was %s)\n",
///         url.c_str(), cookies.c_str(), ret ? "true" : "false");
/// ...
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL GetCookies(const std::string& url, std::string& cookies)
{
  using namespace ::kodi::addon;

  char* cCookies = nullptr;
  bool ret = CPrivateBase::m_interface->toKodi->kodi_filesystem->get_cookies(
      CPrivateBase::m_interface->toKodi->kodiBase, url.c_str(), &cCookies);
  if (cCookies != nullptr)
  {
    cookies = cCookies;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   cCookies);
  }
  return ret;
}
//----------------------------------------------------------------------------

//}}}

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" CFile class
//{{{

//==============================================================================
/// @defgroup cpp_kodi_vfs_CFile 4. class CFile
/// @ingroup cpp_kodi_vfs
///
/// @brief **Creatable class for virtual file server control**\n
/// To perform file read/write with Kodi's filesystem parts.
///
/// CFile is the class used for handling Files in Kodi. This class can be used
/// for creating, reading, writing and modifying files. It directly provides unbuffered, binary disk input/output services
///
/// It has the header @ref Filesystem.h "#include <kodi/Filesystem.h>" be included
/// to enjoy it.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/Filesystem.h>
///
/// ...
///
/// /* Create the needed file handle class */
/// kodi::vfs::CFile myFile();
///
/// /* In this example we use the user path for the add-on */
/// std::string file = kodi::GetUserPath() + "/myFile.txt";
///
/// /* Now create and open the file or overwrite if present */
/// myFile.OpenFileForWrite(file, true);
///
/// const char* str = "I love Kodi!";
///
/// /* write string */
/// myFile.Write(str, sizeof(str));
///
/// /* On this way the Close() is not needed to call, becomes done from destructor */
///
/// ~~~~~~~~~~~~~
///
///@{
class ATTR_DLL_LOCAL CFile
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Construct a new, unopened file.
  ///
  CFile() = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief <b>`Close()`</b> is called from the destructor, so explicitly
  /// closing the file isn't required.
  ///
  virtual ~CFile() { Close(); }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Open the file with filename via Kodi's @ref cpp_kodi_vfs_CFile
  /// "CFile". Needs to be closed by calling Close() when done.
  ///
  /// @param[in] filename The filename to open.
  /// @param[in] flags [opt] The flags to pass, see @ref OpenFileFlags
  /// @return True on success or false on failure
  ///
  bool OpenFile(const std::string& filename, unsigned int flags = 0)
  {
    using namespace kodi::addon;

    Close();
    m_file = CPrivateBase::m_interface->toKodi->kodi_filesystem->open_file(
        CPrivateBase::m_interface->toKodi->kodiBase, filename.c_str(), flags);
    return m_file != nullptr;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Open the file with filename via Kodi's @ref cpp_kodi_vfs_CFile
  /// "CFile" in write mode. Needs to be closed by calling Close() when
  /// done.
  ///
  /// @note Related folders becomes created if not present.
  ///
  /// @param[in] filename The filename to open.
  /// @param[in] overwrite True to overwrite, false otherwise.
  /// @return True on success or false on failure
  ///
  bool OpenFileForWrite(const std::string& filename, bool overwrite = false)
  {
    using namespace kodi::addon;

    Close();

    // Try to open the file. If it fails, check if we need to create the directory first
    // This way we avoid checking if the directory exists every time
    m_file = CPrivateBase::m_interface->toKodi->kodi_filesystem->open_file_for_write(
        CPrivateBase::m_interface->toKodi->kodiBase, filename.c_str(), overwrite);
    if (!m_file)
    {
      std::string cacheDirectory = kodi::vfs::GetDirectoryName(filename);
      if (CPrivateBase::m_interface->toKodi->kodi_filesystem->directory_exists(
              CPrivateBase::m_interface->toKodi->kodiBase, cacheDirectory.c_str()) ||
          CPrivateBase::m_interface->toKodi->kodi_filesystem->create_directory(
              CPrivateBase::m_interface->toKodi->kodiBase, cacheDirectory.c_str()))
        m_file = CPrivateBase::m_interface->toKodi->kodi_filesystem->open_file_for_write(
            CPrivateBase::m_interface->toKodi->kodiBase, filename.c_str(), overwrite);
    }
    return m_file != nullptr;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Check file is opened.
  ///
  /// @return True on open or false on closed or failure
  ///
  bool IsOpen() const { return m_file != nullptr; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Close an open file.
  ///
  void Close()
  {
    using namespace kodi::addon;

    if (!m_file)
      return;
    CPrivateBase::m_interface->toKodi->kodi_filesystem->close_file(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file);
    m_file = nullptr;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Create a Curl representation
  ///
  /// @param[in] url The URL of the Type.
  /// @return True on success or false on failure
  ///
  bool CURLCreate(const std::string& url)
  {
    using namespace kodi::addon;

    m_file = CPrivateBase::m_interface->toKodi->kodi_filesystem->curl_create(
        CPrivateBase::m_interface->toKodi->kodiBase, url.c_str());
    return m_file != nullptr;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Add options to the curl file created with CURLCreate.
  ///
  /// @param[in] type Option type to set, see @ref CURLOptiontype
  /// @param[in] name Name of the option
  /// @param[in] value Value of the option
  /// @return True on success or false on failure
  ///
  bool CURLAddOption(CURLOptiontype type, const std::string& name, const std::string& value)
  {
    using namespace kodi::addon;

    if (!m_file)
    {
      kodi::Log(ADDON_LOG_ERROR, "kodi::vfs::CURLCreate(...) needed to call before!");
      return false;
    }
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->curl_add_option(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file, type, name.c_str(), value.c_str());
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Open the curl file created with CURLCreate.
  ///
  /// @param[in] flags [opt] The flags to pass, see @ref OpenFileFlags
  /// @return True on success or false on failure
  ///
  bool CURLOpen(unsigned int flags = 0)
  {
    using namespace kodi::addon;

    if (!m_file)
    {
      kodi::Log(ADDON_LOG_ERROR, "kodi::vfs::CURLCreate(...) needed to call before!");
      return false;
    }
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->curl_open(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file, flags);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Read from an open file.
  ///
  /// @param[in] ptr The buffer to store the data in.
  /// @param[in] size The size of the buffer.
  /// @return number of successfully read bytes if any bytes were read and
  ///         stored in buffer, zero if no bytes are available to read (end of
  ///         file was reached) or undetectable error occur, -1 in case of any
  ///         explicit error
  ///
  ssize_t Read(void* ptr, size_t size)
  {
    using namespace kodi::addon;

    if (!m_file)
      return -1;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->read_file(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file, ptr, size);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Read a string from an open file.
  ///
  /// @param[out] line The buffer to store the data in.
  /// @return True when a line was read, false otherwise.
  ///
  /// ------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/Filesystem.h>
  ///
  /// ...
  ///
  /// /* Create the needed file handle class */
  /// kodi::vfs::CFile myFile;
  ///
  /// /* Open the wanted file */
  /// if (myFile.OpenFile(kodi::addon::GetUserPath("/myFile.txt")))
  /// {
  ///   /* Read all lines inside file */
  ///   while (1)
  ///   {
  ///     std::string line;
  ///     if (!myFile.ReadLine(line))
  ///       break;
  ///     fprintf(stderr, "%s\n", line.c_str());
  ///   }
  /// }
  ///
  /// ~~~~~~~~~~~~~
  ///
  bool ReadLine(std::string& line)
  {
    using namespace kodi::addon;

    line.clear();
    if (!m_file)
      return false;
    // Read 1024 chars into buffer. If file position advanced that many
    // chars, we didn't hit a newline. Otherwise, we read a newline
    // or we reached the end of the file.
    //
    // The strncpy idiom is used here (C++ allows a simpler implementation):
    //
    // char buffer[BUFFER_SIZE];
    // strncpy(buffer, sourceString, BUFFER_SIZE - 1);
    // buffer[BUFFER_SIZE - 1] = '\0';
    //
    char buffer[1025]{};
    if (CPrivateBase::m_interface->toKodi->kodi_filesystem->read_file_string(
            CPrivateBase::m_interface->toKodi->kodiBase, m_file, buffer, sizeof(buffer) - 1))
    {
      line = buffer;
      return !line.empty();
    }
    return false;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Write to a file opened in write mode.
  ///
  /// @param[in] ptr Pointer to the data to write, converted to a <b>`const void*`</b>.
  /// @param[in] size Size of the data to write.
  /// @return number of successfully written bytes if any bytes were written,
  ///         zero if no bytes were written and no detectable error occur,-1
  ///         in case of any explicit error
  ///
  ssize_t Write(const void* ptr, size_t size)
  {
    using namespace kodi::addon;

    if (!m_file)
      return -1;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->write_file(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file, ptr, size);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Flush buffered data.
  ///
  /// If the given stream was open for writing (or if it was open for updating
  /// and the last i/o operation was an output operation) any unwritten data
  /// in its output buffer is written to the file.
  ///
  /// The stream remains open after this call.
  ///
  /// When a file is closed, either because of a call to close or because the
  /// class is destructed, all the buffers associated with it are
  /// automatically flushed.
  ///
  void Flush()
  {
    using namespace kodi::addon;

    if (!m_file)
      return;
    CPrivateBase::m_interface->toKodi->kodi_filesystem->flush_file(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Set the file's current position.
  ///
  /// The whence argument is optional and defaults to SEEK_SET (0)
  ///
  /// @param[in] position the position that you want to seek to
  /// @param[in] whence [optional] offset relative to You can set the value of
  ///                              whence to one of three things:
  /// |   Value  | int | Description                                         |
  /// |:--------:|:---:|:----------------------------------------------------|
  /// | SEEK_SET |  0  | position is relative to the beginning of the file. This is probably what you had in mind anyway, and is the most commonly used value for whence.
  /// | SEEK_CUR |  1  | position is relative to the current file pointer position. So, in effect, you can say, "Move to my current position plus 30 bytes," or, "move to my current position minus 20 bytes."
  /// | SEEK_END |  2  | position is relative to the end of the file. Just like SEEK_SET except from the other end of the file. Be sure to use negative values for offset if you want to back up from the end of the file, instead of going past the end into oblivion.
  ///
  /// @return Returns the resulting offset location as measured in bytes from
  ///         the beginning of the file. On error, the value -1 is returned.
  ///
  int64_t Seek(int64_t position, int whence = SEEK_SET)
  {
    using namespace kodi::addon;

    if (!m_file)
      return -1;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->seek_file(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file, position, whence);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Truncate a file to the requested size.
  ///
  /// @param[in] size The new max size.
  /// @return New size? On error, the value -1 is returned.
  ///
  int Truncate(int64_t size)
  {
    using namespace kodi::addon;

    if (!m_file)
      return -1;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->truncate_file(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file, size);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief The current offset in an open file.
  ///
  /// @return The requested offset. On error, the value -1 is returned.
  ///
  int64_t GetPosition() const
  {
    using namespace kodi::addon;

    if (!m_file)
      return -1;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->get_file_position(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Get the file size of an open file.
  ///
  /// @return The requested size. On error, the value -1 is returned.
  ///
  int64_t GetLength() const
  {
    using namespace kodi::addon;

    if (!m_file)
      return -1;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->get_file_length(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Checks the file access is on end position.
  ///
  /// @return If you've reached the end of the file, AtEnd() returns true.
  ///
  bool AtEnd() const
  {
    using namespace kodi::addon;

    if (!m_file)
      return true;
    int64_t length = CPrivateBase::m_interface->toKodi->kodi_filesystem->get_file_length(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file);
    int64_t position = CPrivateBase::m_interface->toKodi->kodi_filesystem->get_file_position(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file);
    return position >= length;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Get the chunk size for an open file.
  ///
  /// @return The requested size. On error, the value -1 is returned.
  ///
  int GetChunkSize() const
  {
    using namespace kodi::addon;

    if (!m_file)
      return -1;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->get_file_chunk_size(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief To check seek possible on current stream by file.
  ///
  /// @return true if seek possible, false if not
  ///
  bool IoControlGetSeekPossible() const
  {
    using namespace kodi::addon;

    if (!m_file)
      return false;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->io_control_get_seek_possible(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief To check a running stream on file for state of his cache.
  ///
  /// @param[in] status Information about current cache status
  /// @return true if successfully done, false otherwise
  ///
  ///
  /// @copydetails cpp_kodi_vfs_Defs_CacheStatus_Help
  ///
  bool IoControlGetCacheStatus(CacheStatus& status) const
  {
    using namespace kodi::addon;

    if (!m_file)
      return false;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->io_control_get_cache_status(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file, status);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Unsigned int with speed limit for caching in bytes per second.
  ///
  /// @param[in] rate Cache rate size to use
  /// @return true if successfully done, false otherwise
  ///
  bool IoControlSetCacheRate(uint32_t rate)
  {
    using namespace kodi::addon;

    if (!m_file)
      return false;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->io_control_set_cache_rate(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file, rate);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Enable/disable retry within the protocol handler (if supported).
  ///
  /// @param[in] retry To set the retry, true for use, false for not
  /// @return true if successfully done, false otherwise
  ///
  bool IoControlSetRetry(bool retry)
  {
    using namespace kodi::addon;

    if (!m_file)
      return false;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->io_control_set_retry(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file, retry);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Retrieve a file property.
  ///
  /// @param[in] type The type of the file property to retrieve the value for
  /// @param[in] name The name of a named property value (e.g. Header)
  /// @return value of requested property, empty on failure / non-existance
  ///
  const std::string GetPropertyValue(FilePropertyTypes type, const std::string& name) const
  {
    using namespace kodi::addon;

    if (!m_file)
    {
      kodi::Log(ADDON_LOG_ERROR,
                "kodi::vfs::CURLCreate(...) needed to call before GetPropertyValue!");
      return "";
    }
    std::vector<std::string> values = GetPropertyValues(type, name);
    if (values.empty())
    {
      return "";
    }
    return values[0];
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Retrieve file property values.
  ///
  /// @param[in] type The type of the file property values to retrieve the value for
  /// @param[in] name The name of the named property (e.g. Header)
  /// @return values of requested property, empty vector on failure / non-existance
  ///
  const std::vector<std::string> GetPropertyValues(FilePropertyTypes type,
                                                   const std::string& name) const
  {
    using namespace kodi::addon;

    if (!m_file)
    {
      kodi::Log(ADDON_LOG_ERROR,
                "kodi::vfs::CURLCreate(...) needed to call before GetPropertyValues!");
      return std::vector<std::string>();
    }
    int numValues = 0;
    char** res(CPrivateBase::m_interface->toKodi->kodi_filesystem->get_property_values(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file, type, name.c_str(), &numValues));
    if (res)
    {
      std::vector<std::string> vecReturn;
      vecReturn.reserve(numValues);
      for (int i = 0; i < numValues; ++i)
      {
        vecReturn.emplace_back(res[i]);
      }
      CPrivateBase::m_interface->toKodi->free_string_array(
          CPrivateBase::m_interface->toKodi->kodiBase, res, numValues);
      return vecReturn;
    }
    return std::vector<std::string>();
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_vfs_CFile
  /// @brief Get the current download speed of file if loaded from web.
  ///
  /// @return The current download speed.
  ///
  double GetFileDownloadSpeed() const
  {
    using namespace kodi::addon;

    if (!m_file)
      return 0.0;
    return CPrivateBase::m_interface->toKodi->kodi_filesystem->get_file_download_speed(
        CPrivateBase::m_interface->toKodi->kodiBase, m_file);
  }
  //--------------------------------------------------------------------------

private:
  void* m_file = nullptr;
};
///@}
//------------------------------------------------------------------------------

//}}}

} /* namespace vfs */
} /* namespace kodi */

#endif /* __cplusplus */
