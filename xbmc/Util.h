/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <climits>
#include <cmath>
#include <vector>
#include <string.h>
#include <stdint.h>
#include "MediaSource.h" // Definition of VECSOURCES
#include "utils/Digest.h"

#define ARRAY_SIZE(X)         (sizeof(X)/sizeof((X)[0]))

// A list of filesystem types for LegalPath/FileName
#define LEGAL_NONE            0
#define LEGAL_WIN32_COMPAT    1
#define LEGAL_FATX            2

class CFileItemList;
class CURL;

struct ExternalStreamInfo
{
  std::string name;
  std::string language;
  unsigned int flag = 0;
};

class CUtil
{
  CUtil() = delete;
public:
  static void CleanString(const std::string& strFileName,
                          std::string& strTitle,
                          std::string& strTitleAndYear,
                          std::string& strYear,
                          bool bRemoveExtension = false,
                          bool bCleanChars = true);
  static std::string GetTitleFromPath(const CURL& url, bool bIsFolder = false);
  static std::string GetTitleFromPath(const std::string& strFileNameAndPath, bool bIsFolder = false);
  static void GetQualifiedFilename(const std::string &strBasePath, std::string &strFilename);
  static void RunShortcut(const char* szPath);
  static std::string GetHomePath(std::string strTarget = "KODI_HOME"); // default target is "KODI_HOME"
  static bool IsPVR(const std::string& strFile);
  static bool IsHTSP(const std::string& strFile);
  static bool IsLiveTV(const std::string& strFile);
  static bool IsTVRecording(const std::string& strFile);
  static bool ExcludeFileOrFolder(const std::string& strFileOrFolder, const std::vector<std::string>& regexps);
  static void GetFileAndProtocol(const std::string& strURL, std::string& strDir);
  static int GetDVDIfoTitle(const std::string& strPathFile);

  static bool IsPicture(const std::string& strFile);
  /// Get resolved filesystem location of splash image
  static std::string GetSplashPath();

  /*! \brief retrieve MD5sum of a file
   \param strPath - path to the file to MD5sum
   \return md5 sum of the file
   */
  static std::string GetFileDigest(const std::string& strPath, KODI::UTILITY::CDigest::Type type);
  static bool GetDirectoryName(const std::string& strFileName, std::string& strDescription);
  static void GetDVDDriveIcon(const std::string& strPath, std::string& strIcon);
  static void RemoveTempFiles();
  static void ClearTempFonts();

  static void ClearSubtitles();
  static void ScanForExternalSubtitles(const std::string& strMovie, std::vector<std::string>& vecSubtitles );

  /** \brief Retrieves stream info of external associated files, e.g., subtitles, for a given video.
  *   \param[in] videoPath The full path of the video file.
  *   \param[in] associatedFile A file that provides additional streams for the given video file.
  *   \return stream info for the given associatedFile
  */
  static ExternalStreamInfo GetExternalStreamDetailsFromFilename(const std::string& videoPath, const std::string& associatedFile);
  static bool FindVobSubPair( const std::vector<std::string>& vecSubtitles, const std::string& strIdxPath, std::string& strSubPath );
  static bool IsVobSub(const std::vector<std::string>& vecSubtitles, const std::string& strSubPath);
  static std::string GetVobSubSubFromIdx(const std::string& vobSubIdx);
  static std::string GetVobSubIdxFromSub(const std::string& vobSub);

  /** \brief Retrieves paths of external audio files for a given video.
  *   \param[in] videoPath The full path of the video file.
  *   \param[out] vecAudio A vector containing the full paths of all found external audio files.
  */
  static void ScanForExternalAudio(const std::string& videoPath, std::vector<std::string>& vecAudio);
  static void ScanForExternalDemuxSub(const std::string& videoPath, std::vector<std::string>& vecSubtitles);
  static int64_t ToInt64(uint32_t high, uint32_t low);
  static std::string GetNextFilename(const std::string &fn_template, int max);
  static std::string GetNextPathname(const std::string &path_template, int max);
  static void StatToStatI64(struct _stati64 *result, struct stat *stat);
  static void StatToStat64(struct __stat64 *result, const struct stat *stat);
  static void Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat);
  static void StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat);
  static void Stat64ToStat(struct stat *result, struct __stat64 *stat);
#ifdef TARGET_WINDOWS
  static void Stat64ToStat64i32(struct _stat64i32 *result, struct __stat64 *stat);
#endif
  static bool CreateDirectoryEx(const std::string& strPath);

#ifdef TARGET_WINDOWS
  static std::string MakeLegalFileName(const std::string &strFile, int LegalType=LEGAL_WIN32_COMPAT);
  static std::string MakeLegalPath(const std::string &strPath, int LegalType=LEGAL_WIN32_COMPAT);
#else
  static std::string MakeLegalFileName(const std::string &strFile, int LegalType=LEGAL_NONE);
  static std::string MakeLegalPath(const std::string &strPath, int LegalType=LEGAL_NONE);
