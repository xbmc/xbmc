
#include "stdafx.h"
#include "GUIWindowMusicNav.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "PlayListFactory.h"
#include "util.h"
#include "url.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include <algorithm>
#include "GuiUserMessages.h"
#include "GUIPassword.h"

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY			3
#define CONTROL_BTNSORTASC			4

#define CONTROL_LABELFILES        	12
#define CONTROL_FILTER				15

#define CONTROL_BTNSHUFFLE			21

#define CONTROL_LIST				50
#define CONTROL_THUMBS				51

#define SHOW_ROOT					0
#define SHOW_GENRES					8
#define SHOW_ARTISTS				4
#define SHOW_ALBUMS					2
#define SHOW_SONGS					1

struct SSortMusicNav
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
    	CFileItem& rpStart=*pStart;
   		CFileItem& rpEnd=*pEnd;
		if (rpStart.GetLabel()=="..") return true;
		if (rpEnd.GetLabel()=="..") return false;

		if (rpStart.m_strPath.IsEmpty())
			return true;

		if (rpEnd.m_strPath.IsEmpty())
			return false;

		bool bGreater=true;
		if (m_bSortAscending) bGreater=false;

	   	if (rpStart.m_bIsFolder == rpEnd.m_bIsFolder)
		{
			CStdString strStart;
			CStdString strEnd;
			CStdString strTemp;

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
					// remove "the" for sorting
					strStart = rpStart.m_musicInfoTag.GetTitle();
					strEnd = rpEnd.m_musicInfoTag.GetTitle();
					if (strStart.Left(4).Equals("The "))
					{
						strTemp = strStart.Left(4);
						strStart.TrimLeft(strTemp);
					}
					if (strEnd.Left(4).Equals("The "))
					{
						strTemp = strEnd.Left(4);
						strEnd.TrimLeft(strTemp);
					} 
 					strcpy(szfilename1, strStart.c_str());
					strcpy(szfilename2, strEnd.c_str());
					//strcpy(szfilename1, rpStart.m_musicInfoTag.GetTitle());
					//strcpy(szfilename2, rpEnd.m_musicInfoTag.GetTitle());
				break;

				case 6:	//	Sort by Artist
					// remove "the" for sorting
					strStart = rpStart.m_musicInfoTag.GetArtist();
					strEnd = rpEnd.m_musicInfoTag.GetArtist();
					if (strStart.Left(4).Equals("The "))
					{
						strTemp = strStart.Left(4);
						strStart.TrimLeft(strTemp);
					}
					if (strEnd.Left(4).Equals("The "))
					{
						strTemp = strEnd.Left(4);
						strEnd.TrimLeft(strTemp);
					} 
 					strcpy(szfilename1, strStart.c_str());
					strcpy(szfilename2, strEnd.c_str());
 					//strcpy(szfilename1, rpStart.m_musicInfoTag.GetArtist());
					//strcpy(szfilename2, rpEnd.m_musicInfoTag.GetArtist());
				break;

				case 7:	//	Sort by Album
					// remove "the" for sorting
					strStart = rpStart.m_musicInfoTag.GetAlbum();
					strEnd = rpEnd.m_musicInfoTag.GetAlbum();
					if (strStart.Left(4).Equals("The "))
					{
						strTemp = strStart.Left(4);
						strStart.TrimLeft(strTemp);
					}
					if (strEnd.Left(4).Equals("The "))
					{
						strTemp = strEnd.Left(4);
						strEnd.TrimLeft(strTemp);
					} 
 					strcpy(szfilename1, strStart.c_str());
					strcpy(szfilename2, strEnd.c_str());
 					//strcpy(szfilename1, rpStart.m_musicInfoTag.GetAlbum());
					//strcpy(szfilename2, rpEnd.m_musicInfoTag.GetAlbum());
				break;

				default: //	Sort by Filename by default
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

CGUIWindowMusicNav::CGUIWindowMusicNav(void)
:CGUIWindowMusicBase()
{
	m_iState=SHOW_ROOT;
	m_iPath=m_iState;
	m_strGenre="";
	m_strArtist="";
	m_strAlbum="";
}
CGUIWindowMusicNav::~CGUIWindowMusicNav(void)
{

}

bool CGUIWindowMusicNav::OnMessage(CGUIMessage& message)
{
	switch (message.GetMessage())
	{
		case GUI_MSG_WINDOW_INIT:
		{
			if (m_iViewAsIcons==-1 && m_iViewAsIconsRoot==-1)
			{
				m_iViewAsIcons=g_stSettings.m_iMyMusicNavRootViewAsIcons;
				m_iViewAsIconsRoot=g_stSettings.m_iMyMusicNavRootViewAsIcons;
			}
		}
		break;

		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		case GUI_MSG_DVDDRIVE_CHANGED_CD:
			return true;
		break;

		case GUI_MSG_CLICKED:
		{
			int iControl=message.GetSenderId();
 			if (iControl==CONTROL_BTNSORTBY) // sort by
			{
				if (m_iState==SHOW_ROOT||m_iState==SHOW_GENRES||m_iState==SHOW_ARTISTS)
				{
					// sort by label
					// root is not actually sorted though
					g_stSettings.m_iMyMusicNavRootSortMethod=0; 
				}
				else if (m_iState==SHOW_ALBUMS)
				{
					// allow sort by 6,7
					g_stSettings.m_iMyMusicNavAlbumsSortMethod++;
					if (g_stSettings.m_iMyMusicNavAlbumsSortMethod >=8) g_stSettings.m_iMyMusicNavAlbumsSortMethod=6;
				}
				else if (m_iState==SHOW_SONGS)
				{
					// allow sort by 3,4,5,6,7
					g_stSettings.m_iMyMusicNavSongsSortMethod++;
					if (g_stSettings.m_iMyMusicNavSongsSortMethod >=8) g_stSettings.m_iMyMusicNavSongsSortMethod=3;
				}
				g_settings.Save();

				int nItem=GetSelectedItem();
				CFileItem*pItem=m_vecItems[nItem];
				CStdString strSelected=pItem->m_strPath;
				
				UpdateButtons();
				UpdateListControl();

				CStdString strDirectory=m_Directory.m_strPath;
				if (CUtil::HasSlashAtEnd(strDirectory))
					strDirectory.Delete(strDirectory.size()-1);

				if (!strDirectory.IsEmpty() && m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory==strDirectory && g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC_TEMP)
				{
					int nSong=g_playlistPlayer.GetCurrentSong();
					const CPlayList::CPlayListItem item=g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP)[nSong];
					g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Clear();
					g_playlistPlayer.Reset();
					int nFolderCount=0;
					for (int i = 0; i < (int) m_vecItems.size(); i++) 
					{
						CFileItem* pItem = m_vecItems[i];
						if (pItem->m_bIsFolder) 
						{
							nFolderCount++;
							continue;
						}
						CPlayList::CPlayListItem playlistItem;
						CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
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
						CONTROL_SELECT_ITEM(CONTROL_LIST,i);
						CONTROL_SELECT_ITEM(CONTROL_THUMBS,i);
						break;
					}
				}
				return true;
			}
			else if (iControl==CONTROL_BTNVIEWASICONS)
			{
				// ViewAs is an odd case where you want to call the base
				// class first
				CGUIWindowMusicBase::OnMessage(message);

				if (m_iState==SHOW_ROOT)
					g_stSettings.m_iMyMusicNavRootViewAsIcons=m_iViewAsIconsRoot;
				else if (m_iState==SHOW_GENRES)
					g_stSettings.m_iMyMusicNavGenresViewAsIcons=m_iViewAsIcons;
				else if (m_iState==SHOW_ARTISTS)
					g_stSettings.m_iMyMusicNavArtistsViewAsIcons=m_iViewAsIcons;
				else if (m_iState==SHOW_ALBUMS)
					g_stSettings.m_iMyMusicNavAlbumsViewAsIcons=m_iViewAsIcons;
				else if (m_iState==SHOW_SONGS)
					g_stSettings.m_iMyMusicNavSongsViewAsIcons=m_iViewAsIcons;

				g_settings.Save();

				return true;
			}
			else if (iControl==CONTROL_BTNSORTASC) // sort asc
			{
				if (m_iState==SHOW_ROOT)
					return true;
				else if (m_iState==SHOW_GENRES)
					g_stSettings.m_bMyMusicNavGenresSortAscending=!g_stSettings.m_bMyMusicNavGenresSortAscending;
				else if (m_iState==SHOW_ARTISTS)
					g_stSettings.m_bMyMusicNavArtistsSortAscending=!g_stSettings.m_bMyMusicNavArtistsSortAscending;
				else if (m_iState==SHOW_ALBUMS)
					g_stSettings.m_bMyMusicNavAlbumsSortAscending=!g_stSettings.m_bMyMusicNavAlbumsSortAscending;
				else if (m_iState==SHOW_SONGS)
					g_stSettings.m_bMyMusicNavSongsSortAscending=!g_stSettings.m_bMyMusicNavSongsSortAscending;
				g_settings.Save();

				UpdateButtons();
				UpdateListControl();

				return true;
			}
			else if (iControl==CONTROL_BTNSHUFFLE) // shuffle?
			{
				// inverse current playlist shuffle state but do not save!
				g_stSettings.m_bMyMusicPlaylistShuffle=!g_playlistPlayer.ShuffledPlay(PLAYLIST_MUSIC_TEMP);
				g_playlistPlayer.ShufflePlay(PLAYLIST_MUSIC_TEMP, g_stSettings.m_bMyMusicPlaylistShuffle);

				UpdateButtons();

				return true;
			}
			else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
			{
				int iItem=GetSelectedItem();
				int iAction=message.GetParam1();

				// use play button to add folders of items to temp playlist
				if (iAction==ACTION_MUSIC_PLAY)
				{
					PlayItem(iItem);
				}
			}
		}
		break;
	}
	return CGUIWindowMusicBase::OnMessage(message);
}

