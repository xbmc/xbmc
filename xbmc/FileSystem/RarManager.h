#if !defined(AFX_RARMANAGER_H__06BA7C2E_3FCA_11D9_8186_0050FC718317__INCLUDED_)
#define AFX_RARMANAGER_H__06BA7C2E_3FCA_11D9_8186_0050FC718317__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "File.h"
#include "../fileitem.h"
#include "../utils/criticalsection.h"
#include <map>

#define EXFILE_OVERWRITE 1
#define EXFILE_AUTODELETE 2
#define RAR_DEFAULT_CACHE "Z:\\"
#define RAR_DEFAULT_PASSWORD ""

class CFileInfo{
public:
	CFileInfo();
	~CFileInfo();
	CStdString m_strCachedPath;
	bool	m_bAutoDel;
};

class CRarManager
{
public:
	CRarManager();
	~CRarManager();
	bool CacheRarredFile(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar, BYTE bOptions = EXFILE_AUTODELETE, const CStdString& strDir =RAR_DEFAULT_CACHE );
	bool GetPathInCache(CStdString& strPathInCache, const CStdString& strRarPath, const CStdString& strPathInRar = "");
	bool GetFilesInRar(CFileItemList& vecpItems, const CStdString& strRarPath, bool bMask=true, const CStdString& strPathInRar="");
	bool IsFileInRar(bool& bResult, const CStdString& strRarPath, const CStdString& strPathInRar);
	void MakeCachedPath(CStdString& strCachedPath, const CStdString& strDir, const CStdString& strFilePath);
	void ClearCache();
  void ClearCachedFile(const CStdString& strRarPath, const CStdString& strPathInRar);
  void ExtractArchive(const CStdString& strArchive, const CStdString& strPath);
protected:
  std::map<CStdString, CFileInfo> m_ExFiles;
	CCriticalSection m_CritSection;
};

  extern CRarManager g_RarManager;
#endif