#include "GUIWindowMusicSongs.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "PlayListFactory.h"
#include "util.h"
#include "url.h"
#include "keyboard/virtualkeyboard.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include <algorithm>
#include "GuiUserMessages.h"
#include "SectionLoader.h"

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4

#define CONTROL_BTNTYPE						6
#define CONTROL_BTNPLAYLISTS			7
#define CONTROL_BTNSCAN						9
#define CONTROL_BTNREC						10

#define CONTROL_LABELFILES        12

#define CONTROL_LIST							50
#define CONTROL_THUMBS						51

struct SSortMusicSongs
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
    CFileItem& rpStart=*pStart;
    CFileItem& rpEnd=*pEnd;
		if (rpStart.GetLabel()=="..") return true;
		if (rpEnd.GetLabel()=="..") return false;
		bool bGreater=true;
		if (m_bSortAscending) bGreater=false;

	   if (rpStart.m_bIsFolder == rpEnd.m_bIsFolder)
		{
			char szfilename1[1024];
			char szfilename2[1024];

			switch ( m_iSortMethod ) 
			{
				case 0:	//	Sort by Listlabel
					strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
				break;

				case 1: // Sort by Date
          if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear ) return bGreater;
					if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;
					
					if ( rpStart.m_stTime.wMonth > rpEnd.m_stTime.wMonth ) return bGreater;
					if ( rpStart.m_stTime.wMonth < rpEnd.m_stTime.wMonth ) return !bGreater;
					
					if ( rpStart.m_stTime.wDay > rpEnd.m_stTime.wDay ) return bGreater;
					if ( rpStart.m_stTime.wDay < rpEnd.m_stTime.wDay ) return !bGreater;

					if ( rpStart.m_stTime.wHour > rpEnd.m_stTime.wHour ) return bGreater;
					if ( rpStart.m_stTime.wHour < rpEnd.m_stTime.wHour ) return !bGreater;

					if ( rpStart.m_stTime.wMinute > rpEnd.m_stTime.wMinute ) return bGreater;
					if ( rpStart.m_stTime.wMinute < rpEnd.m_stTime.wMinute ) return !bGreater;

					if ( rpStart.m_stTime.wSecond > rpEnd.m_stTime.wSecond ) return bGreater;
					if ( rpStart.m_stTime.wSecond < rpEnd.m_stTime.wSecond ) return !bGreater;
					return true;
				break;

        case 2:	//	Sort by Size
          if ( rpStart.m_dwSize > rpEnd.m_dwSize) return bGreater;
					if ( rpStart.m_dwSize < rpEnd.m_dwSize) return !bGreater;
					return true;
        break;

        case 3:	//	Sort by TrackNum
					if ( rpStart.m_musicInfoTag.GetTrackNumber() > rpEnd.m_musicInfoTag.GetTrackNumber()) return bGreater;
					if ( rpStart.m_musicInfoTag.GetTrackNumber() < rpEnd.m_musicInfoTag.GetTrackNumber()) return !bGreater;
					return true;
        break;

        case 4:	//	Sort by Duration
					if ( rpStart.m_musicInfoTag.GetDuration() > rpEnd.m_musicInfoTag.GetDuration()) return bGreater;
					if ( rpStart.m_musicInfoTag.GetDuration() < rpEnd.m_musicInfoTag.GetDuration()) return !bGreater;
					return true;
        break;

        case 5:	//	Sort by Title
 					strcpy(szfilename1, rpStart.m_musicInfoTag.GetTitle());
					strcpy(szfilename2, rpEnd.m_musicInfoTag.GetTitle());
        break;

				case 6:	//	Sort by Artist
 					strcpy(szfilename1, rpStart.m_musicInfoTag.GetArtist());
					strcpy(szfilename2, rpEnd.m_musicInfoTag.GetArtist());
				break;

        case 7:	//	Sort by Album
 					strcpy(szfilename1, rpStart.m_musicInfoTag.GetAlbum());
					strcpy(szfilename2, rpEnd.m_musicInfoTag.GetAlbum());
        break;

        case 8:	//	Sort by FileName
 					strcpy(szfilename1, rpStart.m_strPath);
					strcpy(szfilename2, rpEnd.m_strPath);
        break;

				default:	//	Sort by Filename by default
					strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
				break;
			}


			for (int i=0; i < (int)strlen(szfilename1); i++)
				szfilename1[i]=tolower((unsigned char)szfilename1[i]);
			
			for (int i=0; i < (int)strlen(szfilename2); i++)
			{
				szfilename2[i]=tolower((unsigned char)szfilename2[i]);
			}
			//return (rpStart.strPath.compare( rpEnd.strPath )<0);

			if (m_bSortAscending)
				return (strcmp(szfilename1,szfilename2)<0);
			else
				return (strcmp(szfilename1,szfilename2)>=0);
		}
    if (!rpStart.m_bIsFolder) return false;
		return true;
	}

	int m_iSortMethod;
	int m_bSortAscending;
	CStdString m_strDirectory;
};