void CGUIWindowMusicNav::GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)
{
	CLog::Log(LOGDEBUG,"CGUIWindowMusicNav::GetDirectory, strGenre = [%s], strArtist = [%s], strAlbum = [%s]",m_strGenre.c_str(),m_strArtist.c_str(),m_strAlbum.c_str());

	// cleanup items
	if (items.size())
	{
		CFileItemList itemlist(items);
	}

	switch (m_iState)
	{
		case SHOW_ROOT:
		{
			m_iViewAsIconsRoot=g_stSettings.m_iMyMusicNavRootViewAsIcons;

			// we're at the zero point
			// add the initial items to the fileitems
			vector<CStdString> vecRoot;
			vecRoot.push_back(g_localizeStrings.Get(135));	// Genres
			vecRoot.push_back(g_localizeStrings.Get(133));  // Artists
			vecRoot.push_back(g_localizeStrings.Get(132));  // Albums
			vecRoot.push_back(g_localizeStrings.Get(134));  // Songs
			for (int i=0; i<(int)vecRoot.size();++i)
			{
				CFileItem* pFileItem = new CFileItem(vecRoot[i]);
				pFileItem->m_strPath = vecRoot[i];
				pFileItem->m_bIsFolder = true;
				items.push_back(pFileItem);
			}
		}
		break;

		case SHOW_GENRES:
		{
			m_iViewAsIcons=g_stSettings.m_iMyMusicNavGenresViewAsIcons;

			// set parent directory
			if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
			{
				CFileItem* pFileItem = new CFileItem("..");
				pFileItem->m_strPath="";
				pFileItem->m_bIsFolder=true;
				items.push_back(pFileItem);
			}
			m_strParentPath = g_localizeStrings.Get(135);

			// get genres from the database
			VECGENRES genres;	
			bool bTest = g_musicDatabase.GetGenres(genres);

			// Display an error message if the database doesn't contain any genres
			DisplayEmptyDatabaseMessage(genres.empty());

			if (bTest)
			{
				// add "All Genres"
				CFileItem* pFileItem = new CFileItem(g_localizeStrings.Get(15105));
				pFileItem->m_strPath="";
				pFileItem->m_bIsFolder=true;
				items.push_back(pFileItem);

				for (int i=0; i < (int)genres.size(); ++i)
				{
					CStdString strGenre= genres[i];
					CFileItem* pFileItem = new CFileItem(strGenre);
					pFileItem->m_strPath=strGenre;
					pFileItem->m_bIsFolder=true;
					items.push_back(pFileItem);
				}
			}
		}
		break;

		case SHOW_ARTISTS:
		{
			m_iViewAsIcons=g_stSettings.m_iMyMusicNavArtistsViewAsIcons;

			// set parent directory
			if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
			{
				CFileItem* pFileItem = new CFileItem("..");
				pFileItem->m_strPath="";
				pFileItem->m_bIsFolder=true;
				items.push_back(pFileItem);
			}
			if (m_iPath >= 8)
				m_strParentPath = m_strGenre;
			else
				m_strParentPath = g_localizeStrings.Get(135);

			// get artists from the database
			VECARTISTS artists;
			bool bTest = g_musicDatabase.GetArtistsNav(artists,m_strGenre);

			// Display an error message if the database doesn't contain any artists
			DisplayEmptyDatabaseMessage(artists.empty());

			if (bTest)
			{
				// add "All Artists"
				CFileItem* pFileItem = new CFileItem(g_localizeStrings.Get(15103));
				pFileItem->m_strPath="";
				pFileItem->m_bIsFolder=true;
				items.push_back(pFileItem);

				for (int i=0; i < (int)artists.size(); ++i)
				{
					CStdString strArtist= artists[i];
					CFileItem* pFileItem = new CFileItem(strArtist);
					pFileItem->m_strPath=strArtist;
					pFileItem->m_bIsFolder=true;
					items.push_back(pFileItem);
				}
			}
		}
		break;

		case SHOW_ALBUMS:
		{
			m_iViewAsIcons=g_stSettings.m_iMyMusicNavAlbumsViewAsIcons;

			// set parent directory
			if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
			{
				CFileItem* pFileItem = new CFileItem("..");
				pFileItem->m_strPath="";
				pFileItem->m_bIsFolder=true;
				items.push_back(pFileItem);
			}
			if (m_iPath >= 4)
				m_strParentPath = m_strArtist;
			else
				m_strParentPath = g_localizeStrings.Get(132);

			//	get albums from the database
			VECALBUMS albums;
			bool bTest = bTest = g_musicDatabase.GetAlbumsNav(albums,m_strGenre,m_strArtist);

			// Display an error message if the database doesn't contain any albums
			DisplayEmptyDatabaseMessage(albums.empty());
			
			if (bTest)
			{
				// add "All Albums"
				CFileItem* pFileItem = new CFileItem(g_localizeStrings.Get(15102));
				pFileItem->m_strPath="";
				pFileItem->m_bIsFolder=true;
				items.push_back(pFileItem);

				for (int i=0; i < (int)albums.size(); ++i)
				{
					CAlbum &album = albums[i];
					CFileItem* pFileItem = new CFileItem(album);
					items.push_back(pFileItem);
				}
			}
		}
		break;

		case SHOW_SONGS:
		{
			m_iViewAsIcons=g_stSettings.m_iMyMusicNavSongsViewAsIcons;

			// set parent directory
			if (!g_guiSettings.GetBool("MusicLists.HideParentDirItems"))
			{
				CFileItem* pFileItem = new CFileItem("..");
				pFileItem->m_strPath="";
				pFileItem->m_bIsFolder=true;
				items.push_back(pFileItem);
			}
			if (m_iPath >= 2)
				m_strParentPath = m_strAlbum;
			else
				m_strParentPath = g_localizeStrings.Get(134);

			//	get songs from the database
			VECSONGS songs;
			bool bTest = g_musicDatabase.GetSongsNav(songs,m_strGenre,m_strArtist,m_strAlbum);

			// Display an error message if the database doesn't contain any albums
			DisplayEmptyDatabaseMessage(songs.empty());

			if (bTest)
			{
				// add "All Songs"
				CFileItem* pFileItem = new CFileItem(g_localizeStrings.Get(15104));
				pFileItem->m_strPath="";
				pFileItem->m_bIsFolder=true;
				items.push_back(pFileItem);

				for (int i=0; i < (int)songs.size(); ++i)
				{
					CSong &song = songs[i];
					CFileItem* pFileItem = new CFileItem(song);
					items.push_back(pFileItem);
				}
			}
		}
		break;
	}
	CLog::Log(LOGDEBUG,"CGUIWindowMusicNav::GetDirectory Done, m_iState = [%i], m_strParentPath = [%s]",m_iState,m_strParentPath.c_str());
}

