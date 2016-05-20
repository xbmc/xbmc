#pragma once
/*
 *      Copyright (C) 2016 Team KODI
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

#include "../definitions.hpp"
#include "../.internal/AddonLib_internal.hpp"

#include <vector>

API_NAMESPACE

namespace KodiAPI
{
namespace AddOn
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AddOn_VFS Virtual file system
  /// \ingroup CPP_KodiAPI_AddOn
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AddOn_CVFSDirEntry class CVFSDirEntry
  /// \ingroup CPP_KodiAPI_AddOn_VFS
  /// @{
  /// @brief <b>Virtual file system directory entry</b>
  ///
  /// This class is used as an entry for files and folders in
  /// KodiAPI::AddOn::CVFSUtils::GetDirectory(...).
  ///
  ///
  /// ------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api2/addon/VFSUtils.hpp>
  ///
  /// std::vector<KodiAPI::AddOn::CVFSDirEntry> items;
  /// KodiAPI::AddOn::CVFSUtils::GetDirectory("special://temp", "", items);
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
  ///
  /// It has the header \ref VFSUtils.hpp "#include <kodi/api2/addon/VFSUtils.hpp>" be included
  /// to enjoy it.
  ///
  class CVFSDirEntry
  {
  public:
    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSDirEntry
    /// @brief Constructor for VFS directory entry
    ///
    /// @param[in] label   [opt] Name to use for entry
    /// @param[in] path    [opt] Used path of the entry
    /// @param[in] bFolder [opt] If set entry used as folder
    /// @param[in] size    [opt] If used as file, his size defined there
    ///
    CVFSDirEntry(
        const std::string&  label   = "",
        const std::string&  path    = "",
        bool                bFolder = false,
        int64_t             size    = -1);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSDirEntry
    /// @brief Constructor to create own copy
    ///
    /// @param[in] dirEntry pointer to own class type
    ///
    CVFSDirEntry(const VFSDirEntry& dirEntry);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSDirEntry
    /// @brief Get the directory entry name
    ///
    /// @return Name of the entry
    ///
    const std::string& Label(void) const;
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSDirEntry
    /// @brief Get the path of the entry
    ///
    /// @return File system path of the entry
    ///
    const std::string& Path(void) const;
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSDirEntry
    /// @brief Used to check entry is folder
    ///
    /// @return true if entry is a folder
    ///
    bool IsFolder(void) const;
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSDirEntry
    /// @brief If file, the size of the file
    ///
    /// @return Defined file size
    ///
    int64_t Size(void) const;
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSDirEntry
    /// @brief Set the label name
    ///
    /// @param[in] label name of entry
    ///
    void SetLabel(const std::string& label);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSDirEntry
    /// @brief Set the path of the entry
    ///
    /// @param[in] path path of entry
    ///
    void SetPath(const std::string& path);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSDirEntry
    /// @brief Set the entry defined as folder
    ///
    /// @param[in] bFolder If true becomes entry defined as folder
    ///
    void SetFolder(bool bFolder);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSDirEntry
    /// @brief Set a file size for a new entry
    ///
    /// @param[in] size Size to set for dir entry
    ///
    void SetSize(int64_t size);
    //----------------------------------------------------------------------------

  #ifndef DOXYGEN_SHOULD_SKIP_THIS
    IMPL_VFS_DIR_ENTRY;
  #endif
  };
  /// @}


  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AddOn_CVFSFile class CVFSFile
  /// \ingroup CPP_KodiAPI_AddOn_VFS
  /// @{
  /// @brief <b>Virtual file system control</b>
  ///
  /// It has the header \ref VFSUtils.hpp "#include <kodi/api2/addon/VFSUtils.hpp>" be included
  /// to enjoy it.
  ///
  class CVFSFile
  {
  public:
    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Construct a new, unopened file
    ///
    CVFSFile();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Close() is called from the destructor, so explicitly closing the
    /// file isn't required
    ///
    virtual ~CVFSFile();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Open the file with filename via KODI's CFile. Needs to be closed by
    /// calling CloseFile() when done.
    ///
    /// @param[in] strFileName The filename to open.
    /// @param[in] flags The flags to pass. Documented in KODI's VFSUtils.hpp
    /// @return True on success or false on failure
    ///
    bool OpenFile(const std::string& strFileName, unsigned int flags = 0);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Open the file with filename via KODI's CFile  in write mode.  Needs
    /// to be closed by calling CloseFile() when done.
    ///
    /// @param[in] strFileName The filename to open.
    /// @param[in] bOverWrite True to overwrite, false otherwise.
    /// @return True on success or false on failure
    ///
    bool OpenFileForWrite(const std::string& strFileName, bool bOverWrite = false);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Read from an open file.
    ///
    /// @param[in] lpBuf The buffer to store the data in.
    /// @param[in] uiBufSize The size of the buffer.
    /// @return number of successfully read bytes if any bytes were read and stored in
    ///         buffer, zero if no bytes are available to read (end of file was reached)
    ///         or undetectable error occur, -1 in case of any explicit error
    ///
    ssize_t Read(void* lpBuf, size_t uiBufSize);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Read a string from an open file.
    ///
    /// @param[in] strLine The buffer to store the data in.
    /// @return True when a line was read, false otherwise.
    ///
    bool ReadLine(std::string &strLine);

    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Write to a file opened in write mode.
    ///
    /// @param[in] lpBuf The data to write.
    /// @param[in] uiBufSize Size of the data to write.
    /// @return number of successfully written bytes if any bytes were written,
    ///         zero if no bytes were written and no detectable error occur,
    ///         -1 in case of any explicit error
    ///
    ssize_t Write(const void* lpBuf, size_t uiBufSize);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Flush buffered data.
    ///
    void Flush();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Seek in an open file.
    ///
    /// @param[in] iFilePosition The new position.
    /// @param[in] iWhence Seek argument. See stdio.h for possible values.
    /// @return The new position.
    ///
    int64_t Seek(int64_t iFilePosition, int iWhence);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Truncate a file to the requested size.
    ///
    /// @param[in] iSize The new max size.
    /// @return New size?
    ///
    int Truncate(int64_t iSize);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief The current position in an open file.
    ///
    /// @return The requested position.
    ///
    int64_t GetPosition();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Get the file size of an open file.
    ///
    /// @return The requested size.
    ///
    int64_t GetLength();

    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Close an open file.
    ///
    void Close();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_CVFSFile
    /// @brief Get the chunk size for an open file.
    ///
    /// @return The requested size.
    ///
    int GetChunkSize();
    //----------------------------------------------------------------------------
  #ifndef DOXYGEN_SHOULD_SKIP_THIS
    IMPL_FILE;
  #endif
  };
  /// @}


  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AddOn_VFSUtils General VFS Utilities
  /// \ingroup CPP_KodiAPI_AddOn_VFS
  /// @{
  /// @brief <b>Virtual file system utilities</b>
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref VFSUtils.hpp "#include <kodi/api2/addon/VFSUtils.hpp>" be included
  /// to enjoy it.
  ///
  namespace VFSUtils
  {
    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Make a directory
    ///
    /// The KodiAPI::AddOn::VFSUtils::CreateDirectory(...) function shall create a
    /// new directory with name path.
    ///
    /// The newly created directory shall be an empty directory.
    ///
    /// @param[in] strPath Path to the directory.
    /// @return  Upon successful completion, CreateDirectory() shall return true.
    ///          Otherwise,false shall be returned, no directory shall be created,
    ///          and errno shall be set to indicate the error.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// ...
    /// std::string directory = "C:\\my_dir";
    /// bool ret = KodiAPI::AddOn::VFSUtils::CreateDirectory(directory);
    /// fprintf(stderr, "Directory '%s' successfull created: %s\n", directory.c_str(), ret ? "yes" : "no");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    bool CreateDirectory(const std::string& strPath);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Verifying the Existence of a Directory
    ///
    /// The KodiAPI::AddOn::VFSUtils::DirectoryExists(...) method determines whether
    /// a specified folder exists.
    ///
    /// @param[in] strPath Path to the directory.
    /// @return True when it exists, false otherwise.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// ...
    /// std::string directory = "C:\\my_dir";
    /// bool ret = KodiAPI::AddOn::VFSUtils::DirectoryExists(directory);
    /// fprintf(stderr, "Directory '%s' present: %s\n", directory.c_str(), ret ? "yes" : "no");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    bool DirectoryExists(const std::string& strPath);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Removes a directory.
    ///
    /// The KodiAPI::AddOn::VFSUtils::RemoveDirectory(...) function shall remove a
    /// directory whose name is given by path.
    ///
    /// @param[in] strPath Path to the directory.
    /// @return  Upon successful completion, the function RemoveDirectory() shall
    ///          return true. Otherwise, false shall be returned, and errno set
    ///          to indicate the error. If false is returned, the named directory
    ///          shall not be changed.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// ...
    /// bool ret = KodiAPI::AddOn::VFSUtils::RemoveDirectory("C:\\my_dir");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    bool RemoveDirectory(const std::string& strPath);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
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
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    ///
    /// std::vector<KodiAPI::AddOn::CVFSDirEntry> items;
    /// KodiAPI::AddOn::VFSUtils::GetDirectory("special://temp", "", items);
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
    ///
    bool GetDirectory(
               const std::string&          path,
               const std::string&          mask,
               std::vector<CVFSDirEntry>&  items);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Check if a file exists.
    ///
    /// @param[in] strFileName The filename to check.
    /// @param[in] bUseCache Check in file cache.
    /// @return true if the file exists false otherwise.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// ...
    /// bool exists = KodiAPI::AddOn::VFSUtils::FileExists("special://temp/kodi.log");
    /// fprintf(stderr, "Log file should be always present, is it present? %s\n", exists ? "yes" : "no");
    /// ~~~~~~~~~~~~~
    ///
    bool FileExists(const std::string& strFileName, bool bUseCache = false);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Get file status.
    ///
    /// These function return information about a file. Execute (search)
    /// permission is required on all of the directories in path that
    /// lead to the file.
    ///
    /// The call return a stat structure, which contains the following fields:
    ///
    /// ~~~~~~~~~~~~~{.cpp}
    /// struct __stat64
    /// {
    ///   dev_t     st_dev;     // ID of device containing file
    ///   ino_t     st_ino;     // inode number
    ///   mode_t    st_mode;    // protection
    ///   nlink_t   st_nlink;   // number of hard links
    ///   uid_t     st_uid;     // user ID of owner
    ///   gid_t     st_gid;     // group ID of owner
    ///   dev_t     st_rdev;    // device ID (if special file)
    ///   off_t     st_size;    // total size, in bytes
    ///   blksize_t st_blksize; // blocksize for file system I/O
    ///   blkcnt_t  st_blocks;  // number of 512B blocks allocated
    ///   time_t    st_atime;   // time of last access
    ///   time_t    st_mtime;   // time of last modification
    ///   time_t    st_ctime;   // time of last status change
    /// };
    /// ~~~~~~~~~~~~~
    ///
    /// The st_dev field describes the device on which this file resides.
    /// The st_rdev field describes the device that this file (inode) represents.
    ///
    /// The st_size field gives the size of the file (if it is a regular file or
    /// a symbolic link) in bytes. The size of a symbolic link (only on Linux
    /// and Mac OS X) is the length of the pathname it contains, without a
    /// terminating null byte.
    ///
    /// The st_blocks field indicates the number of blocks allocated to the file,
    /// 512-byte units. (This may be smaller than st_size/512 when the file has
    /// holes.).
    ///
    /// The st_blksize field gives the "preferred" blocksize for efficient file
    /// system I/O. (Writing to a file in smaller chunks may cause an inefficient
    /// read-modify-rewrite.)
    ///
    /// @warning Not all of the OS file systems implement all of the time fields.
    ///
    /// @param[in] strFileName The filename to read the status from.
    /// @param[in] buffer The file status is written into this buffer.
    /// @return On success, zero is returned. On error, -1 is returned, and errno
    /// is set appropriately.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// ...
    /// struct __stat64 statFile;
    /// int ret = KodiAPI::AddOn::VFSUtils::StatFile("special://temp/kodi.log", &statFile);
    /// fprintf(stderr, "st_dev (ID of device containing file)       = %lu\n"
    ///                 "st_ino (inode number)                       = %lu\n"
    ///                 "st_mode (protection)                        = %u\n"
    ///                 "st_nlink (number of hard links)             = %lu\n"
    ///                 "st_uid (user ID of owner)                   = %u\n"
    ///                 "st_gid (group ID of owner)                  = %u\n"
    ///                 "st_rdev (device ID (if special file))       = %lu\n"
    ///                 "st_size (total size, in bytes)              = %lu\n"
    ///                 "st_blksize (blocksize for file system I/O)  = %lu\n"
    ///                 "st_blocks (number of 512B blocks allocated) = %lu\n"
    ///                 "st_atime (time of last access)              = %lu\n"
    ///                 "st_mtime (time of last modification)        = %lu\n"
    ///                 "st_ctime (time of last status change)       = %lu\n"
    ///                 "Return value                                = %i\n",
    ///                      statFile.st_dev,
    ///                      statFile.st_ino,
    ///                      statFile.st_mode,
    ///                      statFile.st_nlink,
    ///                      statFile.st_uid,
    ///                      statFile.st_gid,
    ///                      statFile.st_rdev,
    ///                      statFile.st_size,
    ///                      statFile.st_blksize,
    ///                      statFile.st_blocks,
    ///                      statFile.st_atime,
    ///                      statFile.st_mtime,
    ///                      statFile.st_ctime,
    ///                      ret);
    /// ~~~~~~~~~~~~~
    ///
    int StatFile(const std::string& strFileName, struct __stat64* buffer);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Deletes a file.
    ///
    /// @param[in] strFileName The filename to delete.
    /// @return The file was successfully deleted.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// #include <kodi/api2/gui/DialogFileBrowser.h>
    /// #include <kodi/api2/gui/DialogOK.h>
    /// ...
    /// std::string filename;
    /// if (KodiAPI::GUI::CAddonGUIDialogFileBrowser::ShowAndGetFile("local", "",
    ///                                                "Test File selection and delete of them!",
    ///                                                filename))
    /// {
    ///   bool successed = KodiAPI::AddOn::VFSUtils::DeleteFile(filename);
    ///   if (!successed)
    ///     KodiAPI::GUI::CAddonGUIDialogOK::ShowAndGetInput("Error", "Delete of File", filename, "failed!");
    ///   else
    ///     KodiAPI::GUI::CAddonGUIDialogOK::ShowAndGetInput("Information", "Delete of File", filename, "successfull done.");
    /// }
    /// ~~~~~~~~~~~~~
    ///
    bool DeleteFile(const std::string& strFileName);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Retrieve MD5sum of a file
    ///
    /// @param[in] strPath path to the file to MD5sum
    /// @return md5 sum of the file
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// #include <kodi/api2/gui/DialogFileBrowser.h>
    /// ...
    /// std::string md5;
    /// std::string filename;
    /// if (KodiAPI::GUI::CAddonGUIDialogFileBrowser::ShowAndGetFile("local", "*.avi|*.mpg|*.mp4",
    ///                                                "Test File selection to get MD5",
    ///                                                filename))
    /// {
    ///   md5 = KodiAPI::AddOn::VFSUtils::GetFileMD5(filename);
    ///   fprintf(stderr, "MD5 of file '%s' is %s\n", md5.c_str(), filename.c_str());
    /// }
    /// ~~~~~~~~~~~~~
    ///
    std::string GetFileMD5(const std::string& strPath);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Returns a thumb cache filename
    ///
    /// @param[in] strFileName path to file
    /// @return cache filename
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// #include <kodi/api2/gui/DialogFileBrowser.h>
    /// ...
    /// std::string thumb;
    /// std::string filename;
    /// if (KodiAPI::GUI::CAddonGUIDialogFileBrowser::ShowAndGetFile("local", "*.avi|*.mpg|*.mp4",
    ///                                                "Test File selection to get Thumnail",
    ///                                                filename))
    /// {
    ///   thumb = KodiAPI::AddOn::VFSUtils::GetCacheThumbName(filename);
    ///   fprintf(stderr, "Thumb name of file '%s' is %s\n", thumb.c_str(), filename.c_str());
    /// }
    /// ~~~~~~~~~~~~~
    ///
    std::string GetCacheThumbName(const std::string& strFileName);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Make filename valid
    ///
    /// Function to replace not valid characters with '_'. It can be also
    /// compared with original before in a own loop until it is equal
    /// (no invalid characters).
    ///
    /// @param[in] strFileName Filename to check and fix
    /// @return            The legal filename
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// ...
    /// std::string fileName = "///\\jk???lj????.mpg";
    /// std::string legalName = KodiAPI::AddOn::VFSUtils::MakeLegalFileName(fileName);
    /// fprintf(stderr, "Legal name of '%s' is '%s'\n", fileName.c_str(), legalName.c_str());
    ///
    /// /* Returns as legal: 'jk___lj____.mpg' */
    /// ~~~~~~~~~~~~~
    ///
    std::string MakeLegalFileName(const std::string& strFileName);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Make directory name valid
    ///
    /// Function to replace not valid characters with '_'. It can be also
    /// compared with original before in a own loop until it is equal
    /// (no invalid characters).
    ///
    /// @param[in] strPath Directory name to check and fix
    /// @return        The legal directory name
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// ...
    /// std::string path = "///\\jk???lj????\\hgjkg";
    /// std::string legalPath = KodiAPI::AddOn::VFSUtils::MakeLegalPath(path);
    /// fprintf(stderr, "Legal name of '%s' is '%s'\n", path.c_str(), legalPath.c_str());
    ///
    /// /* Returns as legal: '/jk___lj____/hgjkg' */
    /// ~~~~~~~~~~~~~
    ///
    std::string MakeLegalPath(const std::string& strPath);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Return a size aligned to the chunk size at least as large as the chunk size.
    ///
    /// @param[in] chunk The chunk size
    /// @param[in] minimum The minimum size (or maybe the minimum number of chunks?)
    /// @return The aligned size
    ///
    unsigned int GetChunkSize(unsigned int chunk, unsigned int minimum);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Return the file name from given complate path string
    ///
    /// @param[in] path The complete path include file and directory
    /// @return Filename from path
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// ...
    /// std::string fileName = KodiAPI::AddOn::VFSUtils::GetFileName("special://temp/kodi.log");
    /// fprintf(stderr, "File name is '%s'\n", fileName.c_str());
    /// ~~~~~~~~~~~~~
    ///
    std::string GetFileName(const std::string& path);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_VFSUtils
    /// @brief Return the directory name from given complate path string
    ///
    /// @param[in] path The complete path include file and directory
    /// @return Directory name from path
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api2/addon/VFSUtils.hpp>
    /// ...
    /// std::string dirName = KodiAPI::AddOn::VFSUtils::GetDirectoryName("special://temp/kodi.log");
    /// fprintf(stderr, "Directory name is '%s'\n", dirName.c_str());
    /// ~~~~~~~~~~~~~
    ///
    std::string GetDirectoryName(const std::string& path);
    //----------------------------------------------------------------------------
  };
  /// @}

} /* namespace AddOn */
} /* namespace KodiAPI */

END_NAMESPACE()
