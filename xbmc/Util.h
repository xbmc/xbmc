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
#include "MediaSource.h"
#include "StringUtils.h"

namespace XFILE
{
  class IFileCallback;
}

class CFileItem;
class CFileItemList;

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

struct XBOXDETECTION
{
  std::vector<CStdString> client_ip;
  std::vector<CStdString> client_info;
  std::vector<unsigned int> client_lookup_count;
  std::vector<bool> client_informed;
};

namespace MathUtils
{
  // GCC does something stupid with optimization on release builds if we try
  // to assert in these functions
  inline int round_int (double x)
  {
    assert(x > static_cast<double>(INT_MIN / 2) - 1.0);
    assert(x < static_cast <double>(INT_MAX / 2) + 1.0);
    const float round_to_nearest = 0.5f;
    int i;

#ifndef _LINUX
    __asm
    {
      fld x
      fadd st, st (0)
      fadd round_to_nearest
      fistp i
      sar i, 1
    }
#else
    #if defined(__APPLE__) || defined(__powerpc__)
        i = floor(x + round_to_nearest);
    #else
        __asm__ __volatile__ (
            "fadd %%st\n\t"
            "fadd %%st(1)\n\t"
            "fistpl %0\n\t"
            "sarl $1, %0\n"
            : "=m"(i) : "u"(round_to_nearest), "t"(x) : "st"
        );
    #endif
#endif
    return (i);
  }

  inline int ceil_int (double x)
  {
    assert(x > static_cast<double>(INT_MIN / 2) - 1.0);
    assert(x < static_cast <double>(INT_MAX / 2) + 1.0);

    #ifndef __APPLE__
        const float round_towards_p_i = -0.5f;
    #endif
    int i;

#ifndef _LINUX
    __asm
    {
      fld x
      fadd st, st (0)
      fsubr round_towards_p_i
      fistp i
      sar i, 1
    }
#else
    #ifdef __APPLE__
        i = ceil(x);
    #else
        __asm__ __volatile__ (
            "fadd %%st\n\t"
            "fsubr %%st(1)\n\t"
            "fistpl %0\n\t"
            "sarl $1, %0\n"
            : "=m"(i) : "u"(round_towards_p_i), "t"(x) : "st"
        );
    #endif
#endif
    return (-i);
  }

  inline int truncate_int(double x)
  {
    assert(x > static_cast<double>(INT_MIN / 2) - 1.0);
    assert(x < static_cast <double>(INT_MAX / 2) + 1.0);

    #ifndef __APPLE__
        const float round_towards_m_i = -0.5f;
    #endif
    int i;

#ifndef _LINUX
    __asm
    {
      fld x
      fadd st, st (0)
      fabs
      fadd round_towards_m_i
      fistp i
      sar i, 1
    }
#else
    #ifdef __APPLE__
        i = (int)x;
    #else
        __asm__ __volatile__ (
            "fadd %%st\n\t"
            "fabs\n\t"
            "fadd %%st(1)\n\t"
            "fistpl %0\n\t"
            "sarl $1, %0\n"
            : "=m"(i) : "u"(round_towards_m_i), "t"(x) : "st"
        );
    #endif
#endif
    if (x < 0)
      i = -i;
    return (i);
  }

  inline void hack()
  {
    // stupid hack to keep compiler from dropping these
    // functions as unused
    MathUtils::round_int(0.0);
    MathUtils::truncate_int(0.0);
    MathUtils::ceil_int(0.0);
  }
} // namespace MathUtils