void CGUIWindowMusicNav::UpdateButtons()
{
	CGUIWindowMusicBase::UpdateButtons();

	// disallow sorting on the root
	if (m_iState==SHOW_ROOT)
	{
		CONTROL_DISABLE(CONTROL_BTNSORTBY);
		CONTROL_DISABLE(CONTROL_BTNSORTASC);
	}
	else
	{
		CONTROL_ENABLE(CONTROL_BTNSORTBY);
		CONTROL_ENABLE(CONTROL_BTNSORTASC);
	}

	//	Update sorting control
	bool bSortAscending=false;
	if (m_iState==SHOW_ROOT)
		bSortAscending=false;
	else if (m_iState==SHOW_GENRES)
		bSortAscending=g_stSettings.m_bMyMusicNavGenresSortAscending;
	else if (m_iState==SHOW_ARTISTS)
		bSortAscending=g_stSettings.m_bMyMusicNavArtistsSortAscending;
	else if (m_iState==SHOW_ALBUMS)
		bSortAscending=g_stSettings.m_bMyMusicNavAlbumsSortAscending;
	else if (m_iState==SHOW_SONGS)
		bSortAscending=g_stSettings.m_bMyMusicNavSongsSortAscending;

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
		if (!pControl->IsVisible())
			CONTROL_SELECT_ITEM(CONTROL_THUMBS,GetSelectedItem());
	pControl=GetControl(CONTROL_LIST);
	if (pControl)
		if (!pControl->IsVisible())
			CONTROL_SELECT_ITEM(CONTROL_LIST,GetSelectedItem());

	SET_CONTROL_HIDDEN(CONTROL_LIST);
	SET_CONTROL_HIDDEN(CONTROL_THUMBS);

	bool bViewIcon = false;
	int iString;

	if (m_iState==SHOW_ROOT) 
	{
		switch (m_iViewAsIconsRoot)
		{
			case VIEW_AS_LIST:
				iString=101; // view as icons
			break;
			
			case VIEW_AS_ICONS:
				iString=100;  // view as large icons
				bViewIcon=true;	
			break;
			
			case VIEW_AS_LARGEICONS:
				iString=417; // view as list
				bViewIcon=true;
			break;
		}
	}
	else 
	{
		switch (m_iViewAsIcons)
		{
			case VIEW_AS_LIST:
				iString=101; // view as icons
			break;
      
			case VIEW_AS_ICONS:
				iString=100;  // view as large icons
				bViewIcon=true;
			break;
			
			case VIEW_AS_LARGEICONS:
				iString=417; // view as list
				bViewIcon=true;
			break;
		}
	}

	if (bViewIcon) 
	{
		SET_CONTROL_VISIBLE(CONTROL_THUMBS);
	}
	else
	{
		SET_CONTROL_VISIBLE(CONTROL_LIST);
	}

	SET_CONTROL_LABEL(CONTROL_BTNVIEWASICONS,iString);

	// Update object count
	int iItems=m_vecItems.size();
	if (iItems)
	{
		CFileItem* pItem=m_vecItems[0];
		if (pItem->GetLabel()=="..") iItems--;
	}
	WCHAR wszText[20];
	const WCHAR* szText=g_localizeStrings.Get(127).c_str();
	swprintf(wszText,L"%i %s", iItems,szText);

	SET_CONTROL_LABEL(CONTROL_LABELFILES,wszText);

	//	Update sort by button
	if (m_iState==SHOW_ROOT||m_iState==SHOW_GENRES||m_iState==SHOW_ARTISTS)
	{
		SET_CONTROL_LABEL(CONTROL_BTNSORTBY,103);
	}
	else if (m_iState==SHOW_ALBUMS)
	{
		SET_CONTROL_LABEL(CONTROL_BTNSORTBY,g_stSettings.m_iMyMusicNavAlbumsSortMethod+263);
	}
	else if (m_iState==SHOW_SONGS)
	{
		SET_CONTROL_LABEL(CONTROL_BTNSORTBY,g_stSettings.m_iMyMusicNavSongsSortMethod+263);
	}

	// make the filter label
	CStdString strLabel = m_strGenre;

	// Append Artist
	if (!strLabel.IsEmpty() && !m_strArtist.IsEmpty())
		strLabel += "/";
	if (!m_strArtist.IsEmpty())
		strLabel += m_strArtist;

	// Append Album
	if (!strLabel.IsEmpty() && !m_strAlbum.IsEmpty())
		strLabel += "/";
	if (!m_strAlbum.IsEmpty())
		strLabel += m_strAlbum;

	SET_CONTROL_LABEL(CONTROL_FILTER, strLabel);

	// Mark the shuffle button
	if (g_playlistPlayer.ShuffledPlay(PLAYLIST_MUSIC_TEMP))
	{
		CONTROL_SELECT(CONTROL_BTNSHUFFLE);
	}
}

void CGUIWindowMusicNav::OnClick(int iItem)
{
	CFileItem* pItem = m_vecItems[iItem];
	CStdString strPath = pItem->m_strPath;
	if (pItem->m_bIsFolder)
	{
		if ( pItem->m_bIsShareOrDrive ) 
		{
			if ( !CGUIPassword::IsItemUnlocked( pItem, "music" ) )
				return;

			if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
				return;
		}
		if (pItem->GetLabel()=="..")
		{
			// go back a directory
			GoParentFolder();

			// GoParentFolder() calls Update(), so just return
			return;
		}
		else
		{
			switch (m_iState)
			{
				//	set state to the new directory
				case SHOW_ROOT:
				{
					if (strPath.Equals("Genres"))
						m_iState = SHOW_GENRES;
					else if (strPath.Equals("Artists"))
						m_iState = SHOW_ARTISTS;
					else if (strPath.Equals("Albums"))
						m_iState = SHOW_ALBUMS;
					else
						m_iState = SHOW_SONGS;
					m_iPath += m_iState;
				}
				break;

				case SHOW_GENRES:
				{
					m_iState = SHOW_ARTISTS;
					m_iPath += m_iState;
					m_strGenre = strPath;

					// clicked on "All Genres" ?
					if (strPath.IsEmpty())
						m_strGenre.Empty();
				}
				break;

				case SHOW_ARTISTS:
				{
					m_iState = SHOW_ALBUMS;
					m_iPath += m_iState;
					m_strArtist = strPath;

					// clicked on "All Artists" ?
					if (strPath.IsEmpty())
						m_strArtist.Empty();
				}
				break;

				case SHOW_ALBUMS:
				{
					m_iState = SHOW_SONGS;
					m_iPath += m_iState;

					// clicked on "All Albums" ?
					if (strPath.IsEmpty())
						m_strAlbum.Empty();
					// no, set the path to the album title
					else
					{
						strPath = pItem->m_musicInfoTag.GetAlbum();
						m_strAlbum = strPath;
					}
				}
				break;
			}
		}
		Update(strPath);
	}
	else
	{
		// play and add current directory to temporary playlist
		int nFolderCount=0;
		g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Clear();
		g_playlistPlayer.Reset();
		for ( int i = 0; i < (int) m_vecItems.size(); i++ ) 
		{
			CFileItem* pItem = m_vecItems[i];
			if ( pItem->m_bIsFolder ) 
			{
				nFolderCount++;
				continue;
			}
			CPlayList::CPlayListItem playlistItem;
			CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
			g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
		}

		//	Save current window and directory to know where the selected item was
		m_nTempPlayListWindow=GetID();
		m_strTempPlayListDirectory=m_Directory.m_strPath;
		if (CUtil::HasSlashAtEnd(m_strTempPlayListDirectory))
			m_strTempPlayListDirectory.Delete(m_strTempPlayListDirectory.size()-1);

		g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC_TEMP);
		g_playlistPlayer.Play(iItem-nFolderCount);
	}
}

