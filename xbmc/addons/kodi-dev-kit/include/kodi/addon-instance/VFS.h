/*
 *  Copyright (C) 2015-2018 Team Kodi
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../Filesystem.h"
#include "../c-api/addon-instance/vfs.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class CInstanceVFS;

//==============================================================================
/// @ingroup cpp_kodi_addon_vfs_Defs
/// @brief **VFS add-on file handle**\n
/// This used to handle opened files of addon with related memory pointer about
/// class or structure and to have on further file control functions available.
///
/// See @ref cpp_kodi_addon_vfs_filecontrol "file editing functions" for used
/// places.
///
///@{
using VFSFileHandle = VFS_FILE_HANDLE;
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_vfs_Defs_VFSUrl class VFSUrl
/// @ingroup cpp_kodi_addon_vfs_Defs
/// @brief **VFS add-on URL data**\n
/// This class is used to inform the addon of the desired wanted connection.
///
/// Used on mostly all addon functions to identify related target.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_vfs_Defs_VFSUrl_Help
///
///@{
class ATTRIBUTE_HIDDEN VFSUrl : public CStructHdl<VFSUrl, VFSURL>
{
  /*! \cond PRIVATE */
  friend class CInstanceVFS;
  /*! \endcond */

public:
  /// @defgroup cpp_kodi_addon_vfs_Defs_VFSUrl_Help Value Help
  /// @ingroup cpp_kodi_addon_vfs_Defs_VFSUrl
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_vfs_Defs_VFSUrl :</b>
  /// | Name | Type | Get call
  /// |------|------|----------
  /// | **URL** | `std::string` | @ref VFSUrl::GetURL "GetURL"
  /// | **Domain name** | `std::string` | @ref VFSUrl::GetDomain "GetDomain"
  /// | **Hostname** | `std::string` | @ref VFSUrl::GetHostname "GetHostname"
  /// | **Filename** | `std::string` | @ref VFSUrl::GetFilename "GetFilename"
  /// | **Network port** | `unsigned int` | @ref VFSUrl::GetPort "GetPort"
  /// | **Special options** | `std::string` | @ref VFSUrl::GetOptions "GetOptions"
  /// | **Username** | `std::string` | @ref VFSUrl::GetUsername "GetUsername"
  /// | **Password** | `std::string` | @ref VFSUrl::GetPassword "GetPassword"
  /// | **Get URL with user and password hidden** | `std::string` | @ref VFSUrl::GetRedacted "GetRedacted"
  /// | **Sharename** | `std::string` | @ref VFSUrl::GetSharename "GetSharename"
  /// | **Network protocol** | `std::string` | @ref VFSUrl::GetProtocol "GetProtocol"
  ///

  /// @addtogroup cpp_kodi_addon_vfs_Defs_VFSUrl
  ///@{

  /// @brief Desired URL of the file system to be edited
  ///
  /// This includes all available parts of the access and is structured as
  /// follows:
  /// -
  /// <b>`<PROTOCOL>`://`<USERNAME>`:`<PASSWORD>``@``<HOSTNAME>`:`<PORT>`/`<FILENAME>`?`<OPTIONS>`</b>
  std::string GetURL() const { return m_cStructure->url; }

  /// @brief The associated domain name, which is optional and not available
  /// in all cases.
  std::string GetDomain() const { return m_cStructure->domain; }

  /// @brief This includes the network address (e.g. `192.168.0.123`) or if
  /// the addon refers to file packages the path to it
  /// (e.g. `/home/by_me/MyPacket.rar`).
  std::string GetHostname() const { return m_cStructure->hostname; }

  /// @brief With this variable the desired path to a folder or file within
  /// the hostname is given (e.g. `storage/videos/00001.ts`).
  std::string GetFilename() const { return m_cStructure->filename; }

  /// @brief [Networking port](https://en.wikipedia.org/wiki/Port_(computer_networking))
  /// to use for protocol.
  unsigned int GetPort() const { return m_cStructure->port; }

  /// @brief Special options on opened URL, this can e.g. on RAR packages
  /// <b>`?flags=8&nextvalue=123`</b> to inform about to not cache a read.
  ///
  /// Available options from Kodi:
  /// | Value:    | Description:
  /// |-----------|-------------------
  /// | flags=8   | Used on RAR packages so that no data is cached from the requested source.
  /// | cache=no  | Used on ZIP packages so that no data from the requested source is stored in the cache. However, this is currently not available from addons!
  ///
  /// In addition, other addons can use the URLs given by them to give options
  /// that fit the respective VFS addon and allow special operations.
  ///
  /// @note This procedure is not yet standardized and is currently not
  /// exactly available which are handed over.
  std::string GetOptions() const { return m_cStructure->options; }

  /// @brief Desired username.
  std::string GetUsername() const { return m_cStructure->username; }

  /// @brief Desired password.
  std::string GetPassword() const { return m_cStructure->password; }

  /// @brief The complete URL is passed on here, but the user name and
  /// password are not shown and only appear to there as `USERNAME:PASSWORD`.
  ///
  /// As example <b>`sftp://USERNAME:PASSWORD@192.168.178.123/storage/videos/00001.ts`</b>.
  std::string GetRedacted() const { return m_cStructure->redacted; }

  /// @brief The name which is taken as the basis by source and would be first
  /// in folder view.
  ///
  /// As example on <b>`sftp://dudu:isprivate@192.168.178.123/storage/videos/00001.ts`</b>
  /// becomes then <b>`storage`</b> used here.
  std::string GetSharename() const { return m_cStructure->sharename; }

  /// @brief Protocol name used on this stream, e.g. <b>`sftp`</b>.
  std::string GetProtocol() const { return m_cStructure->protocol; }

  ///@}

