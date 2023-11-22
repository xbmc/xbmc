/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

class CURL;
class CAdvancedSettings;
class CFileItem;

/*! \brief Defines the methodology to find hosts on a given subnet 
    \sa IsHostOnLAN
*/
enum class LanCheckMode
{
  /*! \brief Only match if the host is on the same subnet as the kodi host */
  ONLY_LOCAL_SUBNET,
  /*! \brief Match the host if it belongs to any private subnet */
  ANY_PRIVATE_SUBNET
};

class URIUtils
{
public:
  static void RegisterAdvancedSettings(const CAdvancedSettings& advancedSettings);
  static void UnregisterAdvancedSettings();

  static std::string GetDirectory(const std::string &strFilePath);

  static std::string GetFileName(const CURL& url);
  static std::string GetFileName(const std::string& strFileNameAndPath);
  static std::string GetFileOrFolderName(const std::string& path);

  static std::string GetExtension(const CURL& url);
  static std::string GetExtension(const std::string& strFileName);


  /*! \brief Check if the CFileItem has a plugin path.
   \param item The CFileItem.
   \return true if there is a plugin path, false otherwise.
  */
  static bool HasPluginPath(const CFileItem& item);

  /*!
   \brief Check if there is a file extension
   \param strFileName Path or URL to check
   \return \e true if strFileName have an extension.
   \note Returns false when strFileName is empty.
   \sa GetExtension
   */
  static bool HasExtension(const std::string& strFileName);

  /*!
   \brief Check if filename have any of the listed extensions
   \param strFileName Path or URL to check
   \param strExtensions List of '.' prefixed lowercase extensions separated with '|'
   \return \e true if strFileName have any one of the extensions.
   \note The check is case insensitive for strFileName, but requires
         strExtensions to be lowercase. Returns false when strFileName or
         strExtensions is empty.
   \sa GetExtension
   */
  static bool HasExtension(const std::string& strFileName, const std::string& strExtensions);
  static bool HasExtension(const CURL& url, const std::string& strExtensions);

  static void RemoveExtension(std::string& strFileName);
  static std::string ReplaceExtension(const std::string& strFile,
                                     const std::string& strNewExtension);
  static void Split(const std::string& strFileNameAndPath,
                    std::string& strPath, std::string& strFileName);
  static std::vector<std::string> SplitPath(const std::string& strPath);

  static void GetCommonPath(std::string& strParent, const std::string& strPath);
  static std::string GetParentPath(const std::string& strPath);
  static bool GetParentPath(const std::string& strPath, std::string& strParent);

  /*! \brief Retrieve the base path, accounting for stacks and files in rars.
   \param strPath path.
   \return the folder that contains the item.
   */
  static std::string GetBasePath(const std::string& strPath);

  /* \brief Change the base path of a URL: fromPath/fromFile -> toPath/toFile
    Handles changes in path separator and filename URL encoding if necessary to derive toFile.
    \param fromPath the base path of the original URL
    \param fromFile the filename portion of the original URL
    \param toPath the base path of the resulting URL
    \return the full path.
   */
  static std::string ChangeBasePath(const std::string &fromPath, const std::string &fromFile, const std::string &toPath, const bool &bAddPath = true);

  static CURL SubstitutePath(const CURL& url, bool reverse = false);
  static std::string SubstitutePath(const std::string& strPath, bool reverse = false);

  /*! \brief Check whether a URL is a given URL scheme.
   Comparison is case-insensitive as per RFC1738
   \param url a std::string path.
   \param type a lower-case scheme name, e.g. "smb".
   \return true if the url is of the given scheme, false otherwise.
   \sa PathHasParent, PathEquals
   */
  static bool IsProtocol(const std::string& url, const std::string& type);

  /*! \brief Check whether a path has a given parent.
   Comparison is case-sensitive.
   Use IsProtocol() to compare the protocol portion only.
   \param path a std::string path.
   \param parent the string the parent of the path should be compared against.
   \param translate whether to translate any special paths into real paths
   \return true if the path has the given parent string, false otherwise.
   \sa IsProtocol, PathEquals
   */
  static bool PathHasParent(std::string path, std::string parent, bool translate = false);

  /*! \brief Check whether a path equals another path.
   Comparison is case-sensitive.
   \param path1 a std::string path.
   \param path2 the second path the path should be compared against.
   \param ignoreTrailingSlash ignore any trailing slashes in both paths
   \return true if the paths are equal, false otherwise.
   \sa IsProtocol, PathHasParent
   */
  static bool PathEquals(std::string path1, std::string path2, bool ignoreTrailingSlash = false, bool ignoreURLOptions = false);