void CGUIWindowMusicNav::OnFileItemFormatLabel(CFileItem* pItem)
{
	// clear label for special directories
	if (pItem->m_strPath.IsEmpty())
			pItem->SetLabel2("");
	else if (pItem->m_bIsFolder)
	{
		// for albums, set label2 to the artist name
		if (m_iState==SHOW_ALBUMS)
		{
			// if filtering use the filtered artist name
			if (!m_strArtist.IsEmpty())
				pItem->SetLabel2(m_strArtist);
			else
			{
				// otherwise use the label from the album
				// this may be "Various Artists"
				CStdString strArtist=pItem->m_musicInfoTag.GetArtist();
				pItem->SetLabel2(strArtist);
			}
		}
		// for root, genres, artists, clear label2
		else
			pItem->SetLabel2("");
	}
	else
	{
		// for songs, set the label using user defined format string
		if (pItem->m_musicInfoTag.Loaded())
			SetLabelFromTag(pItem);
	}

	//	set thumbs and default icons
	if (m_iState!=SHOW_ROOT)
		pItem->SetMusicThumb();
	if (pItem->GetIconImage()=="music.jpg")
		pItem->SetThumbnailImage("MyMusic.jpg");
	pItem->FillInDefaultIcon();
}

void CGUIWindowMusicNav::DoSort(VECFILEITEMS& items)
{
	// dont sort the root window
	if (m_iState==SHOW_ROOT)
		return;

	SSortMusicNav sortmethod;
	sortmethod.m_strDirectory=m_Directory.m_strPath;

	if (m_iState==SHOW_GENRES)
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyMusicNavRootSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyMusicNavGenresSortAscending;
	}
	else if (m_iState==SHOW_ARTISTS)
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyMusicNavRootSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyMusicNavArtistsSortAscending;
	}
	else if (m_iState==SHOW_ALBUMS)
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyMusicNavAlbumsSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyMusicNavAlbumsSortAscending;
	}
	else if (m_iState==SHOW_SONGS)
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyMusicNavSongsSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyMusicNavSongsSortAscending;
	}

	sort(items.begin(), items.end(), sortmethod);
}

