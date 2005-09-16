#pragma once
#include "playlist.h"
#include "filesystem/rarmanager.h"

//#define SKIN_VERSION_1_3 1

struct network_info
{
  char ip[32];
  char gateway[32];
  char subnet[32];
  char DNS1[32];
  char DNS2[32];
  char DHCP;
};

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

static BYTE OriginalData[57]=
{
  0x55,0x8B,0xEC,0x81,0xEC,0x04,0x01,0x00,0x00,0x8B,0x45,0x08,0x3D,0x04,0x01,0x00,
  0x00,0x53,0x75,0x32,0x8B,0x4D,0x18,0x85,0xC9,0x6A,0x04,0x58,0x74,0x02,0x89,0x01,
  0x39,0x45,0x14,0x73,0x0A,0xB8,0x23,0x00,0x00,0xC0,0xE9,0x59,0x01,0x00,0x00,0x8B,
  0x4D,0x0C,0x89,0x01,0x8B,0x45,0x10,0x8B,0x0D
};

static BYTE PatchData[70]=
{
  0x55,0x8B,0xEC,0xB9,0x04,0x01,0x00,0x00,0x2B,0xE1,0x8B,0x45,0x08,0x53,0x3B,0xC1,
  0x74,0x0C,0x49,0x3B,0xC1,0x75,0x2F,0xB8,0x00,0x03,0x80,0x00,0xEB,0x05,0xB8,0x04,
  0x00,0x00,0x00,0x50,0x8B,0x4D,0x18,0x6A,0x04,0x58,0x85,0xC9,0x74,0x02,0x89,0x01,
  0x8B,0x4D,0x0C,0x89,0x01,0x59,0x8B,0x45,0x10,0x89,0x08,0x33,0xC0,0x5B,0xC9,0xC2,
  0x14,0x00,0x00,0x00,0x00,0x00
};

extern "C"	int __stdcall MmQueryAddressProtect(void *Adr);
extern "C"	void __stdcall MmSetAddressProtect(void *Adr, int Size, int Type);  

using namespace std;
using namespace PLAYLIST;

class CUtil
{
public:
  CUtil(void);
  virtual ~CUtil(void);
  static char* GetExtension(const CStdString& strFileName);
  static void RemoveExtension(CStdString& strFileName);
  static bool GetVolumeFromFileName(const CStdString& strFileName, CStdString& strFileTitle, CStdString& strVolumeNumber);
  static void CleanFileName(CStdString& strFileName);
  static char* GetFileName(const CStdString& strFileNameAndPath);
  static CStdString GetTitleFromPath(const CStdString& strFileNameAndPath);
  static bool IsHD(const CStdString& strFileName);
  static bool IsBuiltIn(const CStdString& execString);
  static void GetBuiltInHelp(CStdString &help);
  static int ExecBuiltIn(const CStdString& execString);
  static int cmpnocase(const char* str1, const char* str2);
  static bool GetParentPath(const CStdString& strPath, CStdString& strParent);
  static void GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename);
  static bool PatchCountryVideo(F_COUNTRY Country, F_VIDEO Video);
  static void RunXBE(const char* szPath, char* szParameters = NULL, F_VIDEO ForceVideo=VIDEO_NULL, F_COUNTRY ForceCountry=COUNTRY_NULL);
  static void LaunchXbe(const char* szPath, const char* szXbe, const char* szParameters, F_VIDEO ForceVideo=VIDEO_NULL, F_COUNTRY ForceCountry=COUNTRY_NULL); 
  static void GetDirectory(const CStdString& strFilePath, CStdString& strDirectoryPath);
  static void GetThumbnail(const CStdString& strFileName, CStdString& strThumb);
  static void GetCachedThumbnail(const CStdString& strFileName, CStdString& strCachedThumb);
  static void GetDate(SYSTEMTIME stTime, CStdString& strDateTime);
  static void GetHomePath(CStdString& strPath);
  static bool InitializeNetwork(int iAssignment, const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway, const char* szNameServer);
  static bool IsEthernetConnected();
  static void GetTitleIP(CStdString& ip);
  static void ConvertTimeTToFileTime(__int64 sec, long nsec, FILETIME &ftTime);
  static __int64 CompareSystemTime(const SYSTEMTIME *a, const SYSTEMTIME *b);
  static void ReplaceExtension(const CStdString& strFile, const CStdString& strNewExtension, CStdString& strChangedFile);
  static void GetExtension(const CStdString& strFile, CStdString& strExtension);
  static void Lower(CStdString& strText);
  static void Unicode2Ansi(const wstring& wstrText, CStdString& strName);
  static bool HasSlashAtEnd(const CStdString& strFile);
  static bool IsRemote(const CStdString& strFile);
  static bool IsDVD(const CStdString& strFile);
  static bool IsRAR(const CStdString& strFile);
  static bool IsZIP(const CStdString& strFile);
  static bool IsCDDA(const CStdString& strFile);
  static void GetFileAndProtocol(const CStdString& strURL, CStdString& strDir);
  static void RemoveCRLF(CStdString& strLine);
  static int GetDVDIfoTitle(const CStdString& strPathFile);
  static void URLEncode(CStdString& strURLData);
  static bool LoadString(string &strTxt, FILE *fd);
  static int LoadString(CStdString &strTxt, byte* pBuffer);
  static void SaveString(const CStdString &strTxt, FILE *fd);
  static void SaveInt(int iValue, FILE *fd);
  static int LoadInt( FILE *fd);
  static void LoadDateTime(SYSTEMTIME& dateTime, FILE *fd);
  static void SaveDateTime(SYSTEMTIME& dateTime, FILE *fd);
  static void GetSongInfo(const CStdString& strFileName, CStdString& strSongCacheName);
  static void GetAlbumFolderThumb(const CStdString& strFileName, CStdString& strAlbumThumb, bool bTempDir = false);
  static void GetAlbumThumb(const CStdString& strAlbumName, const CStdString& strFileName, CStdString& strThumb, bool bTempDir = false);
  static void GetAlbumInfo(const CStdString& strFileName, CStdString& strAlbumThumb);
  static void GetAlbumDatabase(const CStdString& strFileName, CStdString& strAlbumThumb);
  static bool GetXBEIcon(const CStdString& strFilePath, CStdString& strIcon);
  static bool GetXBEDescription(const CStdString& strFileName, CStdString& strDescription);
  static bool GetDirectoryName(const CStdString& strFileName, CStdString& strDescription);
  static DWORD GetXbeID( const CStdString& strFilePath);
  static void CreateShortcuts(CFileItemList &items);
  static void CreateShortcut(CFileItem* pItem);
  static void GetArtistDatabase(const CStdString& strFileName, CStdString& strArtistDBS);
  static void GetGenreDatabase(const CStdString& strFileName, CStdString& strGenreDBS);
  static void GetFatXQualifiedPath(CStdString& strFileNameAndPath);
  static void ShortenFileName(CStdString& strFileNameAndPath);
  static bool IsISO9660(const CStdString& strFile);
  static bool IsSmb(const CStdString& strFile);
  static void ConvertPathToUrl( const CStdString& strPath, const CStdString& strProtocol, CStdString& strOutUrl );
  static void GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon );
  static void RemoveTempFiles();
  static void DeleteTDATA();
  static bool GetFolderThumb(const CStdString& strFolder, CStdString& strThumb);

  static void RemoveIllegalChars( CStdString& strText);
  static void CacheSubtitles(const CStdString& strMovie, CStdString& strExtensionCached, XFILE::IFileCallback *pCallback = NULL);
  static bool CacheRarSubtitles(std::vector<CStdString>& vecExtensionsCached, const CStdString& strRarPath, const char * const* pSubExts  );
  static void ClearSubtitles();
  static void SecondsToHMSString( long lSeconds, CStdString& strHMS, bool bMustUseHHMMSS = false);
  static void PrepareSubtitleFonts();
  static __int64 ToInt64(DWORD dwHigh, DWORD dwLow);
  static void AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult);
  static void GetPath(const CStdString& strFileName, CStdString& strPath);
  static void Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
static void CreateRarPath(CStdString& strUrlPath, const CStdString& strRarPath, 
    const CStdString& strFilePathInRar,  const WORD wOptions = EXFILE_AUTODELETE , 
    const CStdString& strPwd = RAR_DEFAULT_PASSWORD, const CStdString& strCachePath = RAR_DEFAULT_CACHE);
  static bool ThumbExists(const CStdString& strFileName, bool bAddCache = false);
  static bool ThumbCached(const CStdString& strFileName);
  static void ThumbCacheAdd(const CStdString& strFileName, bool bFileExists);
  static void ThumbCacheClear();
  static void PlayDVD();
  static DWORD SetUpNetwork( bool resetmode, struct network_info& networkinfo );
  static void GetVideoThumbnail(const CStdString& strIMDBID, CStdString& strThumb);
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
  static void SortFileItemsByName(CFileItemList& items, bool bSortAscending = true);
  static bool IsNetworkUp() { return m_bNetworkUp; }
  static void Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat);
  static void StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat);
  static void Stat64ToStat(struct _stat *result, struct __stat64 *stat);
  static bool CreateDirectoryEx(const CStdString& strPath);
  static CStdString MakeLegalFileName(const char* strFile, bool bKeepExtension, bool isFATX = true);
  static void ConvertFileItemToPlayListItem(const CFileItem *pItem, CPlayList::CPlayListItem &playlistitem);
  static void AddDirectorySeperator(CStdString& strPath);
  static char GetDirectorySeperator(const CStdString& strFile);

  static bool IsNaturalNumber(const CStdString& str);
  static bool IsNaturalNumber(const CStdStringW& str);
  static bool IsUsingTTFSubtitles();
  static void SplitExecFunction(const CStdString &execString, CStdString &strFunction, CStdString &strParam);
  static int GetMatchingShare(const CStdString& strPath, VECSHARES& vecShares, bool& bIsBookmarkName);
  static CStdString TranslateSpecialDir(const CStdString &strSpecial);
  static void DeleteDatabaseDirectoryCache();

  //GeminiServer
  static bool IsLeapYear(int iLYear, int iLMonth, int iLTag, int &iMonMax, int &iWeekDay);
  static bool SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute);
  static bool XboxAutoDetectionPing(bool bRefresh, CStdString strFTPUserName, CStdString strFTPPass, CStdString strNickName, int iFTPPort, CStdString &strHasClientIP, CStdString &strHasClientInfo, CStdString &strNewClientIP, CStdString &strNewClientInfo );
  static bool XboxAutoDetection();
  static bool IsFTP(const CStdString& strFile);
  static bool CmpNoCase(const char* str1, const char* str2);
  static bool GetFTPServerUserName(int iFTPUserID, CStdString &strFtpUser1, int &iUserMax );
  static bool SetFTPServerUserPassword(CStdString strFtpUserName, CStdString strFtpUserPassword);
  static bool SetXBOXNickName(CStdString strXboxNickNameIn, CStdString &strXboxNickNameOut);
  static void GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask);
  static void GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& item);

private:
  static bool m_bNetworkUp;
};