private:
  VFSUrl() = delete;
  VFSUrl(const VFSUrl& channel) = delete;
  VFSUrl(const VFSURL* channel) : CStructHdl(channel) {}
  VFSUrl(VFSURL* channel) : CStructHdl(channel) {}
};
///@}
//------------------------------------------------------------------------------

//##############################################################################
/// @defgroup cpp_kodi_addon_vfs_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_addon_vfs
/// @brief **VFS add-on general variables**
///
/// Used to exchange the available options between Kodi and addon.
///
///

//==============================================================================
///
/// @addtogroup cpp_kodi_addon_vfs
/// @brief \cpp_class{ kodi::addon::CInstanceVFS }
/// **Virtual Filesystem (VFS) add-on instance**
///
/// This instance type is used to allow Kodi various additional file system
/// types. Be it a special file system, a compressed package or a system
/// available over the network, everything is possible with it.
///
/// This usage can be requested under various conditions, for example explicitly
/// by another addon, by a Mimetype protocol defined in <b>`addon.xml`</b> or supported
/// file extensions.
///
/// Include the header @ref VFS.h "#include <kodi/addon-instance/VFS.h>"
/// to use this class.
///
/// ----------------------------------------------------------------------------
///
/// Here is an example of what the <b>`addon.xml.in`</b> would look like for an VFS addon:
///
/// ~~~~~~~~~~~~~{.xml}
/// <?xml version="1.0" encoding="UTF-8"?>
/// <addon
///   id="vfs.myspecialnamefor"
///   version="1.0.0"
///   name="My VFS addon"
///   provider-name="Your Name">
///   <requires>@ADDON_DEPENDS@</requires>
///   <extension
///     point="kodi.vfs"
///     protocols="myprot"
///     extensions=".abc|.def"
///     files="true"
///     filedirectories="true"
///     directories="true"
///     encodedhostname="true"
///     supportDialog="true"
///     supportPath="true"
///     supportUsername="true"
///     supportPassword="true"
///     supportPort="true"
///     supportBrowsing="true"
///     supportWrite="true"
///     defaultPort="1234"
///     label="30000"
///     zeroconf="your_special_zeroconf_allowed_identifier"
///     library_@PLATFORM@="@LIBRARY_FILENAME@"/>
///   <extension point="xbmc.addon.metadata">
///     <summary lang="en_GB">My VFS addon summary</summary>
///     <description lang="en_GB">My VFS description</description>
///     <platform>@PLATFORM@</platform>
///   </extension>
/// </addon>
/// ~~~~~~~~~~~~~
///
/// @note Regarding boolean values with "false", these can also be omitted,
/// since this would be the default.
///
///
/// ### Standard values that can be declared for processing in `addon.xml`.
///
/// These values are used by Kodi to identify associated streams and file
/// extensions and then to select the associated addon.
///
/// \table_start
///   \table_h3{ Labels, Type,   Description }
///   \table_row3{   <b>`point`</b>,
///                  \anchor cpp_kodi_addon_vfs_point
///                  string,
///     The identification of the addon instance to VFS is mandatory <b>`kodi.vfs`</b>.
///     In addition\, the instance declared in the first <b>`<extension ... />`</b> is also the main type of addon.
///   }
///   \table_row3{   <b>`defaultPort`</b>,
///                  \anchor cpp_kodi_addon_vfs_defaultPort
///                  integer,
///     Default [networking port](https://en.wikipedia.org/wiki/Port_(computer_networking))
///     to use for protocol.
///   }
///   \table_row3{   <b>`directories`</b>,
///                  \anchor cpp_kodi_addon_vfs_directories
///                  boolean,
///     VFS entry can list directories.
///   }
///   \table_row3{   <b>`extensions`</b>,
///                  \anchor cpp_kodi_addon_vfs_extensions
///                  string,
///     Extensions for VFS entry.\n
///     It is possible to declare several using <b>`|`</b>\, e.g. <b>`.abc|.def|.ghi`</b>.
///   }
///   \table_row3{   <b>`encodedhostname`</b>,
///                  \anchor cpp_kodi_addon_vfs_encodedhostname
///                  boolean,
///     URL protocol from add-ons use encoded hostnames.
///   }
///   \table_row3{   <b>`filedirectories`</b>,
///                  \anchor cpp_kodi_addon_vfs_filedirectories
///                  boolean,
///     VFS entry contains file directories.
///   }
///   \table_row3{   <b>`files`</b>,
///                  \anchor cpp_kodi_addon_vfs_directories
///                  boolean,
///     Set to declare that VFS provides files.
///   }
///   \table_row3{   <b>`protocols`</b>,
///                  \anchor cpp_kodi_addon_vfs_protocols
///                  boolean,
///     Protocols for VFS entry.\n
///     It is possible to declare several using <b>`|`</b>\, e.g. <b>`myprot1|myprot2`</b>.\n
///     @note This field also used to show on GUI\, see <b>`supportBrowsing`</b> below about <b>*2:</b>.
///     When used there\, however\, only a **single** protocol is possible!
///   }
///   \table_row3{   <b>`supportWrite`</b>,
///                  \anchor cpp_kodi_addon_vfs_supportWrite
///                  boolean,
///     Protocol supports write operations.
///   }
///   \table_row3{   <b>`zeroconf`</b>,
///                  \anchor cpp_kodi_addon_vfs_zeroconf
///                  string,
///     [Zero conf](https://en.wikipedia.org/wiki/Zero-configuration_networking) announce string for VFS protocol.
///   }
///   \table_row3{   <b>`library_@PLATFORM@`</b>,
///                  \anchor cpp_kodi_addon_vfs_library
///                  string,
///     The runtime library used for the addon. This is usually declared by `cmake` and correctly displayed in the translated <b>`addon.xml`</b>.
///   }
/// \table_end
///
///
/// ### User selectable parts of the addon.
///
/// The following table describes the values that can be defined by <b>`addon.xml`</b>
/// and which part they relate to for user input.
///
/// \table_start
///   \table_h3{ Labels, Type, Description }
///   \table_row3{   <b>`supportBrowsing`</b>,
///                  \anchor cpp_kodi_addon_vfs_protocol_supportBrowsing
///                  boolean,
///     Protocol supports server browsing. Used to open related sources by users in the window.\n\n
///     | Associated places in Kodi: |
///     | :---- |
///     | \image html cpp_kodi_addon_vfs_protocol_1.png |
///     <br>
///     <b>*1:</b> The entry in the menu represented by this option corresponds to the text given with <b>`label`</b>.
///     When the button is pressed\, @ref CInstanceVFS::GetDirectory is called on the add-on to get its content.\n
///     <b>*2:</b> Protocol name of the stream defined with <b>`protocols`</b> in xml.\n
///     @remark See also <b>`supportDialog`</b> about <b>*3:</b>.
///   }
///   \table_row3{   <b>`supportDialog`</b>,
///                  \anchor cpp_kodi_addon_vfs_protocol_supportDialog
///                  boolean,
///     To point out that Kodi assigns a dialog to this VFS in order to compare it with other values e.g. query supportPassword in it.\n
///     This will be available when adding sources in Kodi under <b>"Add network location..."</b>.\n\n
///     | Associated places in Kodi: |
///     | :---- |
///     | \image html cpp_kodi_addon_vfs_protocol_2.png |
///     <br>
///     <b>*1:</b> Field for selecting the VFS handler\, the addon will be available if <b>`supportDialog`</b> is set to <b>`true`</b>.\n
///     <b>*2:</b> To set the associated server address. **Note:** *This field is always activated and cannot be changed by the addon.*\n
///     <b>*3:</b> If <b>`supportBrowsing`</b> is set to <b>`true`</b>\, the button for opening a file selection dialog is given here too\, as in the file window.\n
///     <b>*4:</b> This field is available if <b>`supportPath`</b> is set to <b>`true`</b>.\n
///     <b>*5:</b> To edit the connection port. This field is available if <b>`supportPort`</b> is set to <b>`true`</b>.\n
///     <b>*6:</b> This sets the required username and is available when <b>`supportUsername`</b> is set to <b>`true`</b>.\n
///     <b>*7:</b> This sets the required password and is available when <b>`supportPassword`</b> is set to <b>`true`</b>.
///   }
///   \table_row3{   <b>`supportPath`</b>,
///                  \anchor cpp_kodi_addon_vfs_protocol_supportPath
///                  boolean,
///     Protocol has path in addition to server name (see <b>`supportDialog`</b> about <b>*4:</b>).
///   }
///   \table_row3{   <b>`supportPort`</b>,
///                  \anchor cpp_kodi_addon_vfs_protocol_supportPort
///                  boolean,
///     Protocol supports port customization (see <b>`supportDialog`</b> about <b>*5:</b>).
///   }
///   \table_row3{   <b>`supportUsername`</b>,
///                  \anchor cpp_kodi_addon_vfs_protocol_supportUsername
///                  boolean,
///     Protocol uses logins (see <b>`supportDialog`</b> about <b>*6:</b>).
///   }
///   \table_row3{   <b>`supportPassword`</b>,
///                  \anchor cpp_kodi_addon_vfs_protocol_supportPassword
///                  boolean,
///     Protocol supports passwords (see <b>`supportDialog`</b> about <b>*7:</b>).
///   }
///   \table_row3{   <b>`protocols`</b>,
///                  \anchor cpp_kodi_addon_vfs_protocol_protocols
///                  string,
///     Protocols for VFS entry.
///     @note This field is not editable and only used on GUI to show his name\, see <b>`supportBrowsing`</b> about <b>*2:</b>
///   }
///   \table_row3{   <b>`label`</b>,
///                  \anchor cpp_kodi_addon_vfs_protocol_label
///                  integer,
///     The text identification number used in Kodi for display in the menu at <b>`supportDialog`</b>
///     as a selection option and at <b>`supportBrowsing`</b> (see his image reference <b>*1</b>) as a menu entry.\n
///     This can be a text identifier in Kodi or from addon.\n
///     @remark For addon within <b>30000</b>-<b>30999</b> or <b>32000</b>-<b>32999</b>.
///   }
/// \table_end
///
/// @remark For more detailed description of the <b>`addon.xml`</b>, see also https://kodi.wiki/view/Addon.xml.
///
///
/// --------------------------------------------------------------------------
///
///
/// **Example:**
///
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/VFS.h>
///
/// class CMyVFS : public kodi::addon::CInstanceVFS
/// {
/// public:
///   CMyVFS(KODI_HANDLE instance, const std::string& kodiVersion);
///
///   // Add all your required functions, the most CInstanceVFS functions of
///   // must be included to have addon working correctly.
///   ...
/// };
///
/// CMyVFS::CMyVFS(KODI_HANDLE instance, const std::string& kodiVersion)
///   : kodi::addon::CInstanceVFS(instance, kodiVersion)
/// {
///   ...
/// }
///
/// ...
///
/// //----------------------------------------------------------------------
///
/// class CMyAddon : public kodi::addon::CAddonBase
/// {
/// public:
///   CMyAddon() { }
///   ADDON_STATUS CreateInstance(int instanceType,
///                               const std::string& instanceID,
///                               KODI_HANDLE instance,
///                               const std::string& version,
///                               KODI_HANDLE& addonInstance) override;
/// };
///
/// // If you use only one instance in your add-on, can be instanceType and
/// // instanceID ignored
/// ADDON_STATUS CMyAddon::CreateInstance(int instanceType,
///                                       const std::string& instanceID,
///                                       KODI_HANDLE instance,
///                                       const std::string& version,
///                                       KODI_HANDLE& addonInstance)
/// {
///   if (instanceType == ADDON_INSTANCE_VFS)
///   {
///     kodi::Log(ADDON_LOG_INFO, "Creating my VFS instance");
///     addonInstance = new CMyVFS(instance, version);
///     return ADDON_STATUS_OK;
///   }
///   else if (...)
///   {
///     ...
///   }
///   return ADDON_STATUS_UNKNOWN;
/// }
///
/// ADDONCREATOR(CMyAddon)
/// ~~~~~~~~~~~~~
///
/// The destruction of the example class `CMyVFS` is called from
/// Kodi's header. Manually deleting the add-on instance is not required.
///
//------------------------------------------------------------------------------
class ATTRIBUTE_HIDDEN CInstanceVFS : public IAddonInstance
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs
  /// @brief VFS class constructor used to support multiple instance
  /// types
  ///
  /// @param[in] instance               The instance value given to
  ///                                   <b>`kodi::addon::CAddonBase::CreateInstance(...)`</b>.
  /// @param[in] kodiVersion            [opt] given from Kodi by @ref CAddonBase::CreateInstance
  ///                                   to identify his instance API version
  ///
  /// @note Instance path as a single is not supported by this type. It must
  /// ensure that it can be called up several times.
  ///
  /// @warning Only use `instance` from the @ref CAddonBase::CreateInstance or
  /// @ref CAddonBase::CreateInstance call.
  ///
  explicit CInstanceVFS(KODI_HANDLE instance, const std::string& kodiVersion = "")
    : IAddonInstance(ADDON_INSTANCE_VFS,
                     !kodiVersion.empty() ? kodiVersion : GetKodiTypeVersion(ADDON_INSTANCE_VFS))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceVFS: Creation of multiple together with single "
                             "instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs
  /// @brief Destructor
  ///
  ~CInstanceVFS() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_vfs_general 1. General access functions
  /// @ingroup cpp_kodi_addon_vfs
  /// @brief **General access functions**
  ///
  /// This functions which are intended for getting folders, editing storage
  /// locations and file system queries.
  ///

  //============================================================================
  /// @defgroup cpp_kodi_addon_vfs_filecontrol 2. File editing functions
  /// @ingroup cpp_kodi_addon_vfs
  /// @brief **File editing functions.**
  ///
  /// This value represents the addon-side handlers and to be able to identify
  /// his own parts in the event of further access.
  ///

  //@{
  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Open a file for input
  ///
  /// @param[in] url The URL of the file
  /// @return Context for the opened file
  ///
  ///
  /// ----------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_vfs_Defs_VFSUrl_Help
  ///
  virtual kodi::addon::VFSFileHandle Open(const kodi::addon::VFSUrl& url) { return nullptr; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Open a file for output
  ///
  /// @param[in] url The URL of the file
  /// @param[in] overWrite Whether or not to overwrite an existing file
  /// @return Context for the opened file
  ///
  virtual kodi::addon::VFSFileHandle OpenForWrite(const kodi::addon::VFSUrl& url, bool overWrite)
  {
    return nullptr;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Close a file
  ///
  /// @param[in] context The context of the file
  /// @return True on success, false on failure
  ///
  virtual bool Close(kodi::addon::VFSFileHandle context) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Read from a file
  ///
  /// @param[in] context The context of the file
  /// @param[out] buffer The buffer to read data into
  /// @param[in] uiBufSize Number of bytes to read
  /// @return Number of bytes read
  ///
  virtual ssize_t Read(kodi::addon::VFSFileHandle context, uint8_t* buffer, size_t uiBufSize)
  {
    return -1;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Write to a file
  ///
  /// @param[in] context The context of the file
  /// @param[in] buffer The buffer to read data from
  /// @param[in] uiBufSize Number of bytes to write
  /// @return Number of bytes written
  ///
  virtual ssize_t Write(kodi::addon::VFSFileHandle context, const uint8_t* buffer, size_t uiBufSize)
  {
    return -1;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Seek in a file
  ///
  /// @param[in] context The context of the file
  /// @param[in] position The position to seek to
  /// @param[in] whence Position in file 'position' is relative to (SEEK_CUR, SEEK_SET, SEEK_END):
  /// |   Value  | int | Description                                         |
  /// |:--------:|:---:|:----------------------------------------------------|
  /// | SEEK_SET |  0  | position is relative to the beginning of the file. This is probably what you had in mind anyway, and is the most commonly used value for whence.
  /// | SEEK_CUR |  1  | position is relative to the current file pointer position. So, in effect, you can say, "Move to my current position plus 30 bytes," or, "move to my current position minus 20 bytes."
  /// | SEEK_END |  2  | position is relative to the end of the file. Just like SEEK_SET except from the other end of the file. Be sure to use negative values for offset if you want to back up from the end of the file, instead of going past the end into oblivion.
  /// @return Offset in file after seek
  ///
  virtual int64_t Seek(kodi::addon::VFSFileHandle context, int64_t position, int whence)
  {
    return -1;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Truncate a file
  ///
  /// @param[in] context The context of the file
  /// @param[in] size The size to truncate the file to
  /// @return 0 on success, -1 on error
  ///
  virtual int Truncate(kodi::addon::VFSFileHandle context, int64_t size) { return -1; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Get total size of a file
  ///
  /// @param[in] context The context of the file
  /// @return Total file size
  ///
  virtual int64_t GetLength(kodi::addon::VFSFileHandle context) { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Get current position in a file
  ///
  /// @param[in] context The context of the file
  /// @return Current position
  ///
  virtual int64_t GetPosition(kodi::addon::VFSFileHandle context) { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Get chunk size of a file
  ///
  /// @param[in] context The context of the file
  /// @return Chunk size
  ///
  virtual int GetChunkSize(kodi::addon::VFSFileHandle context) { return 1; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief To check seek possible on current stream by file.
  ///
  /// @return true if seek possible, false if not
  ///
  virtual bool IoControlGetSeekPossible(kodi::addon::VFSFileHandle context) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief To check a running stream on file for state of his cache.
  ///
  /// @param[in] status Information about current cache status
  /// @return true if successfull done, false otherwise
  ///
  ///
  /// @copydetails cpp_kodi_vfs_Defs_CacheStatus_Help
  ///
  virtual bool IoControlGetCacheStatus(kodi::addon::VFSFileHandle context,
                                       kodi::vfs::CacheStatus& status)
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Unsigned int with speed limit for caching in bytes per second.
  ///
  /// @param[in] rate Cache rate size to use
  /// @return true if successfull done, false otherwise
  ///
  virtual bool IoControlSetCacheRate(kodi::addon::VFSFileHandle context, unsigned int rate)
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_filecontrol
  /// @brief Enable/disable retry within the protocol handler (if supported).
  ///
  /// @param[in] retry To set the retry, true for use, false for not
  /// @return true if successfull done, false otherwise
  ///
  virtual bool IoControlSetRetry(kodi::addon::VFSFileHandle context, bool retry) { return false; }
  //----------------------------------------------------------------------------
  //@}

  //@{
  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_general
  /// @brief Stat a file
  ///
  /// @param[in] url The URL of the file
  /// @param[in] buffer The buffer to store results in
  /// @return -1 on error, 0 otherwise
  ///
  ///
  /// ----------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_vfs_Defs_VFSUrl_Help
  ///
  virtual int Stat(const kodi::addon::VFSUrl& url, kodi::vfs::FileStatus& buffer) { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_general
  /// @brief Check for file existence
  ///
  /// @param[in] url The URL of the file
  /// @return True if file exists, false otherwise
  ///
  virtual bool Exists(const kodi::addon::VFSUrl& url) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_general
  /// @brief Clear out any idle connections
  ///
  virtual void ClearOutIdle() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_general
  /// @brief Disconnect all connections
  ///
  virtual void DisconnectAll() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_general
  /// @brief Delete a file
  ///
  /// @param[in] url The URL of the file
  /// @return True if deletion was successful, false otherwise
  ///
  virtual bool Delete(const kodi::addon::VFSUrl& url) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_general
  /// @brief Rename a file
  ///
  /// @param[in] url The URL of the source file
  /// @param[in] url2 The URL of the destination file
  /// @return True if deletion was successful, false otherwise
  ///
  virtual bool Rename(const kodi::addon::VFSUrl& url, const kodi::addon::VFSUrl& url2)
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_general
  /// @brief Check for directory existence
  ///
  /// @param[in] url The URL of the file
  /// @return True if directory exists, false otherwise
  ///
  virtual bool DirectoryExists(const kodi::addon::VFSUrl& url) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_general
  /// @brief Remove a directory
  ///
  /// @param[in] url The URL of the directory
  /// @return True if removal was successful, false otherwise
  ///
  virtual bool RemoveDirectory(const kodi::addon::VFSUrl& url) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_general
  /// @brief Create a directory
  ///
  /// @param[in] url The URL of the file
  /// @return True if creation was successful, false otherwise
  ///
  virtual bool CreateDirectory(const kodi::addon::VFSUrl& url) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_vfs_general_cb_GetDirectory Callbacks GetDirectory()
  /// @ingroup cpp_kodi_addon_vfs_general
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
  /// bool CMyVFS::GetDirectory(const kodi::addon::VFSUrl& url,
  ///                           std::vector<kodi::vfs::CDirEntry>& items,
  ///                           CVFSCallbacks callbacks)
  /// {
  ///   std::string neededString;
  ///   callbacks.GetKeyboardInput("Test", neededString, true);
  ///   if (neededString.empty())
  ///     return false;
  ///
  ///   // Do the work
  ///   ...
  ///   return true;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  class CVFSCallbacks
  {
  public:
    /// @ingroup cpp_kodi_addon_vfs_general_cb_GetDirectory
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
        ::kodi::addon::CAddonBase::m_interface->toKodi->free_string(
            ::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, cInput);
      }
      return ret;
    }

    /// @ingroup cpp_kodi_addon_vfs_general_cb_GetDirectory
    /// @brief Display an error dialog
    ///
    /// @param[in] heading      The heading of the error dialog
    /// @param[in] line1        The first line of the error dialog
    /// @param[in] line2        [opt] The second line of the error dialog
    /// @param[in] line3        [opt] The third line of the error dialog
    ///
    void SetErrorDialog(const std::string& heading,
                        const std::string& line1,
                        const std::string& line2 = "",
                        const std::string& line3 = "")
    {
      m_cb->set_error_dialog(m_cb->ctx, heading.c_str(), line1.c_str(), line2.c_str(),
                             line3.c_str());
    }

    /// @ingroup cpp_kodi_addon_vfs_general_cb_GetDirectory
    /// @brief Prompt the user for authentication of a URL
    ///
    /// @param[in] url The URL
    void RequireAuthentication(const std::string& url)
    {
      m_cb->require_authentication(m_cb->ctx, url.c_str());
    }

    explicit CVFSCallbacks(const VFSGetDirectoryCallbacks* cb) : m_cb(cb) {}

  private:
    const VFSGetDirectoryCallbacks* m_cb;
  };
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_general
  /// @brief List a directory
  ///
  /// @param[in] url The URL of the directory
  /// @param[out] entries The entries in the directory, see
  ///                     @ref cpp_kodi_vfs_CDirEntry "kodi::vfs::CDirEntry"
  ///                     about his content
  /// @param[in] callbacks A callback structure
  /// @return Context for the directory listing
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// ### Callbacks:
  /// @copydetails cpp_kodi_addon_vfs_general_cb_GetDirectory
  ///
  /// **Available callback functions**
  /// | Function: | Description
  /// |--|--
  /// | CVFSCallbacks::GetKeyboardInput | @copybrief CVFSCallbacks::GetKeyboardInput @copydetails CVFSCallbacks::GetKeyboardInput
  /// | CVFSCallbacks::SetErrorDialog | @copybrief CVFSCallbacks::SetErrorDialog @copydetails CVFSCallbacks::SetErrorDialog
  /// | CVFSCallbacks::RequireAuthentication | @copybrief CVFSCallbacks::RequireAuthentication @copydetails CVFSCallbacks::RequireAuthentication
  ///
  virtual bool GetDirectory(const kodi::addon::VFSUrl& url,
                            std::vector<kodi::vfs::CDirEntry>& entries,
                            CVFSCallbacks callbacks)
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_vfs_general
  /// @brief Check if file should be presented as a directory (multiple streams)
  ///
  /// @param[in] url The URL of the file
  /// @param[out] entries The entries in the directory, see
  ///                     @ref cpp_kodi_vfs_CDirEntry "kodi::vfs::CDirEntry"
  ///                     about his content
  /// @param[out] rootPath Path to root directory if multiple entries
  /// @return Context for the directory listing
  ///
  virtual bool ContainsFiles(const kodi::addon::VFSUrl& url,
                             std::vector<kodi::vfs::CDirEntry>& entries,
                             std::string& rootPath)
  {
    return false;
  }
  //----------------------------------------------------------------------------
  //@}

private:
  void SetAddonStruct(KODI_HANDLE instance)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstanceVFS: Creation with empty addon structure not "
                             "allowed, table must be given from Kodi!");

    m_instanceData = static_cast<AddonInstance_VFSEntry*>(instance);
    m_instanceData->toAddon->addonInstance = this;
    m_instanceData->toAddon->open = ADDON_Open;
    m_instanceData->toAddon->open_for_write = ADDON_OpenForWrite;
    m_instanceData->toAddon->read = ADDON_Read;
    m_instanceData->toAddon->write = ADDON_Write;
    m_instanceData->toAddon->seek = ADDON_Seek;
    m_instanceData->toAddon->truncate = ADDON_Truncate;
    m_instanceData->toAddon->get_length = ADDON_GetLength;
    m_instanceData->toAddon->get_position = ADDON_GetPosition;
    m_instanceData->toAddon->get_chunk_size = ADDON_GetChunkSize;
    m_instanceData->toAddon->io_control_get_seek_possible = ADDON_IoControlGetSeekPossible;
    m_instanceData->toAddon->io_control_get_cache_status = ADDON_IoControlGetCacheStatus;
    m_instanceData->toAddon->io_control_set_cache_rate = ADDON_IoControlSetCacheRate;
    m_instanceData->toAddon->io_control_set_retry = ADDON_IoControlSetRetry;
    m_instanceData->toAddon->stat = ADDON_Stat;
    m_instanceData->toAddon->close = ADDON_Close;
    m_instanceData->toAddon->exists = ADDON_Exists;
    m_instanceData->toAddon->clear_out_idle = ADDON_ClearOutIdle;
    m_instanceData->toAddon->disconnect_all = ADDON_DisconnectAll;
    m_instanceData->toAddon->delete_it = ADDON_Delete;
    m_instanceData->toAddon->rename = ADDON_Rename;
    m_instanceData->toAddon->directory_exists = ADDON_DirectoryExists;
    m_instanceData->toAddon->remove_directory = ADDON_RemoveDirectory;
    m_instanceData->toAddon->create_directory = ADDON_CreateDirectory;
    m_instanceData->toAddon->get_directory = ADDON_GetDirectory;
    m_instanceData->toAddon->free_directory = ADDON_FreeDirectory;
    m_instanceData->toAddon->contains_files = ADDON_ContainsFiles;
  }

  inline static VFS_FILE_HANDLE ADDON_Open(const AddonInstance_VFSEntry* instance,
                                           const VFSURL* url)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->Open(url);
  }

  inline static VFS_FILE_HANDLE ADDON_OpenForWrite(const AddonInstance_VFSEntry* instance,
                                                   const VFSURL* url,
                                                   bool overWrite)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)
        ->OpenForWrite(url, overWrite);
  }

  inline static ssize_t ADDON_Read(const AddonInstance_VFSEntry* instance,
                                   VFS_FILE_HANDLE context,
                                   uint8_t* buffer,
                                   size_t uiBufSize)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)
        ->Read(context, buffer, uiBufSize);
  }

  inline static ssize_t ADDON_Write(const AddonInstance_VFSEntry* instance,
                                    VFS_FILE_HANDLE context,
                                    const uint8_t* buffer,
                                    size_t uiBufSize)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)
        ->Write(context, buffer, uiBufSize);
  }

  inline static int64_t ADDON_Seek(const AddonInstance_VFSEntry* instance,
                                   VFS_FILE_HANDLE context,
                                   int64_t position,
                                   int whence)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)
        ->Seek(context, position, whence);
  }

  inline static int ADDON_Truncate(const AddonInstance_VFSEntry* instance,
                                   VFS_FILE_HANDLE context,
                                   int64_t size)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->Truncate(context, size);
  }

  inline static int64_t ADDON_GetLength(const AddonInstance_VFSEntry* instance,
                                        VFS_FILE_HANDLE context)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->GetLength(context);
  }

  inline static int64_t ADDON_GetPosition(const AddonInstance_VFSEntry* instance,
                                          VFS_FILE_HANDLE context)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->GetPosition(context);
  }

  inline static int ADDON_GetChunkSize(const AddonInstance_VFSEntry* instance,
                                       VFS_FILE_HANDLE context)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->GetChunkSize(context);
  }

  inline static bool ADDON_IoControlGetSeekPossible(const AddonInstance_VFSEntry* instance,
                                                    VFS_FILE_HANDLE context)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)
        ->IoControlGetSeekPossible(context);
  }

  inline static bool ADDON_IoControlGetCacheStatus(const struct AddonInstance_VFSEntry* instance,
                                                   VFS_FILE_HANDLE context,
                                                   VFS_CACHE_STATUS_DATA* status)
  {
    kodi::vfs::CacheStatus cppStatus(status);
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)
        ->IoControlGetCacheStatus(context, cppStatus);
  }

  inline static bool ADDON_IoControlSetCacheRate(const struct AddonInstance_VFSEntry* instance,
                                                 VFS_FILE_HANDLE context,
                                                 unsigned int rate)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)
        ->IoControlSetCacheRate(context, rate);
  }

  inline static bool ADDON_IoControlSetRetry(const struct AddonInstance_VFSEntry* instance,
                                             VFS_FILE_HANDLE context,
                                             bool retry)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)
        ->IoControlSetRetry(context, retry);
  }

  inline static int ADDON_Stat(const AddonInstance_VFSEntry* instance,
                               const VFSURL* url,
                               struct STAT_STRUCTURE* buffer)
  {
    kodi::vfs::FileStatus cppBuffer(buffer);
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->Stat(url, cppBuffer);
  }

  inline static bool ADDON_Close(const AddonInstance_VFSEntry* instance, VFS_FILE_HANDLE context)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->Close(context);
  }

  inline static bool ADDON_Exists(const AddonInstance_VFSEntry* instance, const VFSURL* url)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->Exists(url);
  }

  inline static void ADDON_ClearOutIdle(const AddonInstance_VFSEntry* instance)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->ClearOutIdle();
  }

  inline static void ADDON_DisconnectAll(const AddonInstance_VFSEntry* instance)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->DisconnectAll();
  }

  inline static bool ADDON_Delete(const AddonInstance_VFSEntry* instance, const VFSURL* url)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->Delete(url);
  }

  inline static bool ADDON_Rename(const AddonInstance_VFSEntry* instance,
                                  const VFSURL* url,
                                  const VFSURL* url2)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->Rename(url, url2);
  }

  inline static bool ADDON_DirectoryExists(const AddonInstance_VFSEntry* instance,
                                           const VFSURL* url)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->DirectoryExists(url);
  }

  inline static bool ADDON_RemoveDirectory(const AddonInstance_VFSEntry* instance,
                                           const VFSURL* url)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->RemoveDirectory(url);
  }

  inline static bool ADDON_CreateDirectory(const AddonInstance_VFSEntry* instance,
                                           const VFSURL* url)
  {
    return static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)->CreateDirectory(url);
  }

  inline static bool ADDON_GetDirectory(const AddonInstance_VFSEntry* instance,
                                        const VFSURL* url,
                                        VFSDirEntry** retEntries,
                                        int* num_entries,
                                        VFSGetDirectoryCallbacks* callbacks)
  {
    std::vector<kodi::vfs::CDirEntry> addonEntries;
    bool ret = static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)
                   ->GetDirectory(url, addonEntries, CVFSCallbacks(callbacks));
    if (ret)
    {
      VFSDirEntry* entries =
          static_cast<VFSDirEntry*>(malloc(sizeof(VFSDirEntry) * addonEntries.size()));
      for (unsigned int i = 0; i < addonEntries.size(); ++i)
      {
        entries[i].label = strdup(addonEntries[i].Label().c_str());
        entries[i].title = strdup(addonEntries[i].Title().c_str());
        entries[i].path = strdup(addonEntries[i].Path().c_str());
        entries[i].folder = addonEntries[i].IsFolder();
        entries[i].size = addonEntries[i].Size();
        entries[i].date_time = addonEntries[i].DateTime();

        entries[i].num_props = 0;
        const std::map<std::string, std::string>& props = addonEntries[i].GetProperties();
        if (!props.empty())
        {
          entries[i].properties =
              static_cast<VFSProperty*>(malloc(sizeof(VFSProperty) * props.size()));
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
      *num_entries = static_cast<int>(addonEntries.size());
    }
    return ret;
  }

  inline static void ADDON_FreeDirectory(const AddonInstance_VFSEntry* instance,
                                         VFSDirEntry* entries,
                                         int num_entries)
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
    bool ret = static_cast<CInstanceVFS*>(instance->toAddon->addonInstance)
                   ->ContainsFiles(url, addonEntries, cppRootPath);
    if (ret)
    {
      strncpy(rootpath, cppRootPath.c_str(), ADDON_STANDARD_STRING_LENGTH);

      VFSDirEntry* entries =
          static_cast<VFSDirEntry*>(malloc(sizeof(VFSDirEntry) * addonEntries.size()));
      for (size_t i = 0; i < addonEntries.size(); ++i)
      {
        entries[i].label = strdup(addonEntries[i].Label().c_str());
        entries[i].title = strdup(addonEntries[i].Title().c_str());
        entries[i].path = strdup(addonEntries[i].Path().c_str());
        entries[i].folder = addonEntries[i].IsFolder();
        entries[i].size = addonEntries[i].Size();
        entries[i].date_time = addonEntries[i].DateTime();

        entries[i].num_props = 0;
        const std::map<std::string, std::string>& props = addonEntries[i].GetProperties();
        if (!props.empty())
        {
          entries[i].properties =
              static_cast<VFSProperty*>(malloc(sizeof(VFSProperty) * props.size()));
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
      *num_entries = static_cast<int>(addonEntries.size());
    }
    return ret;
  }

  AddonInstance_VFSEntry* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