void CGUIWindowMusicNav::OnSearchItemFound(const CFileItem* pSelItem)
{
	if (pSelItem->m_bIsFolder)
	{
		if (pSelItem->m_musicInfoTag.GetAlbum().IsEmpty())
		{
			m_iState=SHOW_ARTISTS;
			m_iPath=m_iState;
			m_strGenre.Empty();
			m_strAlbum.Empty();
			m_strArtist.Empty();
			Update("");
			for (int i=0; i<(int)m_vecItems.size(); i++)
			{
				CFileItem* pItem=m_vecItems[i];
				if (pItem->m_strPath==pSelItem->m_strPath)
				{
					CONTROL_SELECT_ITEM(CONTROL_LIST, i);
					CONTROL_SELECT_ITEM(CONTROL_THUMBS, i);
					const CGUIControl* pControl=GetControl(CONTROL_LIST);
					if (pControl->IsVisible())
					{
						SET_CONTROL_FOCUS(CONTROL_LIST, 0);
					}
					else
					{
						SET_CONTROL_FOCUS(CONTROL_THUMBS, 0);
					}
					break;
				}
			}
		}
		else
		{
			//	FIXME: various artist albums are not handled
			m_iState=SHOW_ALBUMS;
			m_iPath=m_iState;

			m_strGenre.Empty();
			m_strArtist=pSelItem->m_musicInfoTag.GetArtist();
			m_strAlbum.Empty();

			Update(m_strArtist);

			CStdString strHistory;
			GetDirectoryHistoryString(pSelItem, strHistory);
			m_history.Set(strHistory, m_strArtist);
			m_history.Set(m_strArtist, "");

			for (int i=0; i<(int)m_vecItems.size(); i++)
			{
				CFileItem* pItem=m_vecItems[i];
				if (pItem->m_strPath==pSelItem->m_strPath)
				{
					CONTROL_SELECT_ITEM(CONTROL_LIST, i);
					CONTROL_SELECT_ITEM(CONTROL_THUMBS, i);
					const CGUIControl* pControl=GetControl(CONTROL_LIST);
					if (pControl->IsVisible())
					{
						SET_CONTROL_FOCUS(CONTROL_LIST, 0);
					}
					else
					{
						SET_CONTROL_FOCUS(CONTROL_THUMBS, 0);
					}
					break;
				}
			}
		}
	}
	else
	{
		CStdString strPath;
		CUtil::GetDirectory(pSelItem->m_strPath, strPath);
		
		m_strGenre.Empty();
		m_strArtist=pSelItem->m_musicInfoTag.GetArtist();
		m_strAlbum=pSelItem->m_musicInfoTag.GetAlbum();

		m_iState=SHOW_SONGS;
		m_iPath=m_iState;

		Update(strPath);

		CFileItem parentItem(*pSelItem);
		parentItem.m_bIsFolder=true;
		parentItem.m_strPath=strPath;
		
		CStdString strHistory;
		GetDirectoryHistoryString(&parentItem, strHistory);
		m_history.Set(strHistory, m_strArtist);

		m_history.Set(m_strArtist, "");

		for (int i=0; i<(int)m_vecItems.size(); i++)
		{
			CFileItem* pItem=m_vecItems[i];
			if (pItem->m_strPath==pSelItem->m_strPath)
			{
				CONTROL_SELECT_ITEM(CONTROL_LIST, i);
				CONTROL_SELECT_ITEM(CONTROL_THUMBS, i);
				const CGUIControl* pControl=GetControl(CONTROL_LIST);
				if (pControl->IsVisible())
				{
					SET_CONTROL_FOCUS(CONTROL_LIST, 0);
				}
				else
				{
					SET_CONTROL_FOCUS(CONTROL_THUMBS, 0);
				}
				break;
			}
		}
	}
}

