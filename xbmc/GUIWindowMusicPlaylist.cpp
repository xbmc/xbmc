
#include "stdafx.h"
#include "GUIWindowMusicPlayList.h"
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
#include "filesystem/cddadirectory.h"

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4

#define CONTROL_BTNSHUFFLE				20
#define CONTROL_BTNSAVE						21
#define CONTROL_BTNCLEAR					22

#define CONTROL_BTNPLAY						23
#define CONTROL_BTNNEXT						24
#define CONTROL_BTNPREVIOUS				25

#define CONTROL_BTNREPEAT					26
#define CONTROL_BTNREPEATONE			27

#define CONTROL_LABELFILES        12

#define CONTROL_LIST							50
#define CONTROL_THUMBS						51

CGUIWindowMusicPlayList::CGUIWindowMusicPlayList(void)
:CGUIWindowMusicBase()
{

}

CGUIWindowMusicPlayList::~CGUIWindowMusicPlayList(void)
{

}

bool CGUIWindowMusicPlayList::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_PLAYBACK_STOPPED:
		{
			UpdateButtons();
		}
		break;

		case GUI_MSG_PLAYLIST_CHANGED:
		{
			//	global playlist changed outside playlist window
			Update("");

			if ((m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST) && m_vecItems.size()<=0)
			{
				m_iLastControl=CONTROL_BTNVIEWASICONS;
				SET_CONTROL_FOCUS(GetID(), m_iLastControl, 0);
			}

		}
		break;

    case GUI_MSG_WINDOW_INIT:
		{
			m_iViewAsIconsRoot=g_stSettings.m_iMyMusicPlaylistViewAsIcons;

			CGUIWindowMusicBase::OnMessage(message);

			if (g_playlistPlayer.Repeated(PLAYLIST_MUSIC))
			{
				CONTROL_SELECT(GetID(), CONTROL_BTNREPEAT);
			}

			if (g_playlistPlayer.RepeatedOne(PLAYLIST_MUSIC))
			{
				CONTROL_SELECT(GetID(), CONTROL_BTNREPEATONE);
			}

			if ((m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST) && m_vecItems.size()<=0)
			{
				m_iLastControl=CONTROL_BTNVIEWASICONS;
				SET_CONTROL_FOCUS(GetID(), m_iLastControl, 0);
			}

			if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC)
			{
				int iSong=g_playlistPlayer.GetCurrentSong();
				if (iSong >= 0 && iSong<=(int)m_vecItems.size())
				{
					CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iSong);
					CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iSong);
				}
			}
			return true;
		}
		break;

		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		case GUI_MSG_DVDDRIVE_CHANGED_CD:
			return true;
		break;

		case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();

      if (iControl==CONTROL_BTNVIEWASICONS)
      {
				CGUIWindowMusicBase::OnMessage(message);
				g_stSettings.m_iMyMusicPlaylistViewAsIcons=m_iViewAsIconsRoot;
				g_settings.Save();
				return true;
			}
			else if (iControl==CONTROL_BTNSHUFFLE)
			{
				ShufflePlayList();
			}
			else if (iControl==CONTROL_BTNSAVE)
			{
				SavePlayList();
			}
			else if (iControl==CONTROL_BTNCLEAR)
			{
				ClearPlayList();
			}
			else if (iControl==CONTROL_BTNPLAY)
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
				g_playlistPlayer.Reset();
				g_playlistPlayer.Play(GetSelectedItem());
				UpdateButtons();
			}
			else if (iControl==CONTROL_BTNNEXT)
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
				g_playlistPlayer.PlayNext();
			}
			else if (iControl==CONTROL_BTNPREVIOUS)
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
				g_playlistPlayer.PlayPrevious();
			}
			else if (iControl==CONTROL_BTNREPEAT)
			{
				g_stSettings.m_bMyMusicPlaylistRepeat=!g_stSettings.m_bMyMusicPlaylistRepeat;
				g_playlistPlayer.Repeat(PLAYLIST_MUSIC, g_stSettings.m_bMyMusicPlaylistRepeat);
			}
			else if (iControl==CONTROL_BTNREPEATONE)
			{
				static bool bRepeatOne=false;
				bRepeatOne=!bRepeatOne;
				g_playlistPlayer.RepeatOne(PLAYLIST_MUSIC, bRepeatOne);
			}
		}
		break;

	}
	return CGUIWindowMusicBase::OnMessage(message);
}

