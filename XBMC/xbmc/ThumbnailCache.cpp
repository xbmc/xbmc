
#include "stdafx.h"
#include "ThumbnailCache.h"
#include "util.h"

CThumbnailCache* CThumbnailCache::m_pCacheInstance=NULL;

CCriticalSection CThumbnailCache::m_cs;

CThumbnailCache::~CThumbnailCache()
{
}

CThumbnailCache::CThumbnailCache()
{

}

CThumbnailCache* CThumbnailCache::GetThumbnailCache()
{
	CSingleLock lock(m_cs);

	if (m_pCacheInstance==NULL)
		m_pCacheInstance = new CThumbnailCache;

	return m_pCacheInstance;
}

bool CThumbnailCache::ThumbExists(const CStdString& strFileName, bool bAddCache/*=false*/)
{
	CSingleLock lock(m_cs);

	if (strFileName.size()==0) return false;
	map<CStdString, bool>::iterator it;
	it=m_Cache.find(strFileName);
	if (it!=m_Cache.end())
		return it->second;

	bool bExists=CUtil::FileExists(strFileName);

	if (bAddCache)
		Add(strFileName, bExists);
	return bExists;
}

bool CThumbnailCache::IsCached(const CStdString& strFileName)
{
	CSingleLock lock(m_cs);

	map<CStdString, bool>::iterator it;
	it=m_Cache.find(strFileName);
	if (it!=m_Cache.end())
		return true;

	return false;
}

void CThumbnailCache::Clear()
{
	CSingleLock lock(m_cs);

	if (m_pCacheInstance!=NULL)
	{
		m_Cache.erase(m_Cache.begin(),m_Cache.end());
		delete m_pCacheInstance;
	}
	
	m_pCacheInstance=NULL;
}

void CThumbnailCache::Add(const CStdString& strFileName, bool bExists)
{
	CSingleLock lock(m_cs);

	map<CStdString, bool>::iterator it;
	it=m_Cache.find(strFileName);
	if (it!=m_Cache.end())
	{
		it->second=bExists;
	}
	else
		m_Cache.insert(pair<CStdString, bool>(strFileName, bExists));
}
