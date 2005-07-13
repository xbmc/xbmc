#include "../stdafx.h"
#include "FileRar.h"
#include <sys/stat.h>
#include "../util.h"

using namespace XFILE;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//*********************************************************************************************
CFileRar::CFileRar()
{
	m_strCacheDir.Empty();
	m_strRarPath.Empty();
	m_strPassword.Empty();
	m_strPathInRar.Empty();
	m_bRarOptions = 0;
	m_bFileOptions = 0;
}

//*********************************************************************************************
CFileRar::~CFileRar()
{
  g_RarManager.ClearCachedFile(m_strRarPath,m_strPathInRar); 
}
//*********************************************************************************************
bool CFileRar::Open(const CURL& url, bool bBinary)
{
  InitFromUrl( url );

	CStdString strPathInCache;

  if( !g_RarManager.CacheRarredFile(strPathInCache, m_strRarPath, m_strPathInRar,  m_bFileOptions,  m_strCacheDir) ) 
		return false;
  if( !m_File.Open( strPathInCache, bBinary) )  return false;
  return true;
}
bool CFileRar::Exists(const CURL& url)
{
	InitFromUrl( url );
	CStdString strPathInCache;
	bool bResult;
	if ( !g_RarManager.IsFileInRar(bResult, m_strRarPath, m_strPathInRar) ) return false;
	return bResult;
}
//*********************************************************************************************
int CFileRar::Stat(const CURL& url, struct __stat64* buffer)
{
	InitFromUrl( url );
	CStdString strPathInCache;
	if( !g_RarManager.CacheRarredFile(strPathInCache, m_strRarPath, m_strPathInRar,  m_bFileOptions,  m_strCacheDir) ) 
		return false;
	CFile file;
	return file.Stat(strPathInCache, buffer);
}


//*********************************************************************************************
bool CFileRar::OpenForWrite(const CURL& url, bool bBinary)
{
	InitFromUrl( url );
	CStdString strPathInCache;
	if( !g_RarManager.CacheRarredFile(strPathInCache, m_strRarPath, m_strPathInRar,  m_bFileOptions,  m_strCacheDir) ) 
		return false;
	if(!m_File.OpenForWrite(strPathInCache, bBinary))  return false;
	return true;
}

//*********************************************************************************************
unsigned int CFileRar::Read(void *lpBuf, __int64 uiBufSize)
{
  return m_File.Read(lpBuf, uiBufSize);
}

//*********************************************************************************************
unsigned int CFileRar::Write(void *lpBuf, __int64 uiBufSize)
{
	return m_File.Write(lpBuf, uiBufSize);
}

//*********************************************************************************************
void CFileRar::Close()
{
  m_File.Close();
}

//*********************************************************************************************
__int64 CFileRar::Seek(__int64 iFilePosition, int iWhence)
{
  return m_File.Seek(iFilePosition, iWhence);
}

//*********************************************************************************************
__int64 CFileRar::GetLength()
{
	return m_File.GetLength();
}

//*********************************************************************************************
__int64 CFileRar::GetPosition()
{
	return m_File.GetPosition();
}


//*********************************************************************************************
bool CFileRar::ReadString(char *szLine, int iLineLength)
{
	return m_File.ReadString(szLine,  iLineLength);
}


int CFileRar::Write(const void* lpBuf, __int64 uiBufSize)
{
	return m_File.Write(lpBuf, uiBufSize);
}

bool CFileRar::Delete(const char* strFileName)
{
	//Deletes the cached rar and/or file according to the autodel values. 
	CURL url(strFileName);
	InitFromUrl( url );
	bool bAutoDelFile = (m_bFileOptions & EXFILE_AUTODELETE) !=0;

	CStdString strFileCachedPath, strRarCachedPath;
	g_RarManager.MakeCachedPath(strFileCachedPath, m_strCacheDir, m_strPathInRar);
	g_RarManager.MakeCachedPath(strRarCachedPath, m_strCacheDir, m_strRarPath);
	CFile file;
	bool bRes = true;
	if(bAutoDelFile)
		if( !file.Delete(strFileCachedPath) )	bRes = false;

	return bRes;
}

bool CFileRar::Rename(const char* strFileName, const char* strNewFileName)
{
	return false;
}

void CFileRar::Flush()
{
	m_File.Flush();
}
void CFileRar::InitFromUrl(const CURL& url)
{
  CStdString strURL;
  url.GetURL(strURL);
	m_strCacheDir = url.GetDomain();
	m_strRarPath = url.GetHostName();
	m_strPassword = url.GetPassWord();
	m_strPathInRar = url.GetFileName();
	int iAutoDelMask = url.GetPort();

	m_bRarOptions = 0;

	bool bAutoDelFile = (iAutoDelMask & EXFILE_AUTODELETE) !=0;
	bool bOverwriteFile = (iAutoDelMask & EXFILE_OVERWRITE) !=0;
	m_bFileOptions = 0;
}