#include "StdString.h"
#include <map>
#include "utils/CriticalSection.h"
#pragma once

class CThumbnailCache
{
private:
	CThumbnailCache();
public:
	virtual ~CThumbnailCache();

	static CThumbnailCache* GetThumbnailCache();
	bool ThumbExists(const CStdString& strFileName, bool bAddCache=false);
	void Add(const CStdString& strFileName, bool bExists);
	void Clear();
	bool IsCached(const CStdString& strFileName);
protected:

	static CThumbnailCache* m_pCacheInstance;

	map<CStdString, bool> m_Cache;

	static CCriticalSection m_cs;
};
