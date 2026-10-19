/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>
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

/*!
 * \brief Kodi mixed native-path/URI operations, separate from strict URI syntax.
 * \note Filename, extension and split operations return no result for identifiers
 *       whose scheme has no filesystem hierarchy. Void path mutators and normalizers
 *       leave such identifiers unchanged, including malformed candidates. Navigable
 *       new URI forms retain raw query/fragment bytes during path changes. Returned
 *       strings own their bytes; no URI decoding is implicit.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-1.2.2
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-5.3
 */
class URIUtils
{
public:
  // Don't URL encode "-_.!()" according to RFC1738
  //! @todo Update it to "-_.~" after Gotham according to RFC3986
  static constexpr std::string_view RFC1738{"-._!()"};
  static constexpr std::string_view RFC3986{"-_.~"};

  static std::string URLEncode(std::string_view strURLData, std::string_view URLSpec = RFC1738);
  static std::string URLDecode(std::string_view strURLData);

  /*! \brief Decode a path component, leaving a literal '+' alone. */
  static std::string DecodePathEscapes(std::string_view strURLData);

  static void RegisterAdvancedSettings(const CAdvancedSettings& advancedSettings);
  static void UnregisterAdvancedSettings();

  /*!
   * \brief Return the directory of a mixed native-path/URI location.
   * \return Empty for identifiers without filesystem-style hierarchy. New URI
   *         forms preserve the raw query and fragment while replacing only the path.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.3
   */
  static std::string GetDirectory(const std::string &strFilePath);

  static std::string GetFileName(const CURL& url);
  static std::string GetFileName(const std::string& strFileNameAndPath);

  /*! \brief The file name with any percent escapes resolved, as a label carries it. */
  static std::string GetDecodedFileName(const std::string& strFileNameAndPath);
  static std::string GetFileOrFolderName(std::string_view path);

  static std::string GetExtension(const CURL& url);
  static std::string GetExtension(const std::string& path);

  /*! \brief Check if the CFileItem has a plugin path.
   \param item The CFileItem.
   \return true if there is a plugin path, false otherwise.
  */
  static bool HasPluginPath(const CFileItem& item);

  /*!
   \brief Check if there is a file extension
   \param path Path or URL to check
   \return \e true if strFileName have an extension.
   \note Returns false when strFileName is empty.
   \sa GetExtension
   */
  static bool HasExtension(const std::string& path);

  /*!
   \brief Check if filename have any of the listed extensions
   \param path Path or URL to check
   \param extensions List of '.' prefixed lowercase extensions separated with '|'
   \return \e true if strFileName have any one of the extensions.
   \note The check is case insensitive for strFileName, but requires
         strExtensions to be lowercase. Returns false when strFileName or
         strExtensions is empty.
   \sa GetExtension
   */
  static bool HasExtension(const std::string& path, std::string_view extensions);
  static bool HasExtension(const CURL& url, std::string_view strExtensions);

  static void RemoveExtension(std::string& path);
  static std::string ReplaceExtension(const std::string& path, const std::string& newExtension);
  static void Split(const std::string& strFileNameAndPath,
                    std::string& strPath, std::string& strFileName);
  static std::vector<std::string> SplitPath(const std::string& strPath);

  /*! \brief Retain the common directory of two mixed native-path/URI locations.
   *  \param parent Receives the common path, or is cleared if either URI has no hierarchy.
   *  \param path The other location to compare with parent.
   *  \note Kodi identity policy requires new URI forms to have matching schemes
   *        and raw authority, query, and fragment; differing identities have no common path.
   *  \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3
   */
  static void GetCommonPath(std::string& parent, std::string_view path);
  static std::string GetParentPath(const std::string& strPath);
  /*!
   * \brief Navigate a mixed native-path/URI location to its parent.
   * \param strPath Location to navigate; this does not resolve relative references.
   * \param strParent Receives the parent, or is cleared on failure.
   * \return False for non-file identifiers and for the root of a new URI path.
   * \note Raw query/fragment preservation during Kodi member navigation is Kodi
   *       policy. URN namespace-specific slashes do not establish file hierarchy.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.3
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.4
   * \sa https://www.rfc-editor.org/rfc/rfc8141.html#section-4.3
   */
  static bool GetParentPath(const std::string& strPath, std::string& strParent);