CGUIWindowMusicSongs::CGUIWindowMusicSongs(void)
:CGUIWindowMusicBase()
{
	m_strDirectory="?";
	m_bScan=false;
}
CGUIWindowMusicSongs::~CGUIWindowMusicSongs(void)
{

}

bool CGUIWindowMusicSongs::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_DEINIT:
		{
			m_nSelectedItem=GetSelectedItem();
		}
		break;

    case GUI_MSG_WINDOW_INIT:
		{
			//	This window is started by the home window.
			//	Now we decide which my music window has to be shown and
			//	switch to the my music window the user last activated.
			if (g_stSettings.m_iMyMusicStartWindow!=GetID())
			{
				m_gWindowManager.ActivateWindow(g_stSettings.m_iMyMusicStartWindow);
				return false;
			}

			CGUIWindowMusicBase::OnMessage(message);

			if (m_strDirectory=="?")
				m_strDirectory=g_stSettings.m_szDefaultMusic;

			if (CUtil::IsCDDA(m_strDirectory) || CUtil::IsDVD(m_strDirectory) || CUtil::IsISO9660(m_strDirectory))
			{
				//	No disc in drive but current directory is a dvd share
				if (!CDetectDVDMedia::IsDiscInDrive())
					m_strDirectory.Empty();

				//	look if disc has changed outside this window and url is still the same
				CStdString strDVDUrl=m_rootDir.GetDVDDriveUrl();
				if (CUtil::IsCDDA(m_strDirectory) && !CUtil::IsCDDA(strDVDUrl))
					m_strDirectory.Empty();
				if (CUtil::IsDVD(m_strDirectory) && !CUtil::IsDVD(strDVDUrl))
					m_strDirectory.Empty();
				if (CUtil::IsISO9660(m_strDirectory) && !CUtil::IsISO9660(strDVDUrl))
					m_strDirectory.Empty();
			}

			m_iViewAsIcons=g_stSettings.m_iMyMusicSongsViewAsIcons;
			m_iViewAsIconsRoot=g_stSettings.m_iMyMusicSongsRootViewAsIcons;

			Update(m_strDirectory);
 
			if (m_nSelectedItem>-1)
			{
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,m_nSelectedItem);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,m_nSelectedItem);
			}

			return true;
		}
		break;

		case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();

 			if (iControl==CONTROL_BTNSORTBY) // sort by
      {
				if (m_strDirectory.IsEmpty())
				{
					g_stSettings.m_iMyMusicSongsRootSortMethod++;
					if (g_stSettings.m_iMyMusicSongsRootSortMethod >=3) g_stSettings.m_iMyMusicSongsRootSortMethod=0;
				}
				else
				{
					g_stSettings.m_iMyMusicSongsSortMethod++;
					if (g_stSettings.m_iMyMusicSongsSortMethod >=9) g_stSettings.m_iMyMusicSongsSortMethod=0;
					if (g_stSettings.m_iMyMusicSongsSortMethod >=3) g_stSettings.m_iMyMusicSongsSortMethod=8;
				}
				g_settings.Save();

				int nItem=GetSelectedItem();
				CFileItem*pItem=m_vecItems[nItem];
				CStdString strSelected=pItem->m_strPath;
        
        UpdateButtons();
        UpdateListControl();

				if (!m_strDirectory.IsEmpty() && m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory.Find(m_strDirectory) > -1 && g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC_TEMP)
				{
					int nSong=g_playlistPlayer.GetCurrentSong();
					const CPlayList::CPlayListItem item=g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP)[nSong];
					g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Clear();
					int nFolderCount=0;
					for (int i = 0; i < (int)m_vecItems.size(); i++) 
					{
						CFileItem* pItem = m_vecItems[i];
						if (pItem->m_bIsFolder) 
						{
							nFolderCount++;
							continue;
						}
						CPlayList::CPlayListItem playlistItem ;
						playlistItem.SetFileName(pItem->m_strPath);
						playlistItem.SetDescription(pItem->GetLabel());
						playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
						g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
						if (item.GetFileName()==pItem->m_strPath)
							g_playlistPlayer.SetCurrentSong(i-nFolderCount);
					}
				}

				for (int i=0; i<(int)m_vecItems.size(); i++)
				{
					CFileItem* pItem = m_vecItems[i];
					if (pItem->m_strPath==strSelected)
					{
						CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,i);
						CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,i);
						break;
					}
				}
			}
			else if (iControl==CONTROL_BTNVIEWASICONS)
      {
				CGUIWindowMusicBase::OnMessage(message);
				g_stSettings.m_iMyMusicSongsRootViewAsIcons=m_iViewAsIconsRoot;
				g_stSettings.m_iMyMusicSongsViewAsIcons=m_iViewAsIcons;
				g_settings.Save();
				return true;
			}
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
				if (m_strDirectory.IsEmpty())
	        g_stSettings.m_bMyMusicSongsRootSortAscending=!g_stSettings.m_bMyMusicSongsRootSortAscending;
				else
	        g_stSettings.m_bMyMusicSongsSortAscending=!g_stSettings.m_bMyMusicSongsSortAscending;

				g_settings.Save();
        UpdateButtons();
        UpdateListControl();
      }
			else if (iControl==CONTROL_BTNPLAYLISTS)
			{
				CStdString strDirectory;
				strDirectory.Format("%s\\playlists",g_stSettings.m_szAlbumDirectory);
				if (strDirectory!=m_strDirectory)
				{
					Update(strDirectory);
					SET_CONTROL_FOCUS(GetID(), CONTROL_BTNPLAYLISTS);
				}
			}
 			else if (iControl==CONTROL_BTNSCAN)
			{
				OnScan();
			}
			else if (iControl==CONTROL_BTNREC)
			{
				if (g_application.IsPlayingAudio() )
				{
					if (g_application.m_pPlayer->CanRecord() )
					{
						bool bIsRecording=g_application.m_pPlayer->IsRecording();
						g_application.m_pPlayer->Record(!bIsRecording);
						UpdateButtons();
					}
				}
			}
		}
		break;
	}

	return CGUIWindowMusicBase::OnMessage(message);
}

