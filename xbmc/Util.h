#pragma once
#include <vector>
#include <math.h>
#include "PlayList.h"
#include "FileSystem/RarManager.h"
#include "Settings.h"
#ifdef HAS_XBOX_HARDWARE
#include "xbox/custom_launch_params.h"
#else
typedef void CUSTOM_LAUNCH_DATA;
#endif

#ifdef HAS_TRAINER
class CTrainer;
#endif

// for 'cherry' patching
typedef enum
{
  COUNTRY_NULL = 0,
  COUNTRY_USA,
  COUNTRY_JAP,
  COUNTRY_EUR
} F_COUNTRY;

typedef enum
{
  VIDEO_NULL = 0,
  VIDEO_NTSCM,
  VIDEO_NTSCJ,
  VIDEO_PAL50,
  VIDEO_PAL60
} F_VIDEO;

using namespace std;

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

#ifndef _LINUX
#define _P(x) x
#define PTH_IC(x) (x)
#define PATH_SEPARATOR_CHAR '\\'
#define PATH_SEPARATOR_STRING "\\"
#else
#define PTH_IC(x) CUtil::TranslatePathConvertCase(x)
#define _P(x) CUtil::TranslatePath(x)
#define PATH_SEPARATOR_CHAR '/'
#define PATH_SEPARATOR_STRING "/"
#endif

struct XBOXDETECTION
{
  std::vector<CStdString> client_ip;
  std::vector<CStdString> client_info;
  std::vector<unsigned int> client_lookup_count;
  std::vector<bool> client_informed;
};

namespace MathUtils
{

#ifndef _LINUX
  inline int round_int (double x)
  {
    assert (x > static_cast <double>(INT_MIN / 2) - 1.0);
    assert (x < static_cast <double>(INT_MAX / 2) + 1.0);

    const float round_to_nearest = 0.5f;
    int i;
    __asm
    {
      fld x
      fadd st, st (0)
      fadd round_to_nearest
      fistp i
      sar i, 1
    }
    return (i);
  }
#else
  inline int round_int (float x) { return lrintf(x); }
  inline int round_int (double x) { return lrint(x); }
#endif

  inline int ceil_int (double x)
  {
    assert (x > static_cast <double>(INT_MIN / 2) - 1.0);
    assert (x < static_cast <double>(INT_MAX / 2) + 1.0);
  #ifndef _LINUX
    const float round_towards_p_i = -0.5f;
    int i;
    __asm
    {
      fld x
      fadd st, st (0)
      fsubr round_towards_p_i
      fistp i
      sar i, 1
    }
    return (-i);
  #else
    return (int)(ceil(x));
  #endif
  }

  inline int truncate_int(double x)
  {
    assert (x > static_cast <double>(INT_MIN / 2) - 1.0);
    assert (x < static_cast <double>(INT_MAX / 2) + 1.0);
  #ifndef _LINUX
    const float round_towards_m_i = -0.5f;
    int i;
    __asm
    {
      fld x
      fadd st, st (0)
      fabs
      fadd round_towards_m_i
      fistp i
      sar i, 1
    }
    if (x < 0)
      i = -i;
    return (i);
  #else
    return (int)(x);
  #endif
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
  static void CleanFileName(CStdString& strFileName);
  static const CStdString GetFileName(const CStdString& strFileNameAndPath);  
  static CStdString GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder = false);
  static bool IsHD(const CStdString& strFileName);
  static bool IsBuiltIn(const CStdString& execString);
  static void GetBuiltInHelp(CStdString &help);
  static int ExecBuiltIn(const CStdString& execString);
  static bool GetParentPath(const CStdString& strPath, CStdString& strParent);
  static void GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename);
#ifdef HAS_TRAINER
  static bool InstallTrainer(CTrainer& trainer);
  static bool RemoveTrainer();