  /*! \brief Retrieve the base path, accounting for stacks and files in rars.
   \param strPath path.
   \return the folder that contains the item.
   */
  static std::string GetBasePath(const std::string& strPath);

  static bool IsDiscPath(const std::string& path);

  /*! \brief Given a bluray:// path or disc file path (index.bdmv/video_ts.ifo), return the base .ISO or folder
   containing the disc file structure.
   \param file source path.
   \return the base .ISO or folder containing the disc file structure.
   \note Used to determine file/folder to delete
   */
  static std::string GetDiscBase(const std::string& file);

  /*! \brief Given a bluray:// path or disc file path (index.bdmv/video_ts.ifo), return the folder
   containing the disc file structure or ISO.
   \param file source path.
   \return the folder containing the .ISO or disc file structure.
   */
  static std::string GetDiscBasePath(const std::string& file);

  /*! \brief Given a path (index.bdmv/video_ts.ifo), return the base folder
   (ie the one containing the BDMV or VIDEO_TS folder).
   \param path source path.
   \return the base folder.
   */
  static std::string RemoveDiscPath(const std::string& path);

  /*! \brief Given a bluray:// path, return the base .ISO or index.BDMV.
   \param path bluray:// path.
   \return the base .ISO or index.BDMV.
   */
  static std::string GetDiscFile(const std::string& path);

  /*! \brief Given a bluray:// path, return the underlying file path (eg. smb://, udf:// etc..)
   \param url CURL containing bluray:// path.
   \return return the underlying file path.
   */
  static std::string GetDiscUnderlyingFile(const CURL& url);

  /*! \brief Given a path to an .ISO or index.BDMV, returns a bluray:// path to menu.
   \param path the ISO/index.BDMV path.
   \return the bluray:// menu path.
   */
  static std::string GetBlurayMenuPath(const std::string& path);

  enum class GetAllTitles : bool
  {
    LONG,
    ALL
  };

  enum class AllTitlesOptions : bool
  {
    MOVIES,
    EPISODES
  };

  /*! \brief Given a path to an .ISO or index.BDMV, returns a bluray:// path to select titles.
   \param path the ISO/index.BDMV path.
   \param getAllTitles whether to get all titles or just those within 70% of longest (most likely to be movies/episodes)
   \param options movies or episodes - later dictates how to sort the titles - by length (movies) or by playlist (episodes)
   \return the bluray:// titles path.
   */
  static std::string GetBlurayTitlesPath(const std::string& path,
                                         GetAllTitles getAllTitles = GetAllTitles::LONG,
                                         AllTitlesOptions options = AllTitlesOptions::MOVIES);

  /*! \brief Given a path to an .ISO or index.BDMV, returns a bluray:// path to main title.
   \param path the ISO/index.BDMV path.
   \param getAllTitles whether to get a single title or those within 70% of longest (most likely to be movies/episodes)
   \return the bluray:// root/main path.
   */
  static std::string GetBlurayMainTitlePath(const std::string& path,
                                            GetAllTitles getAllTitles = GetAllTitles::LONG);

  /*! \brief Given a path to an .ISO or index.BDMV, returns a bluray:// path to a given episode.
   \param path the ISO/index.BDMV path.
   \return the bluray:// root/episode/<season number>/<episode number> path.
   */
  static std::string GetBlurayEpisodePath(const std::string& path, int season, int episode);

  /*! \brief Given a path to an .ISO or index.BDMV, returns a bluray:// for all episodes on disc.
   \param path the ISO/index.BDMV path.
   \return the bluray:// root/episode/all path.
   */
  static std::string GetBlurayAllEpisodesPath(const std::string& path);

  /*! \brief Given a path to an .ISO or index.BDMV, returns a bluray:// path to default playlist path.
   \param path the ISO/index.BDMV path.
   \param playlist (optional) the .mpls playlist
   \return the bluray:// playlist path - BDMV/PLAYLIST(/xxxxx.mpls)
   */
  static std::string GetBlurayPlaylistPath(const std::string& path, int playlist = -1);

  /*! \brief Given a path to bluray playlist (bluray://.../xxxxx.mpls), returns the playlist number.
   \param path the bluray:// path
   \return the playlist number
   */
  static int GetBlurayPlaylistFromPath(const std::string& path);

  /*! \brief Resolve two paths to their disc base (if needed) and compare for equality.
   \param path1 first path to resolve and compare
   \param path2 second path to resolve and compare
   \return the playlist number
   */
  static bool CompareDiscPaths(const std::string& path1, const std::string& path2);