void CGUIWindowMusicSongs::GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)
{
	if (items.size() )
	{
		// cleanup items
		CFileItemList itemlist(items);
	}

	CStdString strParentPath;
	bool bParentExists=CUtil::GetParentPath(strDirectory, strParentPath);

	CStdString strPlayListDir;
	strPlayListDir.Format("%s\\playlists",g_stSettings.m_szAlbumDirectory);
	if (strPlayListDir==strDirectory) 
	{
		bParentExists=true;
		strParentPath=m_strPrevDir;
	}

	// check if current directory is a root share
	if ( !m_rootDir.IsShare(strDirectory) )
	{
		// no, do we got a parent dir?
		if ( bParentExists )
		{
			// yes
			CFileItem *pItem = new CFileItem("..");
			pItem->m_strPath=strParentPath;
			pItem->m_bIsFolder=true;
			pItem->m_bIsShareOrDrive=false;
			items.push_back(pItem);
		}
	}
	else
	{
		// yes, this is the root of a share
		// add parent path to the virtual directory
		CFileItem *pItem = new CFileItem("..");
		pItem->m_strPath="";
		pItem->m_bIsShareOrDrive=false;
		pItem->m_bIsFolder=true;
		items.push_back(pItem);
	}
	m_rootDir.GetDirectory(strDirectory,items);

	if (strPlayListDir!=strDirectory) 
	{
		m_strPrevDir=strDirectory;
	}

}

void CGUIWindowMusicSongs::OnScan()
{
	DWORD dwTick=timeGetTime();

	g_application.DisableOverlay();

	m_dlgProgress->SetHeading(189);
	m_dlgProgress->SetLine(0, 330);
	m_dlgProgress->SetLine(1,"");
	m_dlgProgress->SetLine(2,m_strDirectory );
	m_dlgProgress->StartModal(GetID());

	// Preload section for ID3 cover art reading
	CSectionLoader::Load("CXIMAGE");

	CUtil::ThumbCacheClear();

	m_database.BeginTransaction();

	// enable scan mode in OnRetrieveMusicInfo()
	m_bScan=true;

	if (DoScan(m_vecItems))
	{
		m_dlgProgress->SetLine(0,328);
		m_dlgProgress->SetLine(1,"");
		m_dlgProgress->SetLine(2,330 );
		m_dlgProgress->Progress();
		m_database.CommitTransaction();
	}
	else
		m_database.RollbackTransaction();

	m_database.EmptyCache();

	CSectionLoader::Unload("CXIMAGE");

	// disable scan mode
	m_bScan=false;

	CUtil::ThumbCacheClear();

	g_application.EnableOverlay();

	m_dlgProgress->Close();

	dwTick = timeGetTime() - dwTick;
	CStdString strTmp, strTmp1;
	CUtil::SecondsToHMSString(dwTick/1000, strTmp1);
	strTmp.Format("OnScan() took %s\n", strTmp1); 
	OutputDebugString(strTmp.c_str());
}

bool CGUIWindowMusicSongs::DoScan(VECFILEITEMS& items)
{
	m_dlgProgress->SetLine(2,m_strDirectory );
	m_dlgProgress->Progress();

	OnRetrieveMusicInfo(items);
	m_database.CheckVariousArtistsAndCoverArt();
	
	if (m_dlgProgress->IsCanceled()) return false;
	
	bool bCancel=false;
	for (int i=0; i < (int)items.size(); ++i)
	{
    g_application.ResetScreenSaver();
		CFileItem *pItem= items[i];
		if (m_dlgProgress->IsCanceled())
		{
			bCancel=true;
			break;
		}
		if ( pItem->m_bIsFolder)
		{
			if (pItem->GetLabel() != "..")
			{
				// load subfolder
				CStdString strDir=m_strDirectory;
				m_strDirectory=pItem->m_strPath;
				VECFILEITEMS subDirItems;
				CFileItemList itemlist(subDirItems);
				m_rootDir.GetDirectory(pItem->m_strPath,subDirItems);

				if (!DoScan(subDirItems))
				{
					bCancel=true;
				}
				
				m_strDirectory=strDir;
				if (bCancel) break;
			}
		}
	}
	
	return !bCancel;
}

void CGUIWindowMusicSongs::LoadPlayList(const CStdString& strPlayList)
{
	CPlayListFactory factory;
	auto_ptr<CPlayList> pPlayList (factory.Create(strPlayList));
	if ( NULL != pPlayList.get())
	{
		if (!pPlayList->Load(strPlayList))
			return;

		g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Clear();

    if (pPlayList->size() == 1)
    {
      // just 1 song? then play it (no need to have a playlist of 1 song)
      CPlayList::CPlayListItem item=(*pPlayList)[0];
      g_application.PlayFile(item.GetFileName());
      return;
    }


		//	Do not autoshuffle shoutcast playlists
		CStdString strFileName;
		if ((*pPlayList).size())
			strFileName=(*pPlayList)[0].GetFileName();
		if (!CUtil::IsShoutCast(strFileName) && g_stSettings.m_bAutoShufflePlaylist)
			pPlayList->Shuffle();

		for (int i=0; i < (int)pPlayList->size(); ++i)
		{
			const CPlayList::CPlayListItem& playListItem =(*pPlayList)[i];
			CStdString strLabel=playListItem.GetDescription();
			if (strLabel.size()==0) 
				strLabel=CUtil::GetFileName(playListItem.GetFileName());

			CPlayList::CPlayListItem playlistItem;
			playlistItem.SetFileName(playListItem.GetFileName());
			playlistItem.SetDescription(strLabel);
			playlistItem.SetDuration(playListItem.GetDuration());
			g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).Add(playlistItem);
		}
	} 

	if (g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).size() )
	{
		CPlayList& playlist=g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC );
		const CPlayList::CPlayListItem& item=playlist[0];
		if ( !CUtil::IsShoutCast(item.GetFileName()) )
			g_playlistPlayer.Play(0);
		
    if (GetID() == m_gWindowManager.GetActiveWindow())
    {
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
	}
}