#endif
  static std::string ValidatePath(const std::string &path, bool bFixDoubleSlashes = false); ///< return a validated path, with correct directory separators.

  static bool IsUsingTTFSubtitles();

  /*! \brief Split a comma separated parameter list into separate parameters.
   Takes care of the case where we may have a quoted string containing commas, or we may
   have a function (i.e. parentheses) with multiple parameters as a single parameter.

   eg:

    foo, bar(param1, param2), foo

   will return:

    "foo", "bar(param1, param2)", and "foo".

   \param paramString the string to break up
   \param parameters the returned parameters
   */
  static void SplitParams(const std::string &paramString, std::vector<std::string> &parameters);
  static void SplitExecFunction(const std::string &execString, std::string &function, std::vector<std::string> &parameters);
  static int GetMatchingSource(const std::string& strPath, VECSOURCES& VECSOURCES, bool& bIsSourceName);
  static std::string TranslateSpecialSource(const std::string &strSpecial);
  static void DeleteDirectoryCache(const std::string &prefix = "");
  static void DeleteMusicDatabaseDirectoryCache();
  static void DeleteVideoDatabaseDirectoryCache();
  static std::string MusicPlaylistsLocation();
  static std::string VideoPlaylistsLocation();

  static void GetSkinThemes(std::vector<std::string>& vecTheme);
  static void GetRecursiveListing(const std::string& strPath, CFileItemList& items, const std::string& strMask, unsigned int flags = 0 /* DIR_FLAG_DEFAULTS */);
  static void GetRecursiveDirsListing(const std::string& strPath, CFileItemList& items, unsigned int flags = 0 /* DIR_FLAG_DEFAULTS */);
  static void ForceForwardSlashes(std::string& strPath);

  static double AlbumRelevance(const std::string& strAlbumTemp1, const std::string& strAlbum1, const std::string& strArtistTemp1, const std::string& strArtist1);
  static bool MakeShortenPath(std::string StrInput, std::string& StrOutput, size_t iTextMaxLength);
  /*! \brief Checks wether the supplied path supports Write file operations (e.g. Rename, Delete, ...)

   \param strPath the path to be checked

   \return true if Write file operations are supported, false otherwise
   */
  static bool SupportsWriteFileOperations(const std::string& strPath);
  /*! \brief Checks wether the supplied path supports Read file operations (e.g. Copy, ...)

   \param strPath the path to be checked

   \return true if Read file operations are supported, false otherwise
   */
  static bool SupportsReadFileOperations(const std::string& strPath);
  static std::string GetDefaultFolderThumb(const std::string &folderThumb);

  static void InitRandomSeed();

  // Get decimal integer representation of roman digit, ivxlcdm are valid
  // return 0 for other chars;
  static int LookupRomanDigit(char roman_digit);
  // Translate a string of roman numerals to decimal a decimal integer
  // return -1 on error, valid range is 1-3999
  static int TranslateRomanNumeral(const char* roman_numeral);

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN_TVOS)
  //
  // Forks to execute a shell command.
  //
  static bool Command(const std::vector<std::string>& arrArgs, bool waitExit = false);

  //
  // Forks to execute an unparsed shell command line.
  //
  static bool RunCommandLine(const std::string& cmdLine, bool waitExit = false);
#endif
  static std::string ResolveExecutablePath();
  static std::string GetFrameworksPath(bool forPython = false);

  static bool CanBindPrivileged();
  static bool ValidatePort(int port);

  /*!
   * \brief Thread-safe random number generation
   */
  static int GetRandomNumber();

  static int64_t ConvertSecsToMilliSecs(double secs) { return static_cast<int64_t>(secs * 1000); }
  static double ConvertMilliSecsToSecs(int64_t offset) { return offset / 1000.0; }
  static int64_t ConvertMilliSecsToSecsInt(int64_t offset) { return offset / 1000; }
  static int64_t ConvertMilliSecsToSecsIntRounded(int64_t offset) { return ConvertMilliSecsToSecsInt(offset + 499); }

#if !defined(TARGET_WINDOWS)
private:
  static unsigned int s_randomSeed;
#endif

  protected:
    /** \brief Retrieves the base path and the filename of a given video.
    *   \param[in]  videoPath The full path of the video file.
    *   \param[out] basePath The base path of the given video.
    *   \param[out] videoFileName The file name of the given video..
    */
    static void GetVideoBasePathAndFileName(const std::string& videoPath,
                                            std::string& basePath,
                                            std::string& videoFileName);

    /** \brief Retrieves FileItems that could contain associated files of a given video.
    *   \param[in]  videoPath The full path of the video file.
    *   \param[in]  item_exts A | separated string of extensions specifying the associated files.
    *   \param[in]  sub_dirs A vector of sub directory names to look for.
    *   \param[out] items A List of FileItems to scan for associated files.
    */
    static void GetItemsToScan(const std::string& videoPath,
                               const std::string& item_exts,
                               const std::vector<std::string>& sub_dirs,
                               CFileItemList& items);

    /** \brief Searches for associated files of a given video.
    *   \param[in]  videoName The name of the video file.
    *   \param[in]  items A List of FileItems to scan for associated files.
    *   \param[in]  item_exts A vector of extensions specifying the associated files.
    *   \param[out] associatedFiles A vector containing the full paths of all found associated files.
    */
    static void ScanPathsForAssociatedItems(const std::string& videoName,
                                            const CFileItemList& items,
                                            const std::vector<std::string>& item_exts,
                                            std::vector<std::string>& associatedFiles);

    /** \brief Searches in an archive for associated files of a given video.
    *   \param[in]  strArchivePath The full path of the archive.
    *   \param[in]  videoNameNoExt The filename of the video without extension for which associated files should be retrieved.
    *   \param[in]  item_exts A vector of extensions specifying the associated files.
    *   \param[out] associatedFiles A vector containing the full paths of all found associated files.
    */
    static int ScanArchiveForAssociatedItems(const std::string& strArchivePath,
                                             const std::string& videoNameNoExt,
                                             const std::vector<std::string>& item_exts,
                                             std::vector<std::string>& associatedFiles);

};


