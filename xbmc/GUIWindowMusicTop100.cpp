#include "GUIWindowMusicTop100.h"
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

#define CONTROL_LABELFILES        12

#define CONTROL_LIST							50
#define CONTROL_THUMBS						51

CGUIWindowMusicTop100::CGUIWindowMusicTop100(void)
:CGUIWindowMusicBase()
{

}
CGUIWindowMusicTop100::~CGUIWindowMusicTop100(void)
{

}

bool CGUIWindowMusicTop100::OnMessage(CGUIMessage& message)
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
			CGUIWindowMusicBase::OnMessage(message);

			m_bViewAsIconsRoot=g_stSettings.m_bMyMusicTop100ViewAsIcons;

			Update(m_strDirectory);
			CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,m_nSelectedItem);
			CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,m_nSelectedItem);
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
				g_stSettings.m_bMyMusicTop100ViewAsIcons = !g_stSettings.m_bMyMusicTop100ViewAsIcons;
				m_bViewAsIconsRoot=g_stSettings.m_bMyMusicTop100ViewAsIcons;
				g_settings.Save();
				UpdateButtons();
			}
		}
		break;
	}

	return CGUIWindowMusicBase::OnMessage(message);
}
void CGUIWindowMusicTop100::OnAction(const CAction &action)
{
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

void CGUIWindowMusicTop100::GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)
{
	if (items.size() )
	{
		// cleanup items;
		CFileItemList itemlist(items);
	}

	VECSONGS songs;
	m_database.GetTop100(songs);
	for (int i=0; i < (int)songs.size(); ++i)
	{
		CSong & song=songs[i];
		CFileItem* pFileItem = new CFileItem(song.strTitle);
		pFileItem->m_strPath=song.strFileName;
		pFileItem->m_bIsFolder=false;
		pFileItem->m_musicInfoTag.SetAlbum(song.strAlbum);
		pFileItem->m_musicInfoTag.SetArtist(song.strArtist);
		pFileItem->m_musicInfoTag.SetGenre(song.strGenre);
		pFileItem->m_musicInfoTag.SetDuration(song.iDuration);
		pFileItem->m_musicInfoTag.SetTitle(song.strTitle);
		pFileItem->m_musicInfoTag.SetTrackNumber(song.iTrack);
		pFileItem->m_musicInfoTag.SetLoaded(true);
		items.push_back(pFileItem);
	}
}

void CGUIWindowMusicTop100::UpdateButtons()
{
	CONTROL_DISABLE(GetID(), CONTROL_BTNSORTBY);
	CONTROL_DISABLE(GetID(), CONTROL_BTNSORTASC);

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

	int iString=100;
	if ( m_bViewAsIconsRoot ) {
		SET_CONTROL_VISIBLE(GetID(), CONTROL_THUMBS);
		iString=101;
	}
	else {
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

void CGUIWindowMusicTop100::OnClick(int iItem)
{
	CFileItem* pItem=m_vecItems[iItem];

	//	done a search?
	if (pItem->GetLabel()=="..")
	{
		Update("");
		return;
	}

	g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC_TEMP ).Clear();
	for ( int i = 0; i < (int) m_vecItems.size(); i++ ) 
	{
		CFileItem* pItem = m_vecItems[i];

		CPlayList::CPlayListItem playlistItem;
		playlistItem.SetFileName(pItem->m_strPath);
		playlistItem.SetDescription(pItem->GetLabel());
		playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
		g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC_TEMP).Add(playlistItem);
	}

	//	Save current window and directroy to know where the selected item was
	m_nTempPlayListWindow=GetID();
	m_strTempPlayListDirectory=m_strDirectory;

	g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC_TEMP);
	g_playlistPlayer.Play(iItem);
}

void CGUIWindowMusicTop100::OnFileItemFormatLabel(CFileItem* pItem)
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
			if (strArtist.size())
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
					str.Format("%02.2i. %s",iTrack, tag.GetTitle().c_str());
				else 
					str.Format("%s", tag.GetTitle().c_str());
			}
			pItem->SetLabel(str);
		}


		//	set label 2
		int nDuration=tag.GetDuration();
		if (nDuration)
		{
			CUtil::SecondsToHMSString(nDuration, str);
			pItem->SetLabel2(str);
		}
	}
}

void CGUIWindowMusicTop100::DoSort(VECFILEITEMS& items)
{

}