void CGUIWindowMusicSongs::UpdateButtons()
{
	bool bIsPlaying=g_application.IsPlayingAudio();
	bool bCanRecord=false;
	bool bIsRecording=false;
	if (bIsPlaying)
	{
		bCanRecord=g_application.m_pPlayer->CanRecord();
		bIsRecording=g_application.m_pPlayer->IsRecording();
	}

	//	Update Record button
	if (bIsPlaying && bCanRecord)
	{
		CONTROL_ENABLE(GetID(), CONTROL_BTNREC);
		if (bIsRecording)
		{
				SET_CONTROL_LABEL(GetID(), CONTROL_BTNREC,265);//Stop Recording
		}
		else
		{
				SET_CONTROL_LABEL(GetID(), CONTROL_BTNREC,264);//Record
		}
	}
	else
	{
		SET_CONTROL_LABEL(GetID(), CONTROL_BTNREC,264);//Record
		CONTROL_DISABLE(GetID(), CONTROL_BTNREC);
	}

	//	Update sorting control
	bool bSortAscending=false;
	if (m_strDirectory.IsEmpty())
		bSortAscending=g_stSettings.m_bMyMusicSongsRootSortAscending;
	else
		bSortAscending=g_stSettings.m_bMyMusicSongsSortAscending;

	if (bSortAscending)
	{
    CGUIMessage msg(GUI_MSG_DESELECTED,GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_SELECTED,GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }

	//	Update listcontrol and view by icon/list button
	const CGUIControl* pControl=GetControl(CONTROL_THUMBS);
  if (pControl)
  {
	  if (!pControl->IsVisible())
	  {
		  CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_LIST,0,0,NULL);
		  g_graphicsContext.SendMessage(msg);
		  int iItem=msg.GetParam1();
		  CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem);
	  }
  }
	pControl=GetControl(CONTROL_LIST);
  if (pControl)
  {
	  if (!pControl->IsVisible())
	  {
		  CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_THUMBS,0,0,NULL);
		  g_graphicsContext.SendMessage(msg);
		  int iItem=msg.GetParam1();
		  CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem);
	  }
  }

	SET_CONTROL_HIDDEN(GetID(), CONTROL_LIST);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_THUMBS);

	bool bViewIcon = false;
  int iString;
	if ( m_strDirectory.IsEmpty() ) 
  {
		switch (m_iViewAsIconsRoot)
    {
      case VIEW_AS_LIST:
        iString=100; // view as icons
      break;
      
      case VIEW_AS_ICONS:
        iString=417;  // view as large icons
        bViewIcon=true;
      break;
      case VIEW_AS_LARGEICONS:
        iString=101; // view as list
        bViewIcon=true;
      break;
    }
	}
	else 
  {
		switch (m_iViewAsIcons)
    {
      case VIEW_AS_LIST:
        iString=100; // view as icons
      break;
      
      case VIEW_AS_ICONS:
        iString=417;  // view as large icons
        bViewIcon=true;
      break;
      case VIEW_AS_LARGEICONS:
        iString=101; // view as list
        bViewIcon=true;
      break;
    }		
	}

	if (bViewIcon) 
	{
		SET_CONTROL_VISIBLE(GetID(), CONTROL_THUMBS);
	}
	else
	{
		SET_CONTROL_VISIBLE(GetID(), CONTROL_LIST);
	}

	SET_CONTROL_LABEL(GetID(), CONTROL_BTNVIEWASICONS,iString);

	//	Update object count label
	int iItems=m_vecItems.size();
	if (iItems)
	{
		CFileItem* pItem=m_vecItems[0];
		if (pItem->GetLabel()=="..") iItems--;
	}
  WCHAR wszText[20];
  const WCHAR* szText=g_localizeStrings.Get(127).c_str();
  swprintf(wszText,L"%i %s", iItems,szText);

	SET_CONTROL_LABEL(GetID(), CONTROL_LABELFILES,wszText);

	//	Update sort by button
	if (m_strDirectory.IsEmpty())
	{
			SET_CONTROL_LABEL(GetID(), CONTROL_BTNSORTBY,g_stSettings.m_iMyMusicSongsRootSortMethod+103);
	}
	else
	{
		if (g_stSettings.m_iMyMusicSongsSortMethod<=2)
		{
			//	Sort by Name(ItemLabel), Date, Size
			SET_CONTROL_LABEL(GetID(), CONTROL_BTNSORTBY,g_stSettings.m_iMyMusicSongsSortMethod+103);
		}
		else
		{
			//	Sort by FileName
			SET_CONTROL_LABEL(GetID(), CONTROL_BTNSORTBY,363);
		}
	}
}

void CGUIWindowMusicSongs::OnClick(int iItem)
{
	CFileItem* pItem=m_vecItems[iItem];
	CStdString strPath=pItem->m_strPath;
	if (pItem->m_bIsFolder)
	{
		if ( pItem->m_bIsShareOrDrive ) 
		{
			if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
				return;
		}
		Update(strPath);
	}
	else
	{
		if ( CUtil::IsPlayList(strPath)  )
		{
			LoadPlayList(strPath);
		}
		else
		{
			//play and add current directory to temporary playlist
			int nFolderCount=0;
			g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC_TEMP ).Clear();
			for ( int i = 0; i < (int) m_vecItems.size(); i++ ) 
			{
				CFileItem* pItem = m_vecItems[i];
				if ( pItem->m_bIsFolder ) 
				{
					nFolderCount++;
					continue;
				}
				CPlayList::CPlayListItem playlistItem ;
				playlistItem.SetFileName(pItem->m_strPath);
				playlistItem.SetDescription(pItem->GetLabel());
				playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
				g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
			}

			//	Save current window and directory to know where the selected item was
			m_nTempPlayListWindow=GetID();
			m_strTempPlayListDirectory=m_strDirectory;

			g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC_TEMP);
			g_playlistPlayer.Play(iItem-nFolderCount);
		}
	}
}

