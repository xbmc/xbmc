#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <climits>
#include <cmath>
#include <vector>
#include <limits>
#include <string.h>
#include <stdint.h>

#include "MediaSource.h"

// A list of filesystem types for LegalPath/FileName
#define LEGAL_NONE            0
#define LEGAL_WIN32_COMPAT    1
#define LEGAL_FATX            2

namespace XFILE
{
  class IFileCallback;
}

class CFileItem;
class CFileItemList;
class CURL;

struct sortstringbyname
{
  bool operator()(const CStdString& strItem1, const CStdString& strItem2)
  {
    CStdString strLine1 = strItem1;
    CStdString strLine2 = strItem2;
    strLine1 = strLine1.ToLower();
    strLine2 = strLine2.ToLower();
    return strcmp(strLine1.c_str(), strLine2.c_str()) < 0;
  }
};

class CUtil
{
public:
  CUtil(void);
  virtual ~CUtil(void);
  static const CStdString GetExtension(const CStdString& strFileName);
  static void RemoveExtension(CStdString& strFileName);
  static bool GetVolumeFromFileName(const CStdString& strFileName, CStdString& strFileTitle, CStdString& strVolumeNumber);
  static void CleanString(const CStdString& strFileName, CStdString& strTitle, CStdString& strTitleAndYear, CStdString& strYear, bool bRemoveExtension = false, bool bCleanChars = true);
  static const CStdString GetFileName(const CStdString& strFileNameAndPath);
  static CStdString GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder = false);
  static void GetCommonPath(CStdString& strPath, const CStdString& strPath2);
  static bool IsDOSPath(const CStdString &path);
  static bool IsHD(const CStdString& strFileName);
  static CStdString GetParentPath(const CStdString& strPath);
  static bool GetParentPath(const CStdString& strPath, CStdString& strParent);
  static void GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename);
  static void RunShortcut(const char* szPath);
  static void GetDirectory(const CStdString& strFilePath, CStdString& strDirectoryPath);
  static void GetHomePath(CStdString& strPath, const CStdString& strTarget = "XBMC_HOME");
  static CStdString ReplaceExtension(const CStdString& strFile, const CStdString& strNewExtension);
  static void GetExtension(const CStdString& strFile, CStdString& strExtension);
  static bool HasSlashAtEnd(const CStdString& strFile);
  static bool IsRemote(const CStdString& strFile);
  static bool IsOnDVD(const CStdString& strFile);
  static bool IsOnLAN(const CStdString& strFile);
  static bool IsDVD(const CStdString& strFile);
  static bool IsVirtualPath(const CStdString& strFile);
  static bool IsMultiPath(const CStdString& strPath);
  static bool IsStack(const CStdString& strFile);
  static bool IsRAR(const CStdString& strFile);
  static bool IsInRAR(const CStdString& strFile);
  static bool IsZIP(const CStdString& strFile);
  static bool IsInZIP(const CStdString& strFile);
  static bool IsInArchive(const CStdString& strFile);
  static bool IsSpecial(const CStdString& strFile);
  static bool IsPlugin(const CStdString& strFile);
  static bool IsAddonsPath(const CStdString& strFile);
  static bool IsCDDA(const CStdString& strFile);
  static bool IsTuxBox(const CStdString& strFile);
  static bool IsMythTV(const CStdString& strFile);
  static bool IsHDHomeRun(const CStdString& strFile);
  static bool IsVTP(const CStdString& strFile);
  static bool IsHTSP(const CStdString& strFile);
  static bool IsLiveTV(const CStdString& strFile);
  static bool IsMusicDb(const CStdString& strFile);
  static bool IsVideoDb(const CStdString& strFile);
  static bool IsLastFM(const CStdString& strFile);
  static bool ExcludeFileOrFolder(const CStdString& strFileOrFolder, const CStdStringArray& regexps);
  static void GetFileAndProtocol(const CStdString& strURL, CStdString& strDir);
  static int GetDVDIfoTitle(const CStdString& strPathFile);
  static void URLDecode(CStdString& strURLData);
  static void URLEncode(CStdString& strURLData);

  /*! \brief retrieve MD5sum of a file
   \param strPath - path to the file to MD5sum
   \return md5 sum of the file
   */
  static CStdString GetFileMD5(const CStdString& strPath);
  static bool GetDirectoryName(const CStdString& strFileName, CStdString& strDescription);
  static bool IsISO9660(const CStdString& strFile);
  static bool IsSmb(const CStdString& strFile);
  static bool IsXBMS(const CStdString& strFile);
  static bool IsURL(const CStdString& strFile);
  static bool IsFTP(const CStdString& strFile);
  static bool IsInternetStream(const CURL& url, bool bStrictCheck = false);
  static bool IsDAAP(const CStdString& strFile);
  static bool IsUPnP(const CStdString& strFile);
  static bool IsWritable(const CStdString& strFile);
  static bool IsPicture(const CStdString& strFile);
  static void GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon );
  static void RemoveTempFiles();

  static void ClearSubtitles();
  static void ScanForExternalSubtitles(const CStdString& strMovie, std::vector<CStdString>& vecSubtitles );
  static int ScanArchiveForSubtitles( const CStdString& strArchivePath, const CStdString& strMovieFileNameNoExt, std::vector<CStdString>& vecSubtitles );
  static bool FindVobSubPair( const std::vector<CStdString>& vecSubtitles, const CStdString& strIdxPath, CStdString& strSubPath );
  static bool IsVobSub( const std::vector<CStdString>& vecSubtitles, const CStdString& strSubPath );  
  static int64_t ToInt64(uint32_t high, uint32_t low);
  static void AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult);
  static CStdString AddFileToFolder(const CStdString &strFolder, const CStdString &strFile)
  {
    CStdString result;
    AddFileToFolder(strFolder, strFile, result);
    return result;
  }
  static void AddSlashAtEnd(CStdString& strFolder);
  static void RemoveSlashAtEnd(CStdString& strFolder);
  static void Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  static void CreateArchivePath(CStdString& strUrlPath, const CStdString& strType, const CStdString& strArchivePath,
  const CStdString& strFilePathInArchive, const CStdString& strPwd="");
  static bool ThumbExists(const CStdString& strFileName, bool bAddCache = false);
  static bool ThumbCached(const CStdString& strFileName);
  static void ThumbCacheAdd(const CStdString& strFileName, bool bFileExists);
  static void ThumbCacheClear();
  static void PlayDVD(const CStdString& strProtocol="dvd");
  static CStdString GetNextFilename(const CStdString &fn_template, int max);
  static CStdString GetNextPathname(const CStdString &path_template, int max);
  static void TakeScreenshot();
  static void TakeScreenshot(const CStdString &filename, bool sync);
  static void Tokenize(const CStdString& path, std::vector<CStdString>& tokens, const std::string& delimiters);
  static void StatToStatI64(struct _stati64 *result, struct stat *stat);
  static void Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat);
  static void StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat);
  static void Stat64ToStat(struct stat *result, struct __stat64 *stat);