void CGUIWindowMusicPlayList::OnAction(const CAction &action)
{
	if (action.wID==ACTION_PARENT_DIR)
	{
		//	Playlist has no parent dirs
		return;
	}
	if (action.wID==ACTION_SHOW_PLAYLIST)
	{
		m_gWindowManager.PreviousWindow();
		return;
	}
  if (action.wID == ACTION_MOVE_ITEM_UP)
  {
    MoveCurrentPlayListItem(ACTION_MOVE_ITEM_UP);
  }
  if (action.wID == ACTION_MOVE_ITEM_DOWN)
  {
    MoveCurrentPlayListItem(ACTION_MOVE_ITEM_DOWN);
  }

	CGUIWindowMusicBase::OnAction(action);
}

void CGUIWindowMusicPlayList::MoveCurrentPlayListItem(int iAction)
{
    int iFocusedControl = GetFocusedControl();
    if (iFocusedControl==CONTROL_THUMBS || iFocusedControl==CONTROL_LIST)
    {
      int iSelected = GetSelectedItem();
      int iNew      = iSelected;
      if (iAction == ACTION_MOVE_ITEM_UP) {
        iNew--;
      }
      else {
        iNew++;
      }
      //	The current playing or target song can't be moved
      if (
        (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC) && 
        (g_application.IsPlayingAudio()) && 
        (
          (g_playlistPlayer.GetCurrentSong() == iSelected) ||
          (g_playlistPlayer.GetCurrentSong() == iNew)
         )
      ) {
        return;
      }
      CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
      if (playlist.Swap(iSelected, iNew))
      {
        Update(m_strDirectory);
        SetSelectedItem(iNew);
        return;
      }
    }
}


void CGUIWindowMusicPlayList::GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)
{
	if (items.size()) 
	{
		CFileItemList itemlist(items); // will clean up everything
	}

	CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
	/* copy playlist from general playlist*/
	int iCurrentSong=-1;
	if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
		iCurrentSong=g_playlistPlayer.GetCurrentSong();

	CStdString strPath,strFileName;
	m_vecItems.reserve(playlist.size());

	for (int i=0; i < playlist.size(); ++i)
	{
		const CPlayList::CPlayListItem& item = playlist[i];

		CStdString strFileName   = item.GetFileName();
		CStdString strPath,strFName;
		CUtil::Split( strFileName, strPath, strFName);
		m_Pathes.insert(strPath);
		
		CFileItem *pItem = new CFileItem(item);
/*
		if (item.GetDuration())
		{
			int nDuration=item.GetDuration();
			if (nDuration > 0)
			{
				CStdString str;
				CUtil::SecondsToHMSString(nDuration, str);
				pItem->SetLabel2(str);
			}
			else
				pItem->SetLabel2("");
		}*/
		items.push_back(pItem);
	}
}

void CGUIWindowMusicPlayList::SavePlayList()
{
	CStdString strNewFileName;
	if (GetKeyboard(strNewFileName))
	{
		// need 2 rename it
		CStdString strPath=g_stSettings.m_szAlbumDirectory;
		strPath+="\\playlists\\";

		CUtil::RemoveIllegalChars( strNewFileName );
		strPath += strNewFileName;
		strPath+=".m3u";
		CPlayListM3U playlist;
		for (int i=0; i < (int)m_vecItems.size(); ++i)
		{
			CFileItem* pItem = m_vecItems[i];
			CPlayList::CPlayListItem newItem;
			newItem.SetFileName(pItem->m_strPath);
			newItem.SetDescription(pItem->GetLabel());
			if (pItem->m_musicInfoTag.Loaded())
				newItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
			else
				newItem.SetDuration(0);
			playlist.Add(newItem);
		}
		playlist.Save(strPath);
	}
}

