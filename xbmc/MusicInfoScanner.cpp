#include "stdafx.h"
#include "MusicInfoScanner.h"
#include "musicdatabase.h"
#include "musicInfoTagLoaderFactory.h"
#include "Filesystem/cddadirectory.h"
#include "Filesystem/hddirectory.h"
#include "FileSystem/DirectoryCache.h"
#include "Util.h"
#include "Application.h"

using namespace MUSIC_INFO;
using namespace DIRECTORY;

CMusicInfoScanner::CMusicInfoScanner()
{
	m_bRunning=false;
	m_pObserver=NULL;
	m_bCanInterrupt=false;
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

		m_bCanInterrupt=true;

		// check whether we have scanned here before
		CStdString strPaths;
		if (!m_musicDatabase.GetSubpathsFromPath(m_strStartDir, strPaths))
		{
			m_musicDatabase.Close();
			return;
		}

		// Preload section for ID3 cover art reading
		CSectionLoader::Load("CXIMAGE");
		CSectionLoader::Load("LIBMP4");

		CUtil::ThumbCacheClear();
	  g_directoryCache.InitMusicThumbCache();

		m_musicDatabase.BeginTransaction();

		bool bOKtoScan = true;
		if (m_bUpdateAll)
		{
			if (m_pObserver)
				m_pObserver->OnStateChanged(REMOVING_OLD);

			bOKtoScan = m_musicDatabase.RemoveSongsFromPaths(strPaths);
		}

		if (bOKtoScan)
		{
			if (m_bUpdateAll)
			{
				if (m_pObserver)
					m_pObserver->OnStateChanged(CLEANING_UP_DATABASE);

				bOKtoScan = m_musicDatabase.CleanupAlbumsArtistsGenres(strPaths);
			}

			if (m_pObserver)
				m_pObserver->OnStateChanged(READING_MUSIC_INFO);

			//	Database operations should not be canceled
			//	using Interupt() while scanning as it could 
			//	result in unexpected behaviour.
			m_bCanInterrupt=false;

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

					m_musicDatabase.Compress();
				}
			}
			else
				m_musicDatabase.RollbackTransaction();
		}
		else
			m_musicDatabase.RollbackTransaction();

		m_musicDatabase.EmptyCache();

		CSectionLoader::Unload("CXIMAGE");
		CSectionLoader::Unload("LIBMP4");

		CUtil::ThumbCacheClear();
    g_directoryCache.ClearMusicThumbCache();

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
	if (m_bCanInterrupt)
		m_musicDatabase.Interupt();

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
	CFileItemList items;
	CDirectory dir;
	dir.GetDirectory(strDirectory,items);
	// filter items in the sub dir (for .cue sheet support)
	items.FilterCueItems();
	CUtil::SortFileItemsByName(items);

	if (RetrieveMusicInfo(items, strDirectory)>0)
	{
		m_musicDatabase.CheckVariousArtistsAndCoverArt();

		if (m_pObserver)
			m_pObserver->OnDirectoryScanned(strDirectory);
	}

	for (int i=0; i < items.Size(); ++i)
	{
		CFileItem *pItem= items[i];

		if (m_bStop)
			break;

		if (pItem->m_bIsFolder && pItem->GetLabel() != "..")
		{
      // get the item's thumb (this will cache the album thumb)
      pItem->SetMusicThumb();
			if (!DoScan(pItem->m_strPath))
			{
				m_bStop=true;
			}
		}
	}
	
	return !m_bStop;
}

int CMusicInfoScanner::RetrieveMusicInfo(CFileItemList& items, const CStdString& strDirectory)
{
	int nFolderCount=items.GetFolderCount();
	// Skip items with folders only
	if (nFolderCount == items.Size())
		return 0;

	int nFileCount=items.Size()-nFolderCount;

	CStdString strItem;
  MAPSONGS songsMap;
  // get all information for all files in current directory from database 
  m_musicDatabase.GetSongsByPath(strDirectory,songsMap);

	int iFilesAdded=0;
	// for every file found, but skip folder
  for (int i=0; i < items.Size(); ++i)
	{
		CFileItem* pItem=items[i];
		CStdString strExtension;
		CUtil::GetExtension(pItem->m_strPath,strExtension);

		if (m_bStop)
			return iFilesAdded;

    // dont try reading id3tags for folders, playlists or shoutcast streams
		if (!pItem->m_bIsFolder && !pItem->IsPlayList() && !pItem->IsShoutCast() )
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
        pItem->SetMusicThumb();
        song.strThumb = pItem->GetThumbnailImage();
				m_musicDatabase.AddSong(song,false);
				iFilesAdded++;
			}
		}//if (!pItem->m_bIsFolder)
	}

	return iFilesAdded;
}