#ifdef _WIN32
  static void Stat64ToStat64i32(struct _stat64i32 *result, struct __stat64 *stat);
#endif
  static bool CreateDirectoryEx(const CStdString& strPath);

#ifdef _WIN32
  static CStdString MakeLegalFileName(const CStdString &strFile, int LegalType=LEGAL_WIN32_COMPAT);
  static CStdString MakeLegalPath(const CStdString &strPath, int LegalType=LEGAL_WIN32_COMPAT);
#else
  static CStdString MakeLegalFileName(const CStdString &strFile, int LegalType=LEGAL_NONE);
  static CStdString MakeLegalPath(const CStdString &strPath, int LegalType=LEGAL_NONE);
#endif
  static CStdString ValidatePath(const CStdString &path, bool bFixDoubleSlashes = false); ///< return a validated path, with correct directory separators.
  
  static bool IsUsingTTFSubtitles();
  static void SplitExecFunction(const CStdString &execString, CStdString &function, std::vector<CStdString> &parameters);
  static int GetMatchingSource(const CStdString& strPath, VECSOURCES& VECSOURCES, bool& bIsSourceName);
  static CStdString TranslateSpecialSource(const CStdString &strSpecial);
  static void DeleteDirectoryCache(const CStdString &prefix = "");
  static void DeleteMusicDatabaseDirectoryCache();
  static void DeleteVideoDatabaseDirectoryCache();
  static CStdString MusicPlaylistsLocation();
  static CStdString VideoPlaylistsLocation();
  static CStdString SubstitutePath(const CStdString& strFileName);

  static bool SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute);
  static int GMTZoneCalc(int iRescBiases, int iHour, int iMinute, int &iMinuteNew);
  static void GetSkinThemes(std::vector<CStdString>& vecTheme);
  static void GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask, bool bUseFileDirectories=false);
  static void GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& items);
  static void WipeDir(const CStdString& strPath);
  static void CopyDirRecursive(const CStdString& strSrcPath, const CStdString& strDstPath);
  static void ForceForwardSlashes(CStdString& strPath);

  static double AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1);
  static bool MakeShortenPath(CStdString StrInput, CStdString& StrOutput, int iTextMaxLength);
  static bool SupportsFileOperations(const CStdString& strPath);

  static CStdString GetCachedMusicThumb(const CStdString &path);
  static CStdString GetCachedAlbumThumb(const CStdString &album, const CStdString &artist);
  static CStdString GetDefaultFolderThumb(const CStdString &folderThumb);

#ifdef UNIT_TESTING
  static bool TestSplitExec();
#endif

  static void InitRandomSeed();

  // Get decimal integer representation of roman digit, ivxlcdm are valid
  // return 0 for other chars;
  static int LookupRomanDigit(char roman_digit);
  // Translate a string of roman numerals to decimal a decimal integer
  // return -1 on error, valid range is 1-3999
  static int TranslateRomanNumeral(const char* roman_numeral);

#ifdef _LINUX
  // this will run the command using sudo in a new process.
  // the user that runs xbmc should be allowed to issue the given sudo command.
  // in order to allow a user to run sudo without supplying the password you'll need to edit sudoers
  // # sudo visudo
  // and add a line at the end defining the user and allowed commands
  static bool SudoCommand(const CStdString &strCommand);

  //
  // Forks to execute a shell command.
  //
  static bool Command(const CStdStringArray& arrArgs, bool waitExit = false);

  //
  // Forks to execute an unparsed shell command line.
  //
  static bool RunCommandLine(const CStdString& cmdLine, bool waitExit = false);
#endif
  static CStdString ResolveExecutablePath();
};