#endif
  static bool PatchCountryVideo(F_COUNTRY Country, F_VIDEO Video);
  static void RunShortcut(const char* szPath);
  static void RunXBE(const char* szPath, char* szParameters = NULL, F_VIDEO ForceVideo=VIDEO_NULL, F_COUNTRY ForceCountry=COUNTRY_NULL, CUSTOM_LAUNCH_DATA* pData=NULL);
  static void LaunchXbe(const char* szPath, const char* szXbe, const char* szParameters, F_VIDEO ForceVideo=VIDEO_NULL, F_COUNTRY ForceCountry=COUNTRY_NULL, CUSTOM_LAUNCH_DATA* pData=NULL); 
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
  static bool IsCDDA(const CStdString& strFile);
  static bool IsMemCard(const CStdString& strFile);
  static bool IsTuxBox(const CStdString& strFile);
  static void GetFileAndProtocol(const CStdString& strURL, CStdString& strDir);
  static int GetDVDIfoTitle(const CStdString& strPathFile);
  static void UrlDecode(CStdString& strURLData);
  static void URLEncode(CStdString& strURLData);
  static bool CacheXBEIcon(const CStdString& strFilePath, const CStdString& strIcon);
  static bool GetXBEDescription(const CStdString& strFileName, CStdString& strDescription);
  static bool SetXBEDescription(const CStdString& strFileName, const CStdString& strDescription);
  static DWORD GetXbeID( const CStdString& strFilePath);
  static bool GetDirectoryName(const CStdString& strFileName, CStdString& strDescription);
  static void CreateShortcuts(CFileItemList &items);
  static void CreateShortcut(CFileItem* pItem);
  static void GetArtistDatabase(const CStdString& strFileName, CStdString& strArtistDBS);
  static void GetGenreDatabase(const CStdString& strFileName, CStdString& strGenreDBS);
  static void GetFatXQualifiedPath(CStdString& strFileNameAndPath);
  static void ShortenFileName(CStdString& strFileNameAndPath);
  static bool IsISO9660(const CStdString& strFile);
  static bool IsSmb(const CStdString& strFile);
  static bool IsDAAP(const CStdString& strFile);
  static bool IsUPnP(const CStdString& strFile);
  static void ConvertPathToUrl( const CStdString& strPath, const CStdString& strProtocol, CStdString& strOutUrl );
  static void GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon );
  static void RemoveTempFiles();
  static void DeleteGUISettings();

  static void RemoveIllegalChars( CStdString& strText);
  static void CacheSubtitles(const CStdString& strMovie, CStdString& strExtensionCached, XFILE::IFileCallback *pCallback = NULL);
  static bool CacheRarSubtitles(std::vector<CStdString>& vecExtensionsCached, const CStdString& strRarPath, const CStdString& strCompare, const CStdString& strExtExt="");
  static void ClearSubtitles();
  static void PrepareSubtitleFonts();
  static __int64 ToInt64(DWORD dwHigh, DWORD dwLow);
  static void AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult);
  static void AddSlashAtEnd(CStdString& strFolder);
  static void RemoveSlashAtEnd(CStdString& strFolder);  
  static void Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
  static void CreateZipPath(CStdString& strUrlPath, const CStdString& strRarPath, 
    const CStdString& strFilePathInRar,  const WORD wOptions = EXFILE_AUTODELETE , 
    const CStdString& strPwd = RAR_DEFAULT_PASSWORD, const CStdString& strCachePath = RAR_DEFAULT_CACHE);
  static void CreateRarPath(CStdString& strUrlPath, const CStdString& strRarPath, 
    const CStdString& strFilePathInRar,  const WORD wOptions = EXFILE_AUTODELETE , 
    const CStdString& strPwd = RAR_DEFAULT_PASSWORD, const CStdString& strCachePath = RAR_DEFAULT_CACHE);
  static bool ThumbExists(const CStdString& strFileName, bool bAddCache = false);
  static bool ThumbCached(const CStdString& strFileName);
  static void ThumbCacheAdd(const CStdString& strFileName, bool bFileExists);
  static void ThumbCacheClear();
  static void PlayDVD();
  static CStdString GetNextFilename(const char* fn_template, int max);
  static void TakeScreenshot();
  static void TakeScreenshot(const char* fn, bool flash);
  static void SetBrightnessContrastGamma(float Brightness, float Contrast, float Gamma, bool bImmediate);
  static void SetBrightnessContrastGammaPercent(int iBrightNess, int iContrast, int iGamma, bool bImmediate);
  static void Tokenize(const CStdString& path, vector<CStdString>& tokens, const string& delimiters);
  static void FlashScreen(bool bImmediate, bool bOn);
  static void RestoreBrightnessContrastGamma();
  static void InitGamma();
  static void ClearCache();
  static void StatToStatI64(struct _stati64 *result, struct stat *stat);
  static void Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat);
  static void StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat);
#ifndef _LINUX
  static void Stat64ToStat(struct _stat *result, struct __stat64 *stat);
#else
  static void Stat64ToStat(struct stat *result, struct __stat64 *stat);
#endif
  static bool CreateDirectoryEx(const CStdString& strPath);
  static CStdString MakeLegalFileName(const CStdString &strFile, bool isFATX);
  static void ConvertFileItemToPlayListItem(const CFileItem *pItem, PLAYLIST::CPlayList::CPlayListItem &playlistitem);
  static void AddDirectorySeperator(CStdString& strPath);
  static char GetDirectorySeperator(const CStdString& strFile);

  static bool IsUsingTTFSubtitles();
  static void SplitExecFunction(const CStdString &execString, CStdString &strFunction, CStdString &strParam);
  static int GetMatchingShare(const CStdString& strPath, VECSHARES& vecShares, bool& bIsSourceName);
  static CStdString TranslateSpecialPath(const CStdString &strSpecial);
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
  static bool SetXBOXNickName(CStdString strXboxNickNameIn, CStdString &strXboxNickNameOut);
  static bool GetXBOXNickName(CStdString &strXboxNickNameOut);
  static bool AutoDetectionPing(CStdString strFTPUserName, CStdString strFTPPass, CStdString strNickName, int iFTPPort);
  static bool AutoDetection();
  static void AutoDetectionGetShare(VECSHARES &share);
  static void GetSkinThemes(std::vector<CStdString>& vecTheme);
  static void GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask, bool bUseFileDirectories=false);
  static void GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& items);
  static void WipeDir(const CStdString& strPath);
  static void ForceForwardSlashes(CStdString& strPath);
  static bool PWMControl(const CStdString &strRGBa, const CStdString &strRGBb, const CStdString &strWhiteA, const CStdString &strWhiteB, const CStdString &strTransition, int iTrTime);
  static bool RunFFPatchedXBE(CStdString szPath1, CStdString& szNewPath);
  static void RemoveKernelPatch();
  static bool LookForKernelPatch();

  static double AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1);
  static bool MakeShortenPath(CStdString StrInput, CStdString& StrOutput, int iTextMaxLength);
  static float CurrentCpuUsage();
  static bool SupportsFileOperations(const CStdString& strPath);

  static CStdString GetCachedMusicThumb(const CStdString &path);
  static CStdString GetCachedAlbumThumb(const CStdString &album, const CStdString &artist);
  static void ClearFileItemCache();

  static void BootToDash();

  static CStdString TranslatePath(const CStdString& path);
  static CStdString TranslatePathConvertCase(const CStdString& path);
  
private:
  
  static HANDLE m_hCurrentCpuUsage;
};

//static int iAdditionalChecked = -1;