void CGUIWindowMusicPlayList::ClearPlayList()
{
	ClearFileItems();
	g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Clear();
	if (g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC)
	{
			g_playlistPlayer.Reset();
			g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
	}
	UpdateListControl();
	UpdateButtons();
	SET_CONTROL_FOCUS(GetID(), CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowMusicPlayList::ShufflePlayList()
{
	ClearFileItems();
	CPlayList& playlist=g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
	
	CStdString strFileName;
	if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC)
	{
		const CPlayList::CPlayListItem& item=playlist[g_playlistPlayer.GetCurrentSong()];
		strFileName=item.GetFileName();
	}
	playlist.Shuffle();
	if (g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC)
		g_playlistPlayer.Reset();

	if (!strFileName.IsEmpty()) 
	{
		for (int i=0; i < playlist.size(); i++)
		{
			const CPlayList::CPlayListItem& item=playlist[i];
			if (item.GetFileName()==strFileName)
				g_playlistPlayer.SetCurrentSong(i);
		}
	}

	Update(m_strDirectory);
}

void CGUIWindowMusicPlayList::RemovePlayListItem(int iItem)
{
	//	The current playing song can't be removed
	if (g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC && g_application.IsPlayingAudio()
			&& g_playlistPlayer.GetCurrentSong()==iItem)
			return;

	g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).Remove(iItem);

	//	Correct the current playing song in playlistplayer
	if (g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC && g_application.IsPlayingAudio())
	{
		int iCurrentSong=g_playlistPlayer.GetCurrentSong();
		if (iItem<=iCurrentSong)
		{
			iCurrentSong--;
			g_playlistPlayer.SetCurrentSong(iCurrentSong);
		}
	}

	int iCount=0;
	ivecItems it=m_vecItems.begin();
	while (it!=m_vecItems.end())
	{
		if (iCount==iItem)
		{
			m_vecItems.erase(it);
			break;
		}
		
		++it;
		iCount++;
	}

	UpdateListControl();
	UpdateButtons();

	if (m_vecItems.size()<=0)
	{
		SET_CONTROL_FOCUS(GetID(), CONTROL_BTNVIEWASICONS, 0);
	}
	else
	{
		CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem-1)
		CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem-1)
	}
}

void CGUIWindowMusicPlayList::UpdateButtons()
{
	//	Update playlist buttons
	if (m_vecItems.size() )
	{
		CONTROL_ENABLE(GetID(), CONTROL_BTNSHUFFLE);
		CONTROL_ENABLE(GetID(), CONTROL_BTNSAVE);
		CONTROL_ENABLE(GetID(), CONTROL_BTNCLEAR);
		CONTROL_ENABLE(GetID(), CONTROL_BTNREPEAT);
		CONTROL_ENABLE(GetID(), CONTROL_BTNREPEATONE);
		CONTROL_ENABLE(GetID(), CONTROL_BTNPLAY);

		if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC)
		{
			CONTROL_ENABLE(GetID(), CONTROL_BTNNEXT);
			CONTROL_ENABLE(GetID(), CONTROL_BTNPREVIOUS);
		}
		else
		{
			CONTROL_DISABLE(GetID(), CONTROL_BTNNEXT);
			CONTROL_DISABLE(GetID(), CONTROL_BTNPREVIOUS);
		}
	}
	else
	{
		CONTROL_DISABLE(GetID(), CONTROL_BTNSHUFFLE);
		CONTROL_DISABLE(GetID(), CONTROL_BTNSAVE);
		CONTROL_DISABLE(GetID(), CONTROL_BTNCLEAR);
		CONTROL_DISABLE(GetID(), CONTROL_BTNREPEAT);
		CONTROL_DISABLE(GetID(), CONTROL_BTNREPEATONE);
		CONTROL_DISABLE(GetID(), CONTROL_BTNPLAY);
		CONTROL_DISABLE(GetID(), CONTROL_BTNNEXT);
		CONTROL_DISABLE(GetID(), CONTROL_BTNPREVIOUS);
	}

	//	Update listcontrol and and view by icon/list button
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
}