  /*! \brief Get the regex for matching trailing part numbers.
   \return the regex
   */
  static std::string GetTitleTrailingPartNumberRegex();

  /*! \brief Get the regex for matching trailing part numbers including leading path separator.
   \return the regex
   */
  static std::string GetTrailingPartNumberRegex();

  /* \brief Change the base path of a URL: fromPath/fromFile -> toPath/toFile
    Handles changes in path separator and filename URL encoding if necessary to derive toFile.
    \param fromPath the base path of the original URL
    \param fromFile the filename portion of the original URL
    \param toPath the base path of the resulting URL
    \return the full path.
   */
  static std::string ChangeBasePath(const std::string &fromPath, const std::string &fromFile, const std::string &toPath, const bool &bAddPath = true);

  /*! \brief Apply path mappings while preserving explicit intent for unchanged locations.
   * \param url The native path or URI to substitute.
   * \param reverse Whether to invert the configured source and target mappings.
   * \note Kodi guarantees that an unchanged location retains its original CURL
   *       representation. Actual replacements use the target's mixed native-path/URI
   *       interpretation, independently of the original input mode.
   * \sa CURL::ParseURI, CURL::ParseNativePath
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.1
   */
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

  /*! \brief Check equality or directory ancestry for mixed native-path/URI locations.
   Comparison is case-sensitive, except for the scheme of new URI forms.
   Use IsProtocol() to compare the protocol portion only.
   \param path a std::string path.
   \param parent the string the parent of the path should be compared against.
   \param translate whether to translate any special paths into real paths
   \return true for identical nonempty locations or a matching directory ancestor.
   \note Identical non-file identifiers retain the existing equality convention;
         distinct non-file identifiers have no ancestry. Kodi identity policy requires
         new URI forms to match raw authority, query, and fragment, including presence,
         before comparing raw paths at directory boundaries.
   \sa IsProtocol, PathEquals
   \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.3
   \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.4
   \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.5
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
  static bool IsDOSPath(const std::string& path);
  static bool IsAbsolutePOSIXPath(const std::string& path);
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
  static bool IsLocalOrLAN(const std::string& path);
  static bool IsPlugin(const std::string& strFile);
  static bool IsScript(const std::string& strFile);
  static bool IsRAR(const std::string& strFile);
  static bool IsRemote(const std::string& strFile);
  static bool IsSmb(const std::string& strFile);
  static bool IsSpecial(const std::string& strFile);
  static bool IsStack(const std::string& strFile);
  static bool IsContainerPath(const std::string& strFile);
  static bool IsFavourite(const std::string& strFile);
  static bool IsUPnP(const std::string& strFile);
  /*!
   * \brief Recognize Kodi's legacy leading scheme:// form in mixed path input.
   * \param file URI or native path; DOS paths take native precedence.
   * \return True only when // immediately follows the leading scheme colon.
   * \note This compatibility predicate does not validate URI syntax or imply a backend.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.1
   */
  static bool IsURL(std::string_view file);
  static bool IsVideoDb(const std::string& strFile);
  static bool IsAPK(const std::string& strFile);
  static bool IsZIP(const std::string& strFile);
  static bool IsArchive(const std::string& strFile);
  static bool IsArchive(const CURL& url);
  static bool IsDiscImage(const std::string& file);
  static bool IsDiscImageStack(const std::string& file);
  static bool IsBlurayPath(const std::string& strFile);
  static bool IsBlurayMenuPath(const std::string& file);

  /*! \brief Determines if a file is from optical media (ie. index.bdmv, video_ts.ifo etc..)
   \param file file path for determination.
   \return true if file is from optical media
   */
  static bool IsOpticalMediaFile(const std::string& file);

  static bool IsBDFile(const std::string& file);
  static bool IsDVDFile(const std::string& file);
  static bool IsAndroidApp(const std::string& strFile);
  static bool IsLibraryFolder(const std::string& strFile);
  static bool IsLibraryContent(const std::string& strFile);
  static bool IsPVR(const std::string& strFile);
  static bool IsPVRChannel(const std::string& strFile);
  static bool IsPVRRadioChannel(const std::string& strFile);
  static bool IsPVRChannelGroup(const std::string& strFile);
  static bool IsPVRGuideItem(const std::string& strFile);