/// \brief Search for songs, artists and albums with search string \e strSearch in the musicdatabase and return the found \e items
/// \param strSearch The search string 
/// \param items Items Found
void CGUIWindowMusicNav::DoSearch(const CStdString& strSearch,VECFILEITEMS& items)
{
	VECARTISTS artists;
	g_musicDatabase.GetArtistsByName(strSearch, artists);

	if (artists.size())
	{
		CStdString strSong=g_localizeStrings.Get(484);	//	Artist
		for (int i=0; i<(int)artists.size(); i++)
		{
			CStdString& strArtist=artists[i];
			CFileItem* pItem=new CFileItem(strArtist);
			pItem->m_strPath=strArtist;
			pItem->m_bIsFolder=true;
			pItem->SetLabel("[" + strSong + "] " + strArtist);
			items.push_back(pItem);
		}
	}

	VECALBUMS albums;
	g_musicDatabase.FindAlbumsByName(strSearch, albums);

	if (albums.size())
	{
		CStdString strAlbum=g_localizeStrings.Get(483);	//	Album
		for (int i=0; i<(int)albums.size(); i++)
		{
			CAlbum& album=albums[i];
			CFileItem* pItem=new CFileItem(album);
			pItem->SetLabel("[" + strAlbum + "] " + album.strAlbum + " - " + album.strArtist);
			items.push_back(pItem);
		}
	}

	VECSONGS songs;
	g_musicDatabase.FindSongsByName(strSearch, songs);

	if (songs.size())
	{
		CStdString strSong=g_localizeStrings.Get(179);	//	Song
		for (int i=0; i<(int)songs.size(); i++)
		{
			CSong& song=songs[i];
			CFileItem* pItem=new CFileItem(song);
			pItem->SetLabel("[" + strSong + "] " + song.strTitle + " - " + song.strArtist + " - " + song.strAlbum);
			items.push_back(pItem);
		}
	}
}

/// \brief Call to go to parent folder
void CGUIWindowMusicNav::GoParentFolder()
{
	// go back a directory
	m_iPath -= m_iState;
	
	// do we go back to albums?
	if (m_iPath & (1<<1))
	{
		m_iState = SHOW_ALBUMS;
		m_strAlbum.Empty();
	}
	// or back to artists?
	else if (m_iPath & (1<<2))
	{
		m_iState = SHOW_ARTISTS;
		m_strAlbum.Empty();
		m_strArtist.Empty();
	}
	// or back to genres?
	else if (m_iPath & (1<<3))
	{
		m_iState = SHOW_GENRES;
		m_strAlbum.Empty();
		m_strArtist.Empty();
		m_strGenre.Empty();
	}
	// or back to the root?
	else
	{
		m_iState = SHOW_ROOT;
		m_strAlbum.Empty();
		m_strArtist.Empty();
		m_strGenre.Empty();
	}
	Update(m_strParentPath);
}

// what is this for??
// i've not touched it because I see no problems
void CGUIWindowMusicNav::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
	/*
	// set history to concat of Genre + Artist + Album
	strHistoryString=m_strGenre+" / "+m_strArtist+" / "+m_strAlbum;
	*/

	// what was the previous item?
	int iState = m_iState;
	int iPath = m_iPath;
	iPath -= iState;
	
	// do we go back to albums?
	if (iPath & (1<<1))
		iState = SHOW_ALBUMS;
	// or back to artists?
	else if (iPath & (1<<2))
		iState = SHOW_ARTISTS;
	// or back to genres?
	else if (iPath & (1<<3))
		iState = SHOW_GENRES;
	else
		iState = SHOW_ROOT;

	switch (iState)
	{
		case SHOW_ROOT:
			strHistoryString = "";
		break;
		case SHOW_GENRES:
			strHistoryString = m_strGenre;
		break;
		case SHOW_ARTISTS:
			strHistoryString = m_strArtist;
		break;
		case SHOW_ALBUMS:
			strHistoryString = m_strAlbum;
		break;
	}
	CLog::Log(LOGDEBUG,"strHistory = [%s]",strHistoryString.c_str());
}

