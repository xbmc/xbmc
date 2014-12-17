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
#include "utils/StringUtils.h"
#include "MediaSource.h"

#define ARRAY_SIZE(X)         (sizeof(X)/sizeof((X)[0]))

// A list of filesystem types for LegalPath/FileName
#define LEGAL_NONE            0
#define LEGAL_WIN32_COMPAT    1
#define LEGAL_FATX            2

class CFileItemList;
class CURL;

struct sortstringbyname
{
  bool operator()(const CStdString& strItem1, const CStdString& strItem2)
  {
    CStdString strLine1 = strItem1;
    CStdString strLine2 = strItem2;
    StringUtils::ToLower(strLine1);
    StringUtils::ToLower(strLine2);
    return strcmp(strLine1.c_str(), strLine2.c_str()) < 0;
  }
};

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
  static CStdString GetTitleFromPath(const CURL& url, bool bIsFolder = false);
  static CStdString GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder = false);
  static void GetQualifiedFilename(const std::string &strBasePath, std::string &strFilename);
  static void RunShortcut(const char* szPath);
  static void GetHomePath(std::string& strPath, const std::string& strTarget = ""); // default target is "KODI_HOME"
  static bool IsPVR(const CStdString& strFile);
  static bool IsHTSP(const CStdString& strFile);
  static bool IsLiveTV(const CStdString& strFile);
  static bool IsTVRecording(const CStdString& strFile);
  static bool ExcludeFileOrFolder(const CStdString& strFileOrFolder, const std::vector<std::string>& regexps);
  static void GetFileAndProtocol(const CStdString& strURL, CStdString& strDir);
  static int GetDVDIfoTitle(const CStdString& strPathFile);

  static bool IsPicture(const CStdString& strFile);

  /*! \brief retrieve MD5sum of a file
   \param strPath - path to the file to MD5sum
   \return md5 sum of the file
   */
  static CStdString GetFileMD5(const CStdString& strPath);
  static bool GetDirectoryName(const CStdString& strFileName, CStdString& strDescription);
  static void GetDVDDriveIcon(const std::string& strPath, std::string& strIcon);
  static void RemoveTempFiles();
  static void ClearTempFonts();

  static void ClearSubtitles();
  static void ScanForExternalSubtitles(const std::string& strMovie, std::vector<std::string>& vecSubtitles );
  static int ScanArchiveForSubtitles( const std::string& strArchivePath, const std::string& strMovieFileNameNoExt, std::vector<std::string>& vecSubtitles );
  static void GetExternalStreamDetailsFromFilename(const CStdString& strMovie, const CStdString& strSubtitles, ExternalStreamInfo& info); 
  static bool FindVobSubPair( const std::vector<std::string>& vecSubtitles, const std::string& strIdxPath, std::string& strSubPath );
  static bool IsVobSub(const std::vector<std::string>& vecSubtitles, const std::string& strSubPath);
  static std::string GetVobSubSubFromIdx(const std::string& vobSubIdx);
  static std::string GetVobSubIdxFromSub(const std::string& vobSub);
  static int64_t ToInt64(uint32_t high, uint32_t low);
  static CStdString GetNextFilename(const CStdString &fn_template, int max);
  static CStdString GetNextPathname(const CStdString &path_template, int max);
  static void StatToStatI64(struct _stati64 *result, struct stat *stat);
  static void StatToStat64(struct __stat64 *result, const struct stat *stat);
  static void Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat);
  static void StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat);
  static void Stat64ToStat(struct stat *result, struct __stat64 *stat);
#ifdef TARGET_WINDOWS
  static void Stat64ToStat64i32(struct _stat64i32 *result, struct __stat64 *stat);
#endif
  static bool CreateDirectoryEx(const CStdString& strPath);

#ifdef TARGET_WINDOWS
  static CStdString MakeLegalFileName(const CStdString &strFile, int LegalType=LEGAL_WIN32_COMPAT);
  static CStdString MakeLegalPath(const CStdString &strPath, int LegalType=LEGAL_WIN32_COMPAT);
#else
  static CStdString MakeLegalFileName(const CStdString &strFile, int LegalType=LEGAL_NONE);
  static CStdString MakeLegalPath(const CStdString &strPath, int LegalType=LEGAL_NONE);
#endif
  static CStdString ValidatePath(const CStdString &path, bool bFixDoubleSlashes = false); ///< return a validated path, with correct directory separators.
  
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
  static void SplitParams(const CStdString &paramString, std::vector<std::string> &parameters);
  static void SplitExecFunction(const std::string &execString, std::string &function, std::vector<std::string> &parameters);
  static int GetMatchingSource(const CStdString& strPath, VECSOURCES& VECSOURCES, bool& bIsSourceName);
  static CStdString TranslateSpecialSource(const CStdString &strSpecial);
  static void DeleteDirectoryCache(const CStdString &prefix = "");
  static void DeleteMusicDatabaseDirectoryCache();
  static void DeleteVideoDatabaseDirectoryCache();
  static CStdString MusicPlaylistsLocation();
  static CStdString VideoPlaylistsLocation();

  static void GetSkinThemes(std::vector<std::string>& vecTheme);
  static void GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask, unsigned int flags = 0 /* DIR_FLAG_DEFAULTS */);
  static void GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& items, unsigned int flags = 0 /* DIR_FLAG_DEFAULTS */);
  static void ForceForwardSlashes(CStdString& strPath);

  static double AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1);
  static bool MakeShortenPath(std::string StrInput, std::string& StrOutput, size_t iTextMaxLength);
  /*! \brief Checks wether the supplied path supports Write file operations (e.g. Rename, Delete, ...)

   \param strPath the path to be checked

   \return true if Write file operations are supported, false otherwise
   */
  static bool SupportsWriteFileOperations(const CStdString& strPath);
  /*! \brief Checks wether the supplied path supports Read file operations (e.g. Copy, ...)

   \param strPath the path to be checked

   \return true if Read file operations are supported, false otherwise
   */
  static bool SupportsReadFileOperations(const CStdString& strPath);
  static CStdString GetDefaultFolderThumb(const CStdString &folderThumb);

#ifdef UNIT_TESTING
  static bool TestSplitExec();
  static bool TestGetQualifiedFilename();
  static bool TestMakeLegalPath();
#endif

  static void InitRandomSeed();

  // Get decimal integer representation of roman digit, ivxlcdm are valid
  // return 0 for other chars;
  static int LookupRomanDigit(char roman_digit);
  // Translate a string of roman numerals to decimal a decimal integer
  // return -1 on error, valid range is 1-3999
  static int TranslateRomanNumeral(const char* roman_numeral);

#ifdef TARGET_POSIX
  // this will run the command using sudo in a new process.
  // the user that runs xbmc should be allowed to issue the given sudo command.
  // in order to allow a user to run sudo without supplying the password you'll need to edit sudoers
  // # sudo visudo
  // and add a line at the end defining the user and allowed commands
  static bool SudoCommand(const CStdString &strCommand);

  //
  // Forks to execute a shell command.
  //
  static bool Command(const std::vector<std::string>& arrArgs, bool waitExit = false);

  //
  // Forks to execute an unparsed shell command line.
  //
  static bool RunCommandLine(const CStdString& cmdLine, bool waitExit = false);
#endif
  static CStdString ResolveExecutablePath();
  static CStdString GetFrameworksPath(bool forPython = false);

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