  static bool IsAddonsPath(const std::string& strFile);
  static bool IsSourcesPath(const std::string& strFile);
  static bool IsCDDA(const std::string& strFile);
  static bool IsDAV(const std::string& strFile);
  static bool IsDOSPath(const std::string &path);
  static bool IsDVD(const std::string& strFile);
  static bool IsFTP(const std::string& strFile);
  static bool IsHTTP(const std::string& strFile, bool bTranslate = false);
  static bool IsUDP(const std::string& strFile);
  static bool IsTCP(const std::string& strFile);
  static bool IsHD(const std::string& strFileName);
  static bool IsInArchive(const std::string& strFile);
  static bool IsInRAR(const std::string& strFile);
  static bool IsInternetStream(const std::string& path, bool bStrictCheck = false);
  static bool IsInternetStream(const CURL& url, bool bStrictCheck = false);
  static bool IsStreamedFilesystem(const std::string& strPath);
  static bool IsNetworkFilesystem(const std::string& strPath);
  static bool IsInAPK(const std::string& strFile);
  static bool IsInZIP(const std::string& strFile);
  static bool IsISO9660(const std::string& strFile);
  static bool IsLiveTV(const std::string& strFile);
  static bool IsPVRRecording(const std::string& strFile);
  static bool IsPVRRecordingFileOrFolder(const std::string& strFile);
  static bool IsPVRTVRecordingFileOrFolder(const std::string& strFile);
  static bool IsPVRRadioRecordingFileOrFolder(const std::string& strFile);
  static bool IsMultiPath(const std::string& strPath);
  static bool IsMusicDb(const std::string& strFile);
  static bool IsNfs(const std::string& strFile);
  static bool IsOnDVD(const std::string& strFile);
  static bool IsOnLAN(const std::string& strFile,
                      LanCheckMode lanCheckMode = LanCheckMode::ONLY_LOCAL_SUBNET);
  static bool IsHostOnLAN(const std::string& hostName,
                          LanCheckMode lanCheckMode = LanCheckMode::ONLY_LOCAL_SUBNET);
  static bool IsPlugin(const std::string& strFile);
  static bool IsScript(const std::string& strFile);
  static bool IsRAR(const std::string& strFile);
  static bool IsRemote(const std::string& strFile);
  static bool IsSmb(const std::string& strFile);
  static bool IsSpecial(const std::string& strFile);
  static bool IsStack(const std::string& strFile);
  static bool IsFavourite(const std::string& strFile);
  static bool IsUPnP(const std::string& strFile);
  static bool IsURL(const std::string& strFile);
  static bool IsVideoDb(const std::string& strFile);
  static bool IsAPK(const std::string& strFile);
  static bool IsZIP(const std::string& strFile);
  static bool IsArchive(const std::string& strFile);
  static bool IsDiscImage(const std::string& file);
  static bool IsDiscImageStack(const std::string& file);
  static bool IsBluray(const std::string& strFile);
  static bool IsAndroidApp(const std::string& strFile);
  static bool IsLibraryFolder(const std::string& strFile);
  static bool IsLibraryContent(const std::string& strFile);
  static bool IsPVR(const std::string& strFile);
  static bool IsPVRChannel(const std::string& strFile);
  static bool IsPVRChannelGroup(const std::string& strFile);
  static bool IsPVRGuideItem(const std::string& strFile);

  static std::string AppendSlash(std::string strFolder);
  static void AddSlashAtEnd(std::string& strFolder);
  static bool HasSlashAtEnd(const std::string& strFile, bool checkURL = false);
  static void RemoveSlashAtEnd(std::string& strFolder);
  static bool CompareWithoutSlashAtEnd(const std::string& strPath1, const std::string& strPath2);
  static std::string FixSlashesAndDups(const std::string& path, const char slashCharacter = '/', const size_t startFrom = 0);
  /**
   * Convert path to form without duplicated slashes and without relative directories
   * Strip duplicated slashes
   * Resolve and remove relative directories ("/../" and "/./")
   * Will ignore slashes with other direction than specified
   * Will not resolve path starting from relative directory
   * @warning Don't use with "protocol://path"-style URLs
   * @param path string to process
   * @param slashCharacter character to use as directory delimiter
   * @return transformed path
   */
  static std::string CanonicalizePath(const std::string& path, const char slashCharacter = '\\');

  static CURL CreateArchivePath(const std::string& type,
                                const CURL& archiveUrl,
                                const std::string& pathInArchive = "",
                                const std::string& password = "");

  static std::string AddFileToFolder(const std::string& strFolder, const std::string& strFile);
  template <typename... T>
  static std::string AddFileToFolder(const std::string& strFolder, const std::string& strFile, T... args)
  {
    auto newPath = AddFileToFolder(strFolder, strFile);
    return AddFileToFolder(newPath, args...);
  }

  static bool HasParentInHostname(const CURL& url);
  static bool HasEncodedHostname(const CURL& url);
  static bool HasEncodedFilename(const CURL& url);

  /*!
   \brief Cleans up the given path by resolving "." and ".."
   and returns it.

   This methods goes through the given path and removes any "."
   (as it states "this directory") and resolves any ".." by
   removing the previous directory from the path. This is done
   for file paths and host names (in case of VFS paths).

   \param path Path to be cleaned up
   \return Actual path without any "." or ".."
   */
  static std::string GetRealPath(const std::string &path);

  /*!
   \brief Updates the URL encoded hostname of the given path

   This method must only be used to update paths encoded with
   the old (Eden) URL encoding implementation to the new (Frodo)
   URL encoding implementation (which does not URL encode -_.!().

   \param strFilename Path to update
   \return True if the path has been updated/changed otherwise false
   */
  static bool UpdateUrlEncoding(std::string &strFilename);

private:
  static std::string resolvePath(const std::string &path);

  static const CAdvancedSettings* m_advancedSettings;
};