void CGUIWindowMusicPlayList::OnClick(int iItem)
{
	CFileItem* pItem=m_vecItems[iItem];
	CStdString strPath=pItem->m_strPath;
	g_playlistPlayer.SetCurrentPlaylist( PLAYLIST_MUSIC );
	g_playlistPlayer.Reset();
 	g_playlistPlayer.Play( iItem );
}

void CGUIWindowMusicPlayList::OnQueueItem(int iItem)
{
	RemovePlayListItem(iItem);
}

void CGUIWindowMusicPlayList::OnFileItemFormatLabel(CFileItem* pItem)
{
	if (pItem->m_musicInfoTag.Loaded())
	{
		//	set label 1
		CStdString str;
		CMusicInfoTag& tag=pItem->m_musicInfoTag;
		CStdString strTitle=tag.GetTitle();
		CStdString strArtist=tag.GetArtist();
		if (strTitle.size()) 
		{
			if (strArtist)
			{
				//int iTrack=tag.GetTrackNumber();
				int iTrack = (int)m_lPlayListSeq;
				if (iTrack>0 && !g_stSettings.m_bMyMusicHideTrackNumber)
					str.Format("%02.2i. %s - %s",iTrack, tag.GetArtist().c_str(), tag.GetTitle().c_str());
				else 
					str.Format("%s - %s", tag.GetArtist().c_str(), tag.GetTitle().c_str());
			}
			else
			{
				//int iTrack=tag.GetTrackNumber();
				int iTrack = (int)m_lPlayListSeq;
				if (iTrack>0 && !g_stSettings.m_bMyMusicHideTrackNumber)
					str.Format("%02.2i. %s ",iTrack, tag.GetTitle().c_str());
				else 
					str.Format("%s", tag.GetTitle().c_str());
			}
			pItem->SetLabel(str);
		}

		//	set label 2
		int nDuration=tag.GetDuration();
		if (nDuration > 0)
		{
			CUtil::SecondsToHMSString(nDuration, str);
			pItem->SetLabel2(str);
		}
	}	//	if (pItem->m_musicInfoTag.Loaded())
	else
	{
		//	If we have a cdda track without cddb information,...
		if (!pItem->m_musicInfoTag.Loaded() && CUtil::IsCDDA(pItem->m_strPath) )
		{
			//	...we have the duration for display
			int nDuration=pItem->m_musicInfoTag.GetDuration();
			if (nDuration > 0)
			{
				CStdString str;
				CUtil::SecondsToHMSString(nDuration, str);
				pItem->SetLabel2(str);
			}
		}
		else if (pItem->GetLabel() == "") // pls labels come in preformatted
		{
			// No music info and it's not CDDA so we'll just show the filename
			CStdString str;
			str = CUtil::GetTitleFromPath(pItem->m_strPath);
			str.Format("%02.2i. %s ", m_lPlayListSeq, str);
			pItem->SetLabel(str);
		}
	}
	//	set thumbs and default icons
	CUtil::SetMusicThumb(pItem);
	CUtil::FillInDefaultIcon(pItem);
}

void CGUIWindowMusicPlayList::DoSort(VECFILEITEMS& items)
{

}