/// \brief Add file or folder and its subfolders to playlist
/// \param pItem The file item to add
void CGUIWindowMusicNav::AddItemToPlayList(const CFileItem* pItem)
{
	// cant do it from the root
	if (m_iState==SHOW_ROOT) return;
	if (pItem->m_bIsFolder)
	{
		// skip ".."
		if (pItem->GetLabel() == "..") return;
		
		// save state
		int iOldState=m_iState;
		int iOldPath=m_iPath;
		CStdString strOldGenre=m_strGenre;
		CStdString strOldArtist=m_strArtist;
		CStdString strOldAlbum=m_strAlbum;
		CStdString strDirectory=m_Directory.m_strPath;
		CStdString strOldParentPath=m_strParentPath;
		int iViewAsIcons=m_iViewAsIcons;

		// update filter with currently selected item
		switch (m_iState)
		{
			case SHOW_GENRES:
			{
				if (pItem->m_strPath.IsEmpty())
					m_strGenre.Empty();
				else
					m_strGenre=pItem->m_strPath;
			}
			break;

			case SHOW_ARTISTS:
			{
				if (pItem->m_strPath.IsEmpty())
					m_strArtist.Empty();
				else
					m_strArtist=pItem->m_strPath;
			}
			break;

			case SHOW_ALBUMS:
			{
				if (pItem->m_strPath.IsEmpty())
					m_strAlbum.Empty();
				else
					m_strAlbum=pItem->m_musicInfoTag.GetAlbum();
			}
			break;
		}

		m_iState=SHOW_SONGS;
		VECFILEITEMS items;
		CFileItemList itemlist(items);
		GetDirectory("Playlist", items);
		DoSort(items);
		for (int i=0; i < (int) items.size(); ++i)
		{
			if (!items[i]->m_strPath.IsEmpty())
				AddItemToPlayList(items[i]);
		}

		// restore old state
		m_iState=iOldState;
		m_iPath=iOldPath;
		m_strGenre=strOldGenre;
		m_strArtist=strOldArtist;
		m_strAlbum=strOldAlbum;
		m_Directory.m_strPath=strDirectory;
		m_strParentPath=strOldParentPath;
		m_iViewAsIcons=iViewAsIcons;
	}
	else
	{
		if (!pItem->IsNFO() && pItem->IsAudio() && !pItem->IsPlayList())
		{
			CPlayList::CPlayListItem playlistItem;
			CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
			g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Add(playlistItem);
		}
	}
}

/// \brief Add file or folder and its subfolders to playlist
/// \param pItem The file item to add
void CGUIWindowMusicNav::AddItemToTempPlayList(const CFileItem* pItem)
{
	if (pItem->m_bIsFolder)
	{
		// save state
		int iOldState=m_iState;
		int iOldPath=m_iPath;
		CStdString strOldGenre=m_strGenre;
		CStdString strOldArtist=m_strArtist;
		CStdString strOldAlbum=m_strAlbum;
		CStdString strDirectory=m_Directory.m_strPath;
		CStdString strOldParentPath=m_strParentPath;
		int iViewAsIcons=m_iViewAsIcons;

		// update filter with currently selected item
		switch (m_iState)
		{
			case SHOW_GENRES:
			{
				if (pItem->m_strPath.IsEmpty())
					m_strGenre.Empty();
				else
					m_strGenre=pItem->m_strPath;
			}
			break;

			case SHOW_ARTISTS:
			{
				if (pItem->m_strPath.IsEmpty())
					m_strArtist.Empty();
				else
					m_strArtist=pItem->m_strPath;
			}
			break;

			case SHOW_ALBUMS:
			{
				if (pItem->m_strPath.IsEmpty())
					m_strAlbum.Empty();
				else
					m_strAlbum=pItem->m_musicInfoTag.GetAlbum();
			}
			break;
		}

		m_iState=SHOW_SONGS;
		VECFILEITEMS items;
		CFileItemList itemlist(items);
		GetDirectory("Playlist", items);
		DoSort(items);
		for (int i=0; i < (int) items.size(); ++i)
		{
			if (!items[i]->m_strPath.IsEmpty())
				AddItemToPlayList(items[i]);
		}

		// restore old state
		m_iState=iOldState;
		m_iPath=iOldPath;
		m_strGenre=strOldGenre;
		m_strArtist=strOldArtist;
		m_strAlbum=strOldAlbum;
		m_Directory.m_strPath=strDirectory;
		m_strParentPath=strOldParentPath;
		m_iViewAsIcons=iViewAsIcons;
	}
	else
	{
		if (!pItem->IsNFO() && pItem->IsAudio() && !pItem->IsPlayList())
		{
			CPlayList::CPlayListItem playlistItem;
			CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
			g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
		}
	}
}


void CGUIWindowMusicNav::PlayItem(int iItem)
{
	// unlike additemtoplaylist, we need to check the items here
	// before calling it since the current playlist will be stopped
	// and cleared!

	// root is not allowed
	if (m_iState==SHOW_ROOT)
		return;

	const CFileItem* pItem=m_vecItems[iItem];
	// if its a folder, build a temp playlist
	if (pItem->m_bIsFolder)
	{
		// skip ".."
		if (pItem->GetLabel() == "..")
			return;
	
		// clear current temp playlist
		g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Clear();
		g_playlistPlayer.Reset();

		// recursively add items to temp playlist
		AddItemToTempPlayList(pItem);

		// play!
		g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC_TEMP);
		if (g_playlistPlayer.ShuffledPlay(PLAYLIST_MUSIC_TEMP))
		{
			// if shuffled dont start on first song
			g_playlistPlayer.SetCurrentSong(0);
			g_playlistPlayer.PlayNext();
		}
		else
			g_playlistPlayer.Play(0);
	}
	// otherwise just play the song
	else
	{
		OnClick(iItem);
	}
}