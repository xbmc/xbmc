#include "stdafx.h"
#include "MusicInfoScanner.h"
#include "musicdatabase.h"
#include "musicInfoTagLoaderFactory.h"
#include "Filesystem/cddadirectory.h"
#include "Filesystem/hddirectory.h"
#include "Util.h"
#include "settings.h"
#include "utils/Log.h"
#include "GUISettings.h"
#include "SectionLoader.h"
#include "Application.h"

using namespace MUSIC_INFO;
using namespace DIRECTORY;

CMusicInfoScanner::CMusicInfoScanner()
{
	m_bRunning=false;
}

CMusicInfoScanner::~CMusicInfoScanner()
{

}

void CMusicInfoScanner::OnStartup()
{
	m_bRunning=true;
}

void CMusicInfoScanner::Process()
{
	try
	{
		DWORD dwTick=timeGetTime();

		m_musicDatabase.Open();

		if (m_pObserver)
			m_pObserver->OnStateChanged(PREPARING);

		// check whether we have scanned here before
		CStdString strPaths;
		strPaths = m_musicDatabase.GetSubpathsFromPath(m_strStartDir);

		// Preload section for ID3 cover art reading
		CSectionLoader::Load("CXIMAGE");
		CSectionLoader::Load("LIBMP4");

		CUtil::ThumbCacheClear();

		m_musicDatabase.BeginTransaction();

		bool bOKtoScan = true;
		if (m_bUpdateAll)
		{
			if (m_pObserver)
				m_pObserver->OnStateChanged(REMOVING_OLD);

			//m_musicDatabase.BeginTransaction();
			bOKtoScan = m_musicDatabase.RemoveSongsFromPaths(strPaths);
			//m_musicDatabase.CommitTransaction();
		}

		if (bOKtoScan)
		{
			if (m_bUpdateAll)
			{
				if (m_pObserver)
					m_pObserver->OnStateChanged(CLEANING_UP_DATABASE);

				//m_musicDatabase.BeginTransaction();
				bOKtoScan = m_musicDatabase.CleanupAlbumsArtistsGenres(strPaths);
				//m_musicDatabase.CommitTransaction();
			}

			if (m_pObserver)
				m_pObserver->OnStateChanged(READING_MUSIC_INFO);

			bool bCommit = false;
			if (bOKtoScan)
				bCommit = DoScan(m_strStartDir);

			if (bCommit)
			{
				m_musicDatabase.CommitTransaction();

				if (m_bUpdateAll)
				{
					if (m_pObserver)
						m_pObserver->OnStateChanged(COMPRESSING_DATABASE);

					//m_musicDatabase.BeginTransaction();
					m_musicDatabase.Compress();
					//m_musicDatabase.CommitTransaction();
				}
			}

		}

		m_musicDatabase.EmptyCache();

		CSectionLoader::Unload("CXIMAGE");
		CSectionLoader::Unload("LIBMP4");

		CUtil::ThumbCacheClear();

		m_musicDatabase.Close();

		dwTick = timeGetTime() - dwTick;
		CStdString strTmp, strTmp1;
		CUtil::SecondsToHMSString(dwTick/1000, strTmp1);
		strTmp.Format("My Music: Scanning for music info using worker thread, operation took %s", strTmp1); 
		CLog::Log(LOGNOTICE,strTmp.c_str());

		if (m_pObserver)
			m_pObserver->OnFinished();
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "MusicInfoScanner: Exception will scanning.");
	}
}

void CMusicInfoScanner::OnExit()
{
	m_bRunning=false;
}

void CMusicInfoScanner::Start(const CStdString& strDirectory, bool bUpdateAll)
{
	m_strStartDir=strDirectory;
	m_bUpdateAll=bUpdateAll;
	StopThread();
	Create();
}

bool CMusicInfoScanner::IsScanning()
{
	return m_bRunning;
}

void  CMusicInfoScanner::Stop()
{
	StopThread();
}

