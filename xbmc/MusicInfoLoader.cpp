#include "stdafx.h"
#include "MusicInfoLoader.h"
#include "musicdatabase.h"
#include "musicInfoTagLoaderFactory.h"
#include "Filesystem/DirectoryCache.h"
#include "Util.h"

using namespace MUSIC_INFO;
using namespace DIRECTORY;

CMusicInfoLoader::CMusicInfoLoader()
{
	m_bRunning=false;
}

CMusicInfoLoader::~CMusicInfoLoader()
{

}

void CMusicInfoLoader::OnStartup()
{
	m_bRunning=true;
}

void CMusicInfoLoader::Process()
{
	try
	{
		VECFILEITEMS& vecItems=(*m_pVecItems);

		if (vecItems.size()<=0)
			return;

		//	Load previously cached items from HD
		if (!m_strCacheFileName.IsEmpty())
			LoadCache(m_strCacheFileName, m_mapFileItems);

		//	Precache album thumbs
		g_directoryCache.InitMusicThumbCache();

		m_musicDatabase.Open();

		for (int i=0; i<(int)vecItems.size(); ++i)
		{
			CFileItem* pItem=vecItems[i];

			if (m_bStop)
				break;

			if (pItem->m_bIsFolder || pItem->IsPlayList() || pItem->IsNFO() || pItem->IsInternetStream())
				continue;

			//	Fill in tag for the item
			LoadItem(pItem);

			//	Notify observer a item 
			//	is loaded.
			if (m_pObserver)
			{
				g_graphicsContext.Lock();
				m_pObserver->OnItemLoaded(pItem);
				g_graphicsContext.Unlock();
			}
		}

		//	clear precached album thumbs
		g_directoryCache.ClearMusicThumbCache();

		//	cleanup last loaded songs from database
		m_songsMap.erase(m_songsMap.begin(),m_songsMap.end());

		//	cleanup cache loaded from HD
		it=m_mapFileItems.begin();
		while(it!=m_mapFileItems.end())
		{
			delete it->second;
			it++;
		}
		m_mapFileItems.erase(m_mapFileItems.begin(), m_mapFileItems.end());

		//	Save loaded items to HD
		if (!m_strCacheFileName.IsEmpty())
			SaveCache(m_strCacheFileName, vecItems);

		m_musicDatabase.Close();
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "tagloaderthread: Unhandled exception");
	}
}

void CMusicInfoLoader::LoadItem(CFileItem* pItem)
{
	if (pItem->m_musicInfoTag.Loaded())
		return;

	CStdString strFileName, strPath;
	CUtil::GetDirectory(pItem->m_strPath, strPath);

	//	First query cached items
	it=m_mapFileItems.find(pItem->m_strPath);
	if (it!=m_mapFileItems.end()&& it->second->m_musicInfoTag.Loaded() && CUtil::CompareSystemTime(&it->second->m_stTime, &pItem->m_stTime)==0)
	{
		pItem->m_musicInfoTag=it->second->m_musicInfoTag;
	}
	else
	{
		//	Have we loaded this item from database before
		IMAPSONGS it=m_songsMap.find(pItem->m_strPath);
		if (it!=m_songsMap.end())
		{
			CSong& song=it->second;
			pItem->m_musicInfoTag.SetSong(song);
		}
		else if (strPath!=m_strPrevPath)
		{
			//	The item is from another directory as the last one,
			//	query the database for the new directory...
			m_musicDatabase.GetSongsByPath(strPath, m_songsMap);

			//	...and look if we find it
			IMAPSONGS it=m_songsMap.find(pItem->m_strPath);
			if (it!=m_songsMap.end())
			{
				CSong& song=it->second;
				pItem->m_musicInfoTag.SetSong(song);
			}
		}

		//	Nothing found, load tag from file
		if (g_guiSettings.GetBool("MyMusic.UseTags") && !pItem->m_musicInfoTag.Loaded())
		{
			// get correct tag parser
			CMusicInfoTagLoaderFactory factory;
			auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
			if (NULL != pLoader.get())
					// get id3tag
				pLoader->Load(pItem->m_strPath,pItem->m_musicInfoTag);
		}
	}

	m_strPrevPath=strPath;
}

void CMusicInfoLoader::OnExit()
{
	m_bRunning=false;
}

void CMusicInfoLoader::Load(VECFILEITEMS& items)
{
	m_pVecItems=&items;
	StopThread();
	Create();
}

bool CMusicInfoLoader::IsLoading()
{
	return m_bRunning;
}

void CMusicInfoLoader::SetObserver(IMusicInfoLoaderObserver* pObserver)
{
	m_pObserver=pObserver;
}

void CMusicInfoLoader::UseCacheOnHD(const CStdString& strFileName)
{
	m_strCacheFileName=strFileName;
}

void CMusicInfoLoader::LoadCache(const CStdString& strFileName, MAPFILEITEMS& items)
{
	CFile file;

	if (file.Open(strFileName))
	{
		CArchive ar(&file, CArchive::load);
		int iSize=0;
		ar >> iSize;
		for (int i=0; i<iSize; i++)
		{
			CFileItem* pItem=new CFileItem();
			ar >> *pItem;
			items.insert(MAPFILEITEMSPAIR(pItem->m_strPath, pItem));
		}
		ar.Close();
		file.Close();
	}
}

void CMusicInfoLoader::SaveCache(const CStdString& strFileName, VECFILEITEMS& items)
{
	int iSize=items.size();

	if (iSize<=0)
		return;

	CFile file;

	if (file.OpenForWrite(strFileName))
	{
		CArchive ar(&file, CArchive::store);
		ar << (int)items.size();
		for (int i=0; i<iSize; i++)
		{
			CFileItem* pItem=items[i];
			ar << *pItem;
		}
		ar.Close();
		file.Close();
	}

}
