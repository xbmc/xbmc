#pragma once
#include "stdstring.h"
#include "fileitem.h"
#include <map>
struct network_info
{
	char ip[32];
	char gateway[32];
	char subnet[32];
	char DNS1[32];
	char DNS2[32];
	char DHCP;
};

using namespace std;
class CUtil
{
public:
  CUtil(void);
  virtual ~CUtil(void);
  static char* GetExtension(const CStdString& strFileName);
  static char* GetFileName(const CStdString& strFileNameAndPath);
	static bool  IsHD(const CStdString& strFileName);
  static bool  IsXBE(const CStdString& strFileName);
  static bool  IsDefaultXBE(const CStdString& strFileName);
  static bool  IsShortCut(const CStdString& strFileName);
  static int   cmpnocase(const char* str1,const char* str2);
  static bool  GetParentPath(const CStdString& strPath, CStdString& strParent);
  static void GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename);
  static void LaunchXbe(char* szPath, char* szXbe, char* szParameters);
  static bool FileExists(const CStdString& strFileName);
  static void GetThumbnail(const CStdString& strFileName, CStdString& strThumb);
  static void GetFileSize(__int64 dwFileSize, CStdString& strFileSize);
  static void GetDate(SYSTEMTIME stTime, CStdString& strDateTime);
	static void GetHomePath(CStdString& strPath);
  static bool InitializeNetwork(const char* szLocalAddress, const char* szLocalSubnet, const char* szLocalGateway, const char* szNameServer);
  static bool IsEthernetConnected();
	static void GetTitleIP(CStdString& ip);
  static void ConvertTimeTToFileTime(__int64 sec, long nsec, FILETIME &ftTime);
	static void ReplaceExtension(const CStdString& strFile, const CStdString& strNewExtension, CStdString& strChangedFile);
	static void GetExtension(const CStdString& strFile, CStdString& strExtension);
  static void Lower(CStdString& strText);
  static void Unicode2Ansi(const wstring& wstrText,CStdString& strName);
  static bool HasSlashAtEnd(const CStdString& strFile);
	static bool IsRemote(const CStdString& strFile);
	static bool IsDVD(const CStdString& strFile);
	static bool IsCDDA(const CStdString& strFile);
	static void GetFileAndProtocol(const CStdString& strURL, CStdString& strDir);
	static void RemoveCRLF(CStdString& strLine);
	static bool IsPicture(const CStdString& strLine) ;
	static bool IsAudio(const CStdString& strLine) ;
	static bool IsVideo(const CStdString& strLine) ;
	static bool IsDVDImage(const CStdString& strLine) ;
	static bool IsDVDFile(const CStdString& strFile, bool bVobs = true, bool bIfos = true);
	static bool IsRAR(const CStdString& strLine);
	static int GetDVDIfoTitle(const CStdString& strPathFile);
	static void URLEncode(CStdString& strURLData);
	static bool LoadString(string &strTxt, FILE *fd);
	static int  LoadString(CStdString &strTxt, byte* pBuffer);
	static void SaveString(const CStdString &strTxt, FILE *fd);
	static void	SaveInt(int iValue, FILE *fd);
	static int	LoadInt( FILE *fd);
	static void LoadDateTime(SYSTEMTIME& dateTime, FILE *fd);
	static void SaveDateTime(SYSTEMTIME& dateTime, FILE *fd);
	static void GetSongInfo(const CStdString& strFileName, CStdString& strSongCacheName);
	static void GetAlbumThumb(const CStdString& strFileName, CStdString& strAlbumThumb, bool bTempDir=false);
	static void GetAlbumInfo(const CStdString& strFileName, CStdString& strAlbumThumb);
	static void GetAlbumDatabase(const CStdString& strFileName, CStdString& strAlbumThumb);
	static bool GetXBEIcon(const CStdString& strFilePath, CStdString& strIcon);
	static bool GetXBEDescription(const CStdString& strFileName, CStdString& strDescription);
	static bool GetDirectoryName(const CStdString& strFileName, CStdString& strDescription);
	static DWORD GetXbeID( const CStdString& strFilePath);
	static void FillInDefaultIcons(VECFILEITEMS &items);
	static void FillInDefaultIcon(CFileItem* pItem);
	static void CreateShortcuts(VECFILEITEMS &items);
	static void CreateShortcut(CFileItem* pItem);
	static void SetThumbs(VECFILEITEMS &items);
	static void SetThumb(CFileItem* pItem);
	static void GetArtistDatabase(const CStdString& strFileName, CStdString& strArtistDBS);
	static void GetGenreDatabase(const CStdString& strFileName, CStdString& strGenreDBS);
	static bool IsPlayList(const CStdString& strFile) ;
	static void ShortenFileName(CStdString& strFileNameAndPath);
	static bool IsISO9660(const CStdString& strFile);
	static void ConvertPathToUrl( const CStdString& strPath, const CStdString& strProtocol, CStdString& strOutUrl );
	static void GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon );
	static void RemoveTempFiles();

	// Added by JM to determine possible type of file from framesize
	static bool IsNTSC_VCD(int iWidth, int iHeight);
	static bool IsNTSC_SVCD(int iWidth, int iHeight);
	static bool IsNTSC_DVD(int iWidth, int iHeight);
	static bool IsPAL_VCD(int iWidth, int iHeight);
	static bool IsPAL_SVCD(int iWidth, int iHeight);
	static bool IsPAL_DVD(int iWidth, int iHeight);
	static bool IsShoutCast(const CStdString& strFileName);
	static void RemoveIllegalChars( CStdString& strText);
	static void CacheSubtitles(const CStdString& strMovie);
	static void SecondsToHMSString( long lSeconds, CStdString& strHMS);
  static void PrepareSubtitleFonts();
  static __int64 ToInt64(DWORD dwHigh, DWORD dwLow);
  static void AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult);
  static bool IsNFO(const CStdString& strFile);
  static void GetPath(const CStdString& strFileName, CStdString& strPath);
	static void Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName);
	static int  GetFolderCount(VECFILEITEMS &items);
	static bool	ThumbExists(const CStdString& strFileName, bool bAddCache=false);
	static bool ThumbCached(const CStdString& strFileName);
	static void ThumbCacheAdd(const CStdString& strFileName, bool bFileExists);
	static void ThumbCacheClear();
  static void PlayDVD();
  static DWORD SetUpNetwork( bool resetmode, struct network_info& networkinfo );
  static bool  GetFolderThumb(const CStdString& strFolder, CStdString& strThumb);
  static void GetVideoThumbnail(const CStdString& strIMDBID, CStdString& strThumb);
  static void SetMusicThumbs(VECFILEITEMS &items);
  static void SetMusicThumb(CFileItem* pItem);
  static CStdString GetNextFilename(const char* fn_template, int max);
  static void TakeScreenshot();
	static void SetBrightnessContrastGamma(float Brightness, float Contrast, float Gamma, bool bImmediate);
  static void SetBrightnessContrastGammaPercent(int iBrightNess, int iContrast, int iGamma, bool bImmediate);
	static void FlashScreen(bool bImmediate, bool bOn);
  static void RestoreBrightnessContrastGamma();
  static void InitGamma();
  static void ClearCache();
};
