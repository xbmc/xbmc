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

struct SSortMusic
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
				case 0:	//	Sort by Filename
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

			m_bViewAsIcons=g_stSettings.m_bMyMusicAlbumViewAsIcons;
			m_bViewAsIconsRoot=g_stSettings.m_bMyMusicAlbumRootViewAsIcons;

			Update(m_strDirectory);
			CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,m_nSelectedItem);
			CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,m_nSelectedItem);

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
					if (g_stSettings.m_iMyMusicSongsSortMethod >=3) g_stSettings.m_iMyMusicSongsSortMethod=0;
				}
				g_settings.Save();
        UpdateButtons();
        UpdateListControl();

				if (m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory.Find(m_strDirectory) > -1 && g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC_TEMP)
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
			}
			else if (iControl==CONTROL_BTNVIEWASICONS)
      {
				if (m_strDirectory.IsEmpty())
				{
					g_stSettings.m_bMyMusicSongsRootViewAsIcons = !g_stSettings.m_bMyMusicSongsRootViewAsIcons;
					m_bViewAsIconsRoot=g_stSettings.m_bMyMusicSongsRootViewAsIcons;
				}
				else
				{
					g_stSettings.m_bMyMusicSongsViewAsIcons = !g_stSettings.m_bMyMusicSongsViewAsIcons;
					m_bViewAsIcons=g_stSettings.m_bMyMusicSongsViewAsIcons;
				}
				g_settings.Save();
				UpdateButtons();
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
				OnScan(m_vecItems);
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
void CGUIWindowMusicSongs::OnAction(const CAction &action)
{
	if (action.wID==ACTION_PARENT_DIR)
	{
		GoParentFolder();
		return;
	}

  if (action.wID==ACTION_PREVIOUS_MENU)
  {
		m_gWindowManager.ActivateWindow(WINDOW_HOME);
		return;
  }

	if (action.wID==ACTION_SHOW_PLAYLIST)
	{
		m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
		return;
	}

	CGUIWindowMusicBase::OnAction(action);
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

bool CGUIWindowMusicSongs::OnScan(VECFILEITEMS& items)
{
	m_dlgProgress->SetHeading(189);
	m_dlgProgress->SetLine(0,"");
	m_dlgProgress->SetLine(1,"");
	m_dlgProgress->SetLine(2,m_strDirectory );
	m_dlgProgress->StartModal(GetID());

	OnRetrieveMusicInfo(items,true);
	m_dlgProgress->SetLine(2,m_strDirectory );
	if (m_dlgProgress->IsCanceled()) return false;
	
	bool bCancel=false;
	for (int i=0; i < (int)items.size(); ++i)
	{
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
							
				if (!OnScan(subDirItems))
				{
					bCancel=true;
				}
				
				m_strDirectory=strDir;
				if (bCancel) break;
			}
		}
	}
	
	m_dlgProgress->Close();
	return !bCancel;
}

void CGUIWindowMusicSongs::LoadPlayList(const CStdString& strPlayList)
{
	g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Clear();
	CPlayListFactory factory;
	auto_ptr<CPlayList> pPlayList (factory.Create(strPlayList));
	if ( NULL != pPlayList.get())
	{
		pPlayList->Load(strPlayList);

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
		
		m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
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

	//	Update listcontrol and and view by icon/list button
	const CGUIControl* pControl=GetControl(CONTROL_THUMBS);
	if (!pControl->IsVisible())
	{
		CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_LIST,0,0,NULL);
		g_graphicsContext.SendMessage(msg);
		int iItem=msg.GetParam1();
		CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem);
	}
	pControl=GetControl(CONTROL_LIST);
	if (!pControl->IsVisible())
	{
		CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_THUMBS,0,0,NULL);
		g_graphicsContext.SendMessage(msg);
		int iItem=msg.GetParam1();
		CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem);
	}

	SET_CONTROL_HIDDEN(GetID(), CONTROL_LIST);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_THUMBS);

	int iString=101;
	if ( m_strDirectory.IsEmpty() ) 
	{
		if ( m_bViewAsIconsRoot ) 
		{
			SET_CONTROL_VISIBLE(GetID(), CONTROL_THUMBS);
			iString=100;
		}
		else 
		{
			SET_CONTROL_VISIBLE(GetID(), CONTROL_LIST);
		}
	}
	else 
	{
		if ( m_bViewAsIcons ) 
		{
			SET_CONTROL_VISIBLE(GetID(), CONTROL_THUMBS);
			iString=100;
		}
		else 
		{
			SET_CONTROL_VISIBLE(GetID(), CONTROL_LIST);
		}
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
		SET_CONTROL_LABEL(GetID(), CONTROL_BTNSORTBY,g_stSettings.m_iMyMusicSongsSortMethod+103);
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

	if (nMyMusicSortMethod==0||nMyMusicSortMethod==2)
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
				if (nMyMusicSortMethod==0 && pItem->m_musicInfoTag.Loaded()) 
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
				if (nMyMusicSortMethod==0 && CStdString(CUtil::GetExtension(pItem->m_strPath))==".cdda")
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
	SSortMusic sortmethod;

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
