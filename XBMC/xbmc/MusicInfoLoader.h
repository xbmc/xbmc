#include "utils/Thread.h"

#pragma once

namespace MUSIC_INFO
{
	class IMusicInfoLoaderObserver
	{
	public:
		virtual void OnItemLoaded(CFileItem* pItem)=0;
	};

	class CMusicInfoLoader : public CThread
	{
	public:
		CMusicInfoLoader();
		virtual ~CMusicInfoLoader();

						void		Load(VECFILEITEMS& items);
						bool		IsLoading();
		virtual void		OnStartup();
		virtual void		OnExit();
						void		UseCacheOnHD(const CStdString& strFileName);
		virtual void		Process();
						void		SetObserver(IMusicInfoLoaderObserver* pObserver);

	protected:
						void		LoadItem(CFileItem* pItem);
						void		LoadCache(const CStdString& strFileName, MAPFILEITEMS& items);
						void		SaveCache(const CStdString& strFileName, VECFILEITEMS& items);

	protected:
		VECFILEITEMS*								m_pVecItems;
		IMusicInfoLoaderObserver*		m_pObserver;
		CStdString									m_strCacheFileName;
		bool												m_bRunning;
		MAPFILEITEMS								m_mapFileItems;
		IMAPFILEITEMS								it;
		MAPSONGS										m_songsMap;
		CStdString									m_strPrevPath;
		CMusicDatabase							m_musicDatabase;
	};
};