void CGUIWindowMusicSongs::OnFileItemFormatLabel(CFileItem* pItem)
{
	//	set label 1
	if (pItem->m_musicInfoTag.Loaded())
	{
		CStdString str;
		CMusicInfoTag& tag=pItem->m_musicInfoTag;
		CStdString strTitle=tag.GetTitle();
		CStdString strArtist=tag.GetArtist();
		if (strTitle.size()) 
		{
			if (strArtist)
			{
				int iTrack=tag.GetTrackNumber();
				if (iTrack>0)
					str.Format("%02.2i. %s - %s",iTrack, tag.GetArtist().c_str(), tag.GetTitle().c_str());
				else 
					str.Format("%s - %s", tag.GetArtist().c_str(), tag.GetTitle().c_str());
			}
			else
			{
				int iTrack=tag.GetTrackNumber();
				if (iTrack>0)
					str.Format("%02.2i. %s ",iTrack, tag.GetTitle().c_str());
				else 
					str.Format("%s", tag.GetTitle().c_str());
			}
			pItem->SetLabel( str );
		}
	}

	//	set label 2

	int nMyMusicSortMethod=0;
	if (m_strDirectory.IsEmpty())
		nMyMusicSortMethod=g_stSettings.m_iMyMusicSongsRootSortMethod;
	else
		nMyMusicSortMethod=g_stSettings.m_iMyMusicSongsSortMethod;

	if (nMyMusicSortMethod==0||nMyMusicSortMethod==2||nMyMusicSortMethod==8)
	{
			if (pItem->m_bIsFolder) 
				pItem->SetLabel2("");
			else 
			{
				if (pItem->m_dwSize > 0) 
				{
					CStdString strFileSize;
					CUtil::GetFileSize(pItem->m_dwSize, strFileSize);
					pItem->SetLabel2(strFileSize);
				}
				if ((nMyMusicSortMethod==0 || nMyMusicSortMethod==8) && pItem->m_musicInfoTag.Loaded()) 
				{
					int nDuration=pItem->m_musicInfoTag.GetDuration();
					if (nDuration)
					{
						CStdString strDuration;
						CUtil::SecondsToHMSString(nDuration, strDuration);
						pItem->SetLabel2(strDuration);
					}
				}
				//	cdda items always have duration
				if ((nMyMusicSortMethod==0 || nMyMusicSortMethod==8) && CStdString(CUtil::GetExtension(pItem->m_strPath))==".cdda")
				{
					int nDuration=pItem->m_musicInfoTag.GetDuration();
					if (nDuration)
					{
						CStdString strDuration;
						CUtil::SecondsToHMSString(nDuration, strDuration);
						pItem->SetLabel2(strDuration);
					}
				}
			}
		}
		else
		{
			if (pItem->m_stTime.wYear)
			{
				CStdString strDateTime;
				CUtil::GetDate(pItem->m_stTime, strDateTime);
				pItem->SetLabel2(strDateTime);
			}
			else
				pItem->SetLabel2("");
		}
}

void CGUIWindowMusicSongs::DoSort(VECFILEITEMS& items)
{
	SSortMusicSongs sortmethod;

	sortmethod.m_strDirectory=m_strDirectory;

	if (m_strDirectory.IsEmpty())
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyMusicSongsRootSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyMusicSongsRootSortAscending;
	}
	else
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyMusicSongsSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyMusicSongsSortAscending;
	}

	sort(items.begin(), items.end(), sortmethod);
}

