#pragma once
#include <xtl.h>
#include "stdstring.h"
using namespace std;

class CUtil
{
public:
  CUtil(void);
  static char* GetExtension(const CStdString& strFileName);
  static char* GetFileName(const CStdString& strFileNameAndPath);
  static bool  IsXBE(const CStdString& strFileName);
  static bool  IsShortCut(const CStdString& strFileName);
  static int   cmpnocase(const char* str1,const char* str2);
  static bool  GetParentPath(const CStdString& strPath, CStdString& strParent);
  static void LaunchXbe(char* szPath, char* szXbe, char* szParameters);
  static bool FileExists(const CStdString& strFileName);
  static void GetThumbnail(const CStdString& strFileName, CStdString& strThumb);
  static void GetFileSize(DWORD dwFileSize, CStdString& strFileSize);
  static void GetDate(SYSTEMTIME stTime, CStdString& strDateTime);
	static void GetHomePath(CStdString& strPath);
  static bool InitializeNetwork(const char* szLocalAdres, const char* szLocalSubnet, const char* szLocalGateway);
  static bool IsEthernetConnected();
  static void ConvertTimeTToFileTime(__int64 sec, long nsec, FILETIME &ftTime);
	static void ReplaceExtension(const CStdString& strFile, const CStdString& strNewExtension, CStdString& strChangedFile);
	static void GetExtension(const CStdString& strFile, CStdString& strExtension);
  static void Lower(CStdString& strText);
  static void Unicode2Ansi(const wstring& wstrText,CStdString& strName);
  static bool HasSlashAtEnd(const CStdString& strFile);
	static bool IsRemote(const CStdString& strFile);
	static bool IsDVD(const CStdString& strFile);
	static void GetFileAndProtocol(const CStdString& strURL, CStdString& strDir);
	static void RemoveCRLF(CStdString& strLine);
	static bool IsPicture(const CStdString& strLine) ;
	static bool IsAudio(const CStdString& strLine) ;
	static bool IsVideo(const CStdString& strLine) ;
	static void URLEncode(CStdString& strURLData);
	static bool LoadString(CStdString &strTxt, FILE *fd);
	static void SaveString(const CStdString &strTxt, FILE *fd);
	static void	SaveInt(int iValue, FILE *fd);
	static int	LoadInt( FILE *fd);
	static void LoadDateTime(SYSTEMTIME& dateTime, FILE *fd);
	static void SaveDateTime(SYSTEMTIME& dateTime, FILE *fd);
	static void GetSongCacheName(const CStdString& strFileName, CStdString& strSongCacheName);
	static void GetAlbumThumb(const CStdString& strFileName, CStdString& strAlbumThumb);
	static void GetAlbumCacheName(const CStdString& strFileName, CStdString& strAlbumThumb);
	static bool GetXBEIcon(const CStdString& strFilePath, CStdString& strIcon);
	static bool GetXBEDescription(const CStdString& strFileName, CStdString& strDescription);
	static DWORD GetXbeID( const CStdString& strFilePath);

  virtual ~CUtil(void);
};