void CGUIWindowMusicPlayList::OnRetrieveMusicInfo(VECFILEITEMS& items)
{
	if (items.size()<=0)
		return;

	m_strPrevPath.Empty();

	bool bShowProgress=false;
	bool bProgressVisible=false;

	//	Show a progress dialog when playlist isn't loaded
	//	after 1.5 secs 
	if (!m_gWindowManager.IsRouted())
		bShowProgress=true;

	DWORD dwTick=timeGetTime();

	CSong song;
	for (int i=0; i<(int)items.size(); i++)
	{
		CFileItem* pItem=items[i];

		CStdString strExtension;
		CUtil::GetExtension(pItem->m_strPath,strExtension);

		//	Should we init a progress dialog
		if (bShowProgress && !bProgressVisible)
		{
			DWORD dwElapsed = timeGetTime() - dwTick;

			//	if tag loading took more then 1.5 secs. till now
			//	show the progress dialog 
			if (dwElapsed>1500)
			{
				if (m_dlgProgress) 
				{
					m_dlgProgress->SetHeading(189);
					m_dlgProgress->SetLine(0, 505);
					m_dlgProgress->SetLine(1,"");
					m_dlgProgress->SetLine(2,"");
					m_dlgProgress->StartModal(GetID());
					m_dlgProgress->ShowProgressBar(true);
					m_dlgProgress->SetPercentage((i*100)/m_vecItems.size());
					m_dlgProgress->Progress();
					bProgressVisible=true;
				}
			}
		}		

		if (bProgressVisible && (i%10)==0 && i>0)
		{
			m_dlgProgress->SetPercentage((i*100)/items.size());
			m_dlgProgress->Progress();
		}

		//	Progress key presses from controller or remote
		if (bProgressVisible)
			if (m_dlgProgress) m_dlgProgress->ProgressKeys();

		//	Canceled by the user, finish
		if (bProgressVisible && m_dlgProgress->IsCanceled())
		{
			if (m_dlgProgress) m_dlgProgress->Close();
			return;
		}

		LoadItem(pItem);

	}

	m_songsMap.erase(m_songsMap.begin(),m_songsMap.end());

	if (bShowProgress)
	{
		if (m_dlgProgress) m_dlgProgress->Close();
		return;
	}
}


void CGUIWindowMusicPlayList::LoadItem(CFileItem* pItem)
{
        CMusicInfoTag& tag=pItem->m_musicInfoTag;
	if (tag.Loaded())
            return;	// nothing to do here
	
        CStdString strFileName, strPath;
	CUtil::Split(pItem->m_strPath, strPath, strFileName);

	if (CUtil::IsCDDA(pItem->m_strPath))
	{
		//	We have cdda item...
		VECFILEITEMS  items;
		CCDDADirectory dir;
		//	... use the directory of the cd to 
		//	get its cddb information...
		if (dir.GetDirectory("D:",items))
		{
			for (int i=0; i < (int)items.size(); ++i)
			{
				CFileItem* pCDDAItem=items[i];
				if (pCDDAItem->m_strPath==pItem->m_strPath)
				{
					//	...and find current track to use
					//	cddb information for display.
					pItem->m_musicInfoTag=pCDDAItem->m_musicInfoTag;
				}
			}
		}
		{
			CFileItemList itemlist(items);	//	cleanup everything
		}
	}
	else
	{
		//	Have we loaded this item from database before
		IMAPSONGS it=m_songsMap.find(pItem->m_strPath);
		if (it!=m_songsMap.end())
		{
			CSong& song=it->second;
			pItem->m_musicInfoTag.SetSong(song);
			m_songsMap.erase(it);
		}
		else if (strPath!=m_strPrevPath)
		{
			//	The item is from another directory then the last one,
			//	query the database for the new directory...
			g_musicDatabase.GetSongsByPath(strPath, m_songsMap, true);

			//	...and look if we find it
			IMAPSONGS it=m_songsMap.find(pItem->m_strPath);
			if (it!=m_songsMap.end())
			{
				CSong& song=it->second;
				pItem->m_musicInfoTag.SetSong(song);
				m_songsMap.erase(it);
			}
		}

		//	Nothing found, load tag from file
		if (g_stSettings.m_bUseID3 && !pItem->m_musicInfoTag.Loaded())
		{
			// get correct tag parser
			CMusicInfoTagLoaderFactory factory;
			auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
			if (NULL != pLoader.get())
					// get id3tag
				pLoader->Load(pItem->m_strPath,pItem->m_musicInfoTag);
		}
		else
		{	// no ID3 tag stuff, so get the title from the current label (to remove extensions etc.)
			pItem->SetLabel(CUtil::GetTitleFromPath(pItem->GetLabel()));
		}
	}

	m_strPrevPath=strPath;
}