  static std::string AppendSlash(std::string strFolder);
  static void AddSlashAtEnd(std::string& strFolder);
  static bool HasSlashAtEnd(std::string_view file, bool checkURL = false);
  static void RemoveSlashAtEnd(std::string& strFolder);
  static bool CompareWithoutSlashAtEnd(const std::string& strPath1, const std::string& strPath2);
  static std::string FixSlashesAndDups(const std::string& path, const char slashCharacter = '/', const size_t startFrom = 0);
  /**
   * Convert path to form without duplicated slashes and without relative directories
   * Strip duplicated slashes
   * Resolve and remove relative directories ("/../" and "/./")
   * Will ignore slashes with other direction than specified
   * Will not resolve path starting from relative directory
   * URI operations apply only to the path component; non-file identifiers are unchanged.
   * @param path string to process
   * @param slashCharacter character to use as directory delimiter
   * @return transformed path
   */
  static std::string CanonicalizePath(const std::string& path, const char slashCharacter = '\\');

  static CURL CreateArchivePath(const std::string& type,
                                const CURL& archiveUrl,
                                const std::string& pathInArchive = "",
                                const std::string& password = "");

  /*!
   * \brief Append a path component to a mixed native-path/URI location.
   * \param strFolder Location whose scheme permits filesystem-style navigation.
   * \param strFile Path component, not a separately recognized URI.
   * \return The combined location, or empty when the folder has no hierarchy or
   *         a raw URI path cannot represent the supplied component.
   * \note New URI forms preserve raw query and fragment bytes. Keeping a magnet
   *       query during Kodi member navigation is Kodi policy; BEP 9 defines the
   *       external empty-path identifier, not member locations.
   * \sa https://www.bittorrent.org/beps/bep_0009.html#magnet-uri-format
   */
  static std::string AddFileToFolder(const std::string& strFolder, const std::string& strFile);
  template<typename... T>
  static std::string AddFileToFolder(const std::string& strFolder,
                                     const std::string& strFile,
                                     const T&... args)
  {
    auto newPath = AddFileToFolder(strFolder, strFile);
    if (newPath.empty())
      return {};
    return AddFileToFolder(newPath, args...);
  }

  /*!
   \brief Append a segment to a folder, matching the folder's filename encoding.

   Sources whose paths keep the filename portion percent-encoded (WebDAV, http, ...)
   cannot resolve a path built by appending a raw, human readable segment, because the
   result ends up with mixed encoding. This helper percent-encodes \p strFile when
   \p strFolder reports an encoded filename and appends it verbatim otherwise.

   \param strFolder base folder, either a local path or a URL
   \param strFile raw (unencoded) segment to append
   \return the combined path, consistently encoded
   */
  static std::string AddFileToFolderMatchingEncoding(const std::string& strFolder,
                                                     const std::string& strFile);

  static bool HasParentInHostname(const CURL& url);
  static bool HasEncodedFilename(const CURL& url);

  /*!
   \brief Cleans up the given path by resolving "." and ".."
   and returns it.

   This methods goes through the given path and removes any "."
   (as it states "this directory") and resolves any ".." by
   removing the previous directory from the path. This is done
   for file paths and host names (in case of VFS paths).

   \param path Path to be cleaned up
   \return Actual path without any "." or "..", or the unchanged non-file identifier.
   \note Only explicit Kodi nested-location wrappers recurse into their hostname;
         new URI forms change their path without altering raw query or fragment.
   */
  static std::string GetRealPath(const std::string &path);

  static CURL AddCredentials(CURL url);

  /*!
   \brief Updates the URL encoding (if needed) of the hostname element of the given url path 
   and returns the updated path.

   Ensures that the hex encoded characters are lower case
   Ensures that DOS paths use '\' as path separator and not '/'
   as this can cause unintentional path mismatches

   \param path Path to update
   \return Updated path
   */
  static std::string SanitiseUrlEncoding(std::string_view path);

  static std::string GetDecodedPath(const std::string& path);

private:
  static std::string resolvePath(const std::string &path);

  /*! \brief Given a path to an .ISO or index.BDMV, returns a bluray:// path.
   \param path the ISO/index.BDMV path.
   \return the bluray:// path.
   */
  static std::string GetBlurayPath(const std::string& path);

  static const CAdvancedSettings* m_advancedSettings;
};

