#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include <climits>
#include <cmath>
#include <vector>
#include <string.h>
#include <stdint.h>
#include "MediaSource.h" // Definition of VECSOURCES

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
  unsigned int flag;

  ExternalStreamInfo() : flag(0){};
};

class CUtil
{
public:
  CUtil(void);
  virtual ~CUtil(void);
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
  static void GetHomePath(std::string& strPath, const std::string& strTarget = ""); // default target is "KODI_HOME"
  static bool IsPVR(const std::string& strFile);
  static bool IsHTSP(const std::string& strFile);
  static bool IsLiveTV(const std::string& strFile);
  static bool IsTVRecording(const std::string& strFile);
  static bool ExcludeFileOrFolder(const std::string& strFileOrFolder, const std::vector<std::string>& regexps);
  static void GetFileAndProtocol(const std::string& strURL, std::string& strDir);
  static int GetDVDIfoTitle(const std::string& strPathFile);

  static bool IsPicture(const std::string& strFile);

  /*! \brief retrieve MD5sum of a file
   \param strPath - path to the file to MD5sum
   \return md5 sum of the file
   */
  static std::string GetFileMD5(const std::string& strPath);
  static bool GetDirectoryName(const std::string& strFileName, std::string& strDescription);
  static void GetDVDDriveIcon(const std::string& strPath, std::string& strIcon);
  static void RemoveTempFiles();
  static void ClearTempFonts();

  static void ClearSubtitles();
  static void ScanForExternalSubtitles(const std::string& strMovie, std::vector<std::string>& vecSubtitles );
  static int ScanArchiveForSubtitles( const std::string& strArchivePath, const std::string& strMovieFileNameNoExt, std::vector<std::string>& vecSubtitles );
  static void GetExternalStreamDetailsFromFilename(const std::string& strMovie, const std::string& strSubtitles, ExternalStreamInfo& info); 
  static bool FindVobSubPair( const std::vector<std::string>& vecSubtitles, const std::string& strIdxPath, std::string& strSubPath );
  static bool IsVobSub(const std::vector<std::string>& vecSubtitles, const std::string& strSubPath);
  static std::string GetVobSubSubFromIdx(const std::string& vobSubIdx);
  static std::string GetVobSubIdxFromSub(const std::string& vobSub);
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

#ifdef TARGET_POSIX
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

#if !defined(TARGET_WINDOWS)
private:
  static unsigned int s_randomSeed;
#endif
};