void CMusicInfoScanner::SetObserver(IMusicInfoScannerObserver* pObserver)
{
	m_pObserver=pObserver;
}

bool CMusicInfoScanner::DoScan(const CStdString& strDirectory)
{
	if (m_pObserver)
		m_pObserver->OnDirectoryChanged(strDirectory);

	// load subfolder
	VECFILEITEMS items;
	CFileItemList itemlist(items);
	CDirectory dir;
	dir.GetDirectory(strDirectory,items);
	CUtil::SortFileItemsByName(items);
	// filter items in the sub dir (for .cue sheet support)
	g_application.m_guiMyMusicSongs.FilterItems(items);

	if (RetrieveMusicInfo(items, strDirectory)>0)
	{
		//m_musicDatabase.BeginTransaction();
		m_musicDatabase.CheckVariousArtistsAndCoverArt();
		//m_musicDatabase.CommitTransaction();
	}

	if (CUtil::GetFolderCount(items)!=items.size())
	{
		if (m_pObserver)
			m_pObserver->OnDirectoryScanned(strDirectory);
	}

	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem *pItem= items[i];

		if (m_bStop)
			break;

		if ( pItem->m_bIsFolder)
		{
			if (pItem->GetLabel() != "..")
			{
				if (!DoScan(pItem->m_strPath))
				{
					m_bStop=true;
				}
			}
		}
	}
	
	return !m_bStop;
}

int CMusicInfoScanner::RetrieveMusicInfo(VECFILEITEMS& items, const CStdString& strDirectory)
{
	int nFolderCount=CUtil::GetFolderCount(items);
	// Skip items with folders only
	if (nFolderCount == (int)items.size())
		return 0;

	int nFileCount=(int)items.size()-nFolderCount;

	CStdString strItem;
  MAPSONGS songsMap;
  // get all information for all files in current directory from database 
  m_musicDatabase.GetSongsByPath(strDirectory,songsMap);

	int iFilesAdded=0;
	// for every file found, but skip folder
  for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem* pItem=items[i];
		CStdString strExtension;
		CUtil::GetExtension(pItem->m_strPath,strExtension);

		if (m_bStop)
			return iFilesAdded;

    // dont try reading id3tags for folders, playlists or shoutcast streams
		if (!pItem->m_bIsFolder && !CUtil::IsPlayList(pItem->m_strPath) && !CUtil::IsShoutCast(pItem->m_strPath) )
		{
      // is tag for this file already loaded?
			bool bNewFile=false;
			CMusicInfoTag& tag=pItem->m_musicInfoTag;
			if (!tag.Loaded() )
			{
        // no, then we gonna load it.
        // first search for file in our list of the current directory
				CSong song;
        bool bFound(false);
				IMAPSONGS it=songsMap.find(pItem->m_strPath);
				if (it!=songsMap.end())
				{
					song=it->second;
					bFound=true;
				}
				if ( !bFound )
				{
          // if id3 tag scanning is turned on OR we're scanning the directory
          // then parse id3tag from file
          // get correct tag parser
					CMusicInfoTagLoaderFactory factory;
					auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
					if (NULL != pLoader.get())
					{						
              // get id3tag
						if ( pLoader->Load(pItem->m_strPath,tag))
						{
							bNewFile=true;
						}
					}
				}
				else // of if ( !bFound )
				{
					tag.SetSong(song);
				}
			}//if (!tag.Loaded() )
			else
			{
				IMAPSONGS it=songsMap.find(pItem->m_strPath);
				if (it==songsMap.end())
					bNewFile=true;
			}

			if (tag.Loaded() && bNewFile)
			{
				CSong song(tag);
				song.iStartOffset = pItem->m_lStartOffset;
				song.iEndOffset = pItem->m_lEndOffset;
				//m_musicDatabase.BeginTransaction();
				m_musicDatabase.AddSong(song,false);
				//m_musicDatabase.CommitTransaction();
				iFilesAdded++;
			}
		}//if (!pItem->m_bIsFolder)
	}

	return iFilesAdded;
}