class CUtil
{
public:
  CUtil(void);
  virtual ~CUtil(void);
  static const CStdString GetExtension(const CStdString& strFileName);
  static void RemoveExtension(CStdString& strFileName);
  static bool GetVolumeFromFileName(const CStdString& strFileName, CStdString& strFileTitle, CStdString& strVolumeNumber);
  static void CleanString(CStdString& strFileName, bool bIsFolder = false);
  static const CStdString GetFileName(const CStdString& strFileNameAndPath);
  static CStdString GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder = false);
  static void GetCommonPath(CStdString& strPath, const CStdString& strPath2);
  static bool IsDOSPath(const CStdString &path);
  static bool IsHD(const CStdString& strFileName);
  static bool IsBuiltIn(const CStdString& execString);
  static void GetBuiltInHelp(CStdString &help);
  static int ExecBuiltIn(const CStdString& execString);
  static bool GetParentPath(const CStdString& strPath, CStdString& strParent);
  static const CStdString  GetMovieName(CFileItem* pItem, bool bUseFolderNames = false);
  static void GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename);
  static void RunShortcut(const char* szPath);
  static void GetDirectory(const CStdString& strFilePath, CStdString& strDirectoryPath);
  static void GetHomePath(CStdString& strPath);
  static void ReplaceExtension(const CStdString& strFile, const CStdString& strNewExtension, CStdString& strChangedFile);
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
  static bool IsCDDA(const CStdString& strFile);
  static bool IsMemCard(const CStdString& strFile);
  static bool IsTuxBox(const CStdString& strFile);
  static bool IsMythTV(const CStdString& strFile);
  static bool IsVTP(const CStdString& strFile);
  static bool IsTV(const CStdString& strFile);
  static bool ExcludeFileOrFolder(const CStdString& strFileOrFolder, const CStdStringArray& regexps);
  static void GetFileAndProtocol(const CStdString& strURL, CStdString& strDir);
  static int GetDVDIfoTitle(const CStdString& strPathFile);
  static void UrlDecode(CStdString& strURLData);
  static void URLEncode(CStdString& strURLData);
  static bool GetDirectoryName(const CStdString& strFileName, CStdString& strDescription);
  static void CreateShortcuts(CFileItemList &items);
  static void CreateShortcut(CFileItem* pItem);
  static void GetArtistDatabase(const CStdString& strFileName, CStdString& strArtistDBS);
  static void GetGenreDatabase(const CStdString& strFileName, CStdString& strGenreDBS);
  static bool IsISO9660(const CStdString& strFile);
  static bool IsSmb(const CStdString& strFile);
  static bool IsDAAP(const CStdString& strFile);
  static bool IsUPnP(const CStdString& strFile);
  static void ConvertPathToUrl( const CStdString& strPath, const CStdString& strProtocol, CStdString& strOutUrl );
  static void GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon );
  static void RemoveTempFiles();
  static void DeleteGUISettings();

  static void CacheSubtitles(const CStdString& strMovie, CStdString& strExtensionCached, XFILE::IFileCallback *pCallback = NULL);
  static bool CacheRarSubtitles(std::vector<CStdString>& vecExtensionsCached, const CStdString& strRarPath, const CStdString& strCompare, const CStdString& strExtExt="");
  static void ClearSubtitles();
  static void PrepareSubtitleFonts();
  static __int64 ToInt64(DWORD dwHigh, DWORD dwLow);
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
  static void PlayDVD();
  static CStdString GetNextFilename(const CStdString &fn_template, int max);
  static void TakeScreenshot();
  static void TakeScreenshot(const char* fn, bool flash);
  static void SetBrightnessContrastGamma(float Brightness, float Contrast, float Gamma, bool bImmediate);
  static void SetBrightnessContrastGammaPercent(int iBrightNess, int iContrast, int iGamma, bool bImmediate);
  static void Tokenize(const CStdString& path, std::vector<CStdString>& tokens, const std::string& delimiters);
  static void FlashScreen(bool bImmediate, bool bOn);
  static void RestoreBrightnessContrastGamma();
  static void InitGamma();
  static void ClearCache();
  static void StatToStatI64(struct _stati64 *result, struct stat *stat);
  static void Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat);
  static void StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat);
  static void Stat64ToStat(struct stat *result, struct __stat64 *stat);
  static bool CreateDirectoryEx(const CStdString& strPath);
  static CStdString MakeLegalFileName(const CStdString &strFile);
  static CStdString MakeLegalPath(const CStdString &strPath);
  static void AddDirectorySeperator(CStdString& strPath);
  static char GetDirectorySeperator(const CStdString& strFile);

  static bool IsUsingTTFSubtitles();
  static void SplitExecFunction(const CStdString &execString, CStdString &strFunction, CStdString &strParam);
  static int GetMatchingSource(const CStdString& strPath, VECSOURCES& VECSOURCES, bool& bIsSourceName);
  static CStdString TranslateSpecialSource(const CStdString &strSpecial);
  static void DeleteDirectoryCache(const CStdString strType = "");
  static void DeleteMusicDatabaseDirectoryCache();
  static void DeleteVideoDatabaseDirectoryCache();
  static CStdString MusicPlaylistsLocation();
  static CStdString VideoPlaylistsLocation();
  static CStdString SubstitutePath(const CStdString& strFileName);

  static bool SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute);
  static int GMTZoneCalc(int iRescBiases, int iHour, int iMinute, int &iMinuteNew);
  static bool IsFTP(const CStdString& strFile);
  static bool GetFTPServerUserName(int iFTPUserID, CStdString &strFtpUser1, int &iUserMax );
  static bool SetFTPServerUserPassword(CStdString strFtpUserName, CStdString strFtpUserPassword);
  static bool AutoDetectionPing(CStdString strFTPUserName, CStdString strFTPPass, CStdString strNickName, int iFTPPort);
  static bool AutoDetection();
  static void AutoDetectionGetSource(VECSOURCES &share);
  static void GetSkinThemes(std::vector<CStdString>& vecTheme);
  static void GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask, bool bUseFileDirectories=false);
  static void GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& items);
  static void WipeDir(const CStdString& strPath);
  static void ForceForwardSlashes(CStdString& strPath);

  static double AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1);
  static bool MakeShortenPath(CStdString StrInput, CStdString& StrOutput, int iTextMaxLength);
  static bool SupportsFileOperations(const CStdString& strPath);

  static CStdString GetCachedMusicThumb(const CStdString &path);
  static CStdString GetCachedAlbumThumb(const CStdString &album, const CStdString &artist);
  static CStdString GetDefaultFolderThumb(const CStdString &folderThumb);
  static void ClearFileItemCache();

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
  static bool Command(const CStdStringArray& arrArgs);
#endif

private:

  static HANDLE m_hCurrentCpuUsage;
};