void CGUIWindowMusicSongs::OnRetrieveMusicInfo(VECFILEITEMS& items)
{

  //**** TO SPEED UP SCANNING *****
  // We need to speedup scanning of music
  // what we could do is:
  // 1. cache all genres & artists (with name and id) in a map (read them @ start of this routine)
  // 2. cache current path with database id
  // 3. scan the entire dir and then if we find a new song:
  // 4.   check if genre is known already (in cached map) if not then add it to database & map
  // 4.   check if artist is known already (in cached map) if not then add it to database & map
  // 5.   add the new song to the database 
  //         we need a new version of musicdatabase::addsong() for this
  //         which just adds the song (and not the artists/genre/path like musicdatabase::addsong does now!)
  //
  // this way we prevent that for each call to musicdatabase::addsong
  //    -the genre id is looked up and or added
  //    -the artist id is looked up and or added
  //    -the path id is lookup and or added

  
	int nFolderCount=CUtil::GetFolderCount(items);
	// Skip items with folders only
	if (nFolderCount == (int)items.size())
		return;

	int nFileCount=(int)items.size()-nFolderCount;

	CStdString strItem;
  MAPSONGS songsMap;
  // get all information for all files in current directory from database 
  m_database.GetSongsByPath(m_strDirectory,songsMap);

  // for every file found, but skip folder
  for (int i=nFolderCount; i < (int)items.size(); ++i)
	{
    g_application.ResetScreenSaver();
		CFileItem* pItem=items[i];
		CStdString strExtension;
		CUtil::GetExtension(pItem->m_strPath,strExtension);

		if (m_bScan)
			m_dlgProgress->ProgressKeys();

    // dont try reading id3tags for folders or playlists
		if (!pItem->m_bIsFolder && !CUtil::IsPlayList(pItem->m_strPath) )
		{
      // is tag for this file already loaded?
			bool bNewFile=false;
			CMusicInfoTag& tag=pItem->m_musicInfoTag;
			if (!tag.Loaded() )
			{
        // no, then we gonna load it. But dont load tags from cdda files
				if (strExtension!=".cdda" )
				{
          // first search for file in our list of the current directory
					CSong song;
          bool bFound(false);
					IMAPSONGS it=songsMap.find(pItem->m_strPath);
					if (it!=songsMap.end())
					{
						song=it->second;
						bFound=true;
					}
          if (!bFound && !m_bScan)
          {
            // try finding it in the database
            CStdString strPathName;
            CStdString strFileName;
            CUtil::Split(pItem->m_strPath, strPathName, strFileName);
            if (strPathName != m_strDirectory)
            {
              if ( m_database.GetSongByFileName(pItem->m_strPath, song) )
              {
                bFound=true;
              }
            }
          }
					if ( !bFound )
					{
            // if id3 tag scanning is turned on OR we're scanning the directory
            // then parse id3tag from file
						if (g_stSettings.m_bUseID3 || m_bScan)
						{
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
					}
					else // of if ( !bFound )
					{
						tag.SetAlbum(song.strAlbum);
						tag.SetArtist(song.strArtist);
						tag.SetGenre(song.strGenre);
						tag.SetDuration(song.iDuration);
						tag.SetTitle(song.strTitle);
						tag.SetTrackNumber(song.iTrack);
						tag.SetLoaded(true);
					}
				}//if (strExtension!=".cdda" )
			}//if (!tag.Loaded() )
			else if (m_bScan)
			{
				IMAPSONGS it=songsMap.find(pItem->m_strPath);
				if (it==songsMap.end())
					bNewFile=true;
			}

			if (tag.Loaded() && m_bScan && bNewFile)
			{
				SYSTEMTIME stTime;
				tag.GetReleaseDate(stTime);
				CSong song;
				song.strTitle		= tag.GetTitle();
				song.strGenre		= tag.GetGenre();
				song.strFileName= pItem->m_strPath;
				song.strArtist	= tag.GetArtist();
				song.strAlbum		= tag.GetAlbum();
				song.iYear			=	stTime.wYear;
				song.iTrack			= tag.GetTrackNumber();
				song.iDuration	= tag.GetDuration();

				m_database.AddSong(song,false);
			}
		}//if (!pItem->m_bIsFolder)
	}
}
