#include "stdafx.h"
#include "GUIWindowMusicBase.h"
#include "MusicInfoTagLoaderFactory.h"
#include "GUIWindowMusicInfo.h"
#include "FileSystem/HDdirectory.h"
#include "PlayListFactory.h"
#include "Util.h"
#include "PlayListM3U.h"
#include "Application.h"
#include "PlayListPlayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIListControl.h"
#include "FileSystem/DirectoryCache.h"
#include "CDRip/CDDARipper.h"
#include "GUIPassword.h"
#include "AutoSwitch.h"
#include "GUIFontManager.h"

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNTYPE						6
#define CONTROL_BTNSEARCH					8

#define CONTROL_LIST							50
#define CONTROL_THUMBS						51

using namespace MUSIC_GRABBER;
using namespace DIRECTORY;
using namespace PLAYLIST;

int CGUIWindowMusicBase::m_nTempPlayListWindow=0;
CStdString CGUIWindowMusicBase::m_strTempPlayListDirectory="";

CGUIWindowMusicBase::CGUIWindowMusicBase ()
:CGUIWindow(0)
{
	m_nSelectedItem=-1;
	m_iLastControl=-1;
	m_bDisplayEmptyDatabaseMessage=false;
	m_Directory.m_bIsFolder=true;
}

CGUIWindowMusicBase::~CGUIWindowMusicBase ()
{

}

/// \brief Handle actions on window.
/// \param action Action that can be reacted on.
void CGUIWindowMusicBase::OnAction(const CAction& action)
{
	if (action.wID==ACTION_PARENT_DIR)
	{
		GoParentFolder();
		return;
	}

  if (action.wID==ACTION_PREVIOUS_MENU)
  {
		if (!g_application.m_guiDialogMusicScan.IsRunning())
		{
			CUtil::ThumbCacheClear();
			CUtil::RemoveTempFiles();
		}

		m_gWindowManager.ActivateWindow(WINDOW_HOME);
		return;
  }

	if (action.wID==ACTION_SHOW_PLAYLIST)
	{
		m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
		return;
	}

	CGUIWindow::OnAction(action);
}

/*!
	\brief Handle messages on window.
	\param message GUI Message that can be reacted on.
	\return if a message can't be processed, return \e false

	On these messages this class reacts.\n
	When retrieving...
		- #GUI_MSG_PLAYBACK_ENDED\n
			...and...
		- #GUI_MSG_PLAYBACK_STOPPED\n
			...it deselects the current playing item in list/thumb control,
			if we are in a temporary playlist or in playlistwindow
		- #GUI_MSG_PLAYLIST_PLAY_NEXT_PREV\n
			...the next playing item is set in list/thumb control
		- #GUI_MSG_DVDDRIVE_EJECTED_CD\n
			...it will look, if m_strDirectory contains a path from a DVD share.
			If it is, Update() is called with a empty directory.
		- #GUI_MSG_DVDDRIVE_CHANGED_CD\n
			...and m_strDirectory is empty, Update is called to renew icons after
			disc is changed.
		- #GUI_MSG_WINDOW_DEINIT\n
			...the last focused control is saved to m_iLastControl.
		- #GUI_MSG_WINDOW_INIT\n
			...the musicdatabase is opend and the music extensions and shares are set.
			The last focused control is set.
		- #GUI_MSG_CLICKED\n
			... the base class reacts on the following controls:\n
				Buttons:\n
				- #CONTROL_BTNVIEWASICONS - switch between list, thumb and with large items
				- #CONTROL_BTNTYPE - switch between music windows
				- #CONTROL_BTNSEARCH - Search for items\n
				Other Controls:
				- #CONTROL_LIST and #CONTROL_THUMB\n
					Have the following actions in message them clicking on them.
					- #ACTION_QUEUE_ITEM - add selected item to playlist
					- #ACTION_SHOW_INFO - retrieve album info from the internet
					- #ACTION_SELECT_ITEM - Item has been selected. Overwrite OnClick() to react on it
	*/
bool CGUIWindowMusicBase::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
		case GUI_MSG_PLAYBACK_STARTED:
		{
			UpdateButtons();
		}
		break;

		case GUI_MSG_PLAYBACK_ENDED:
		case GUI_MSG_PLAYBACK_STOPPED:
		case GUI_MSG_PLAYLISTPLAYER_STOPPED:
		{
			CStdString strDirectory=m_Directory.m_strPath;
			if (CUtil::HasSlashAtEnd(strDirectory))
				strDirectory.Delete(strDirectory.size()-1);
			if ((m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory==strDirectory)
					|| (GetID()==WINDOW_MUSIC_PLAYLIST) )
			{
				for (int i=0; i < m_vecItems.Size(); ++i)
				{
					CFileItem* pItem=m_vecItems[i];
					if (pItem && pItem->IsSelected())
					{
						pItem->Select(false);
						break;
					}
				}
			}

			UpdateButtons();
		}
		break;

		case GUI_MSG_PLAYLISTPLAYER_STARTED:
		case GUI_MSG_PLAYLISTPLAYER_CHANGED:
		{
			// started playing another song...
			int nCurrentPlaylist=message.GetParam1();
			CStdString strDirectory=m_Directory.m_strPath;
			if (CUtil::HasSlashAtEnd(strDirectory))
				strDirectory.Delete(strDirectory.size()-1);
			if ((nCurrentPlaylist==PLAYLIST_MUSIC_TEMP && m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory==strDirectory )
					|| (GetID()==WINDOW_MUSIC_PLAYLIST && nCurrentPlaylist==PLAYLIST_MUSIC))
			{
				int nCurrentItem=0;
				int nPreviousItem=-1;
				if (message.GetMessage()==GUI_MSG_PLAYLISTPLAYER_STARTED)
				{
					nCurrentItem=message.GetParam2();
				}
				else if (message.GetMessage()==GUI_MSG_PLAYLISTPLAYER_CHANGED)
				{
					nCurrentItem=LOWORD(message.GetParam2());
					nPreviousItem=HIWORD(message.GetParam2());
				}

				int nFolderCount=m_vecItems.GetFolderCount();

				//	is the previous item in this directory
				for (int i=nFolderCount, n=0; i<m_vecItems.Size(); i++)
				{
					CFileItem* pItem=m_vecItems[i];

					if (pItem)
						pItem->Select(false);
				}

				if (nFolderCount+nCurrentItem<m_vecItems.Size())
				{
					for (int i=nFolderCount, n=0; i<m_vecItems.Size(); i++)
					{
						CFileItem* pItem=m_vecItems[i];

						if (pItem)
						{
							if (!pItem->IsPlayList() && !pItem->IsNFO())
								n++;
							if ((n-1)==nCurrentItem)
							{
								pItem->Select(true);
								break;
							}
						}
					}	//	for (int i=nFolderCount, n=0; i<(int)m_vecItems.size(); i++)
				}

			}
		}
		break;

		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		{
			if ( !m_Directory.IsVirtualDirectoryRoot() )
			{
				if (m_Directory.IsCDDA() || m_Directory.IsDVD() || m_Directory.IsISO9660())
				{
					//	Disc has changed and we are inside a DVD Drive share, get out of here :)
					Update("");
				}
			}
			else
			{
				int iItem = GetSelectedItem();
				Update(m_Directory.m_strPath);
				CONTROL_SELECT_ITEM(CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(CONTROL_THUMBS,iItem)
			}
		}
		break;

		case GUI_MSG_DVDDRIVE_CHANGED_CD:
		{
			if (m_Directory.IsVirtualDirectoryRoot())
			{
				int iItem = GetSelectedItem();
				Update(m_Directory.m_strPath);
				CONTROL_SELECT_ITEM(CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(CONTROL_THUMBS,iItem)
			}
		}
		break;

		case GUI_MSG_WINDOW_DEINIT:
		{
			m_nSelectedItem=GetSelectedItem();
			m_iLastControl=GetFocusedControl();
			ClearFileItems();
			g_musicDatabase.Close();
			CSectionLoader::Unload("LIBID3");
			CSectionLoader::Unload("LIBMP4");
		}
		break;

		case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);

			CSectionLoader::Load("LIBID3");
			CSectionLoader::Load("LIBMP4");

			g_musicDatabase.Open();

			m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

			m_rootDir.SetMask(g_stSettings.m_szMyMusicExtensions);
			m_rootDir.SetShares(g_settings.m_vecMyMusicShares);

			if (m_iLastControl>-1)
			{
				SET_CONTROL_FOCUS(m_iLastControl, 0);
			}

			Update(m_Directory.m_strPath);

			if (m_nSelectedItem>-1)
			{
				SetSelectedItem(m_nSelectedItem);
			}
			return true;
		}
		break;

		case GUI_MSG_CLICKED:
		{
			int iControl=message.GetSenderId();
			if (iControl==CONTROL_BTNVIEWASICONS)
			{
				if ( m_Directory.IsVirtualDirectoryRoot() )
				{
					m_iViewAsIconsRoot++;
					if (m_iViewAsIconsRoot > VIEW_AS_LARGEICONS) m_iViewAsIconsRoot=VIEW_AS_LIST;
				}
				else
				{
					m_iViewAsIcons++;
					if (m_iViewAsIcons > VIEW_AS_LARGEICONS) m_iViewAsIcons=VIEW_AS_LIST;
				}
				ShowThumbPanel();
				UpdateButtons();
			}
			else if (iControl==CONTROL_BTNTYPE)
			{
				CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(),CONTROL_BTNTYPE);
				m_gWindowManager.SendMessage(msg);

				int nWindow=WINDOW_MUSIC_FILES+msg.GetParam1();

				if (nWindow==GetID())
					return true;

				g_stSettings.m_iMyMusicStartWindow=nWindow;
				g_settings.Save();
				m_gWindowManager.ActivateWindow(g_stSettings.m_iMyMusicStartWindow);

				CGUIMessage msg2(GUI_MSG_SETFOCUS, g_stSettings.m_iMyMusicStartWindow, CONTROL_BTNTYPE);
				g_graphicsContext.SendMessage(msg2);

				return true;
			}
			else if (iControl==CONTROL_BTNSEARCH)
			{
				OnSearch();
			}
			else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
			{
				int iItem=GetSelectedItem();
				int iAction=message.GetParam1();

				// iItem is checked for validity inside these routines
				if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
				{
					OnQueueItem(iItem);
				}
				else if (iAction==ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
				{
					OnClick(iItem);
				}
				else if (iAction==ACTION_SHOW_INFO)
				{
					OnInfo(iItem);
				}
				else if (iAction==ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
				{
					OnPopupMenu(iItem);
				}
			}
		}

	}
	return CGUIWindow::OnMessage(message);
}

/// \brief Remove items from list/thumb control and \e m_vecItems.
void CGUIWindowMusicBase::ClearFileItems()
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg1);

  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);

	m_vecItems.Clear(); // will clean up everything
}

/// \brief Updates list/thumb control
/// Sets item labels (text and thumbs), sorts items and adds them to the control
void CGUIWindowMusicBase::UpdateListControl()
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg1);

  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);

	//	Cache available album thumbs
	g_directoryCache.InitMusicThumbCache();

	for (int i=0; i < m_vecItems.Size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];

		//	Format label for listcontrol
		//	and set thumb/icon for item
		OnFileItemFormatLabel(pItem);
	}

  g_directoryCache.ClearMusicThumbCache();

	DoSort(m_vecItems);

	ShowThumbPanel();

	for (int i=0; i < m_vecItems.Size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];

    CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg);

    CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg2);
	}
}

/// \brief Returns the selected list/thumb control item
int CGUIWindowMusicBase::GetSelectedItem()
{
	int iControl;

	if ( ViewByIcon() )
	{
		iControl=CONTROL_THUMBS;
	}
	else
		iControl=CONTROL_LIST;

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
  g_graphicsContext.SendMessage(msg);

  int iItem=msg.GetParam1();
	if (iItem >= m_vecItems.Size())
		return -1;
	return iItem;
}

void CGUIWindowMusicBase::SetSelectedItem(int index)
{
  CONTROL_SELECT_ITEM(CONTROL_LIST,   index);
  CONTROL_SELECT_ITEM(CONTROL_THUMBS, index);
}
/// \brief Set window to a specific directory
/// \param strDirectory The directory to be displayed in list/thumb control
void CGUIWindowMusicBase::Update(const CStdString &strDirectory)
{
	// get selected item
	int iItem=GetSelectedItem();
	CStdString strSelectedItem="";
	if (iItem >=0 && iItem < m_vecItems.Size())
	{
		CFileItem* pItem=m_vecItems[iItem];
		if (pItem->GetLabel() != "..")
		{
			GetDirectoryHistoryString(pItem, strSelectedItem);
		}
	}

	m_iLastControl=GetFocusedControl();

	ClearFileItems();

	m_history.Set(strSelectedItem,m_Directory.m_strPath);
	m_Directory.m_strPath=strDirectory;

	GetDirectory(m_Directory.m_strPath, m_vecItems);

	RetrieveMusicInfo();
	
	UpdateListControl();
	UpdateButtons();
  
  strSelectedItem=m_history.Get(m_Directory.m_strPath);

	if (m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST)
	{
		if (ViewByIcon())
		{
			SET_CONTROL_FOCUS(CONTROL_THUMBS, 0);
		}
		else
		{
			SET_CONTROL_FOCUS(CONTROL_LIST, 0);
		}
	}
	ShowThumbPanel();

	int iCurrentPlaylistSong=-1;
	//	Search current playlist item
	CStdString strCurrentDirectory=m_Directory.m_strPath;
	if (CUtil::HasSlashAtEnd(strCurrentDirectory))
		strCurrentDirectory.Delete(strCurrentDirectory.size()-1);
	if ((m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory==strCurrentDirectory && g_application.IsPlayingAudio()
			&& g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC_TEMP)
			|| (GetID()==WINDOW_MUSIC_PLAYLIST && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC && g_application.IsPlayingAudio()) )
	{
		iCurrentPlaylistSong=g_playlistPlayer.GetCurrentSong();
	}

	bool bSelectedFound=false, bCurrentSongFound=false;
	int iSongInDirectory=-1;
	for (int i=0; i < m_vecItems.Size(); ++i)
	{
		CFileItem* pItem=m_vecItems[i];

		//	Update selected item
		if (!bSelectedFound)
		{
			CStdString strHistory;
			GetDirectoryHistoryString(pItem, strHistory);
			if (strHistory==strSelectedItem)
			{
				CONTROL_SELECT_ITEM(CONTROL_LIST,i);
				CONTROL_SELECT_ITEM(CONTROL_THUMBS,i);
				bSelectedFound=true;
			}
		}

		//	synchronize playlist with current directory
		if (!bCurrentSongFound && iCurrentPlaylistSong>-1)
		{
			if (!pItem->m_bIsFolder && !pItem->IsPlayList() && !pItem->IsNFO())
				iSongInDirectory++;
			if (iSongInDirectory==iCurrentPlaylistSong)
			{
				pItem->Select(true);
				bCurrentSongFound=true;
			}
		}

	}
}

/// \brief Call to go to parent folder
void CGUIWindowMusicBase::GoParentFolder()
{
	CStdString strPath(m_strParentPath), strOldPath(m_Directory.m_strPath);
	Update(strPath);

  if(!g_guiSettings.GetBool("FileLists.FullDirectoryHistory"))
    m_history.Remove(strOldPath); //Delete current path

	/*
	if (m_vecItems.size()==0) return;
	CFileItem* pItem=m_vecItems[0];
	if (pItem->m_bIsFolder)
	{
		if (pItem->GetLabel()=="..")
		{
			CStdString strPath=pItem->m_strPath;
			Update(strPath);
		}
	}*/
}

/// \brief Tests if a network/removeable share is available
/// \param strPath Root share to go into
/// \param iDriveType If share is remote, dvd or hd. See: CShare
/// \return If drive is available, returns \e true
/// \todo Handle not connected to a remote share
bool CGUIWindowMusicBase::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
	if ( iDriveType==SHARE_TYPE_DVD )
	{
		CDetectDVDMedia::WaitMediaReady();
		if ( !CDetectDVDMedia::IsDiscInDrive() )
		{
			CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dlg)
      {
			  dlg->SetHeading( 218 );
			  dlg->SetLine( 0, 219 );
			  dlg->SetLine( 1, L"" );
			  dlg->SetLine( 2, L"" );
			  dlg->DoModal( GetID() );
      }
			//	Update listcontrol, maybe share
			//	was selected while disc change
			int iItem = GetSelectedItem();
			Update( m_Directory.m_strPath );
			CONTROL_SELECT_ITEM(CONTROL_LIST,iItem)
			CONTROL_SELECT_ITEM(CONTROL_THUMBS,iItem)
			return false;
		}
	}
	else if (iDriveType==SHARE_TYPE_REMOTE)
	{
		// TODO: Handle not connected to a remote share
		if ( !CUtil::IsEthernetConnected() )
		{
			CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dlg)
      {
			  dlg->SetHeading( 220 );
			  dlg->SetLine( 0, 221 );
			  dlg->SetLine( 1, L"" );
			  dlg->SetLine( 2, L"" );
			  dlg->DoModal( GetID() );
      }
			return false;
		}
	}

	return true;
}

/// \brief Retrieves music info for albums from allmusic.com and displays them in CGUIWindowMusicInfo
/// \param iItem Item in list/thumb control
void CGUIWindowMusicBase::OnInfo(int iItem)
{
	if ( iItem < 0 || iItem >= m_vecItems.Size() ) return;
	CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  CFileItem* pItem;
	pItem=m_vecItems[iItem];
  if (pItem->m_bIsFolder && pItem->GetLabel() == "..") return;

	// show dialog box indicating we're searching the album name
  if (m_dlgProgress)
  {
	  m_dlgProgress->SetHeading(185);
	  m_dlgProgress->SetLine(0,501);
	  m_dlgProgress->SetLine(1,"");
	  m_dlgProgress->SetLine(2,"");
	  m_dlgProgress->StartModal(GetID());
	  m_dlgProgress->Progress();
  }

  CStdString strPath;
	if (pItem->m_bIsFolder)
	{
		strPath=pItem->m_strPath;
		if (CUtil::HasSlashAtEnd(strPath))
			strPath.Delete(strPath.size()-1);
	}
	else
	{
		CUtil::GetDirectory(pItem->m_strPath, strPath);
	}

	//	Try to find an album name for this item.
	//	Only save to database, if album name is found there.
	VECALBUMS albums;
	bool bSaveDb=false;
	bool bSaveDirThumb=false;
	CStdString strLabel=pItem->GetLabel();

	CAlbum album;
	if (pItem->m_musicInfoTag.Loaded())
	{
		CStdString strAlbum=pItem->m_musicInfoTag.GetAlbum();
		if (!strAlbum.IsEmpty())
			strLabel=strAlbum;

		if (g_musicDatabase.GetAlbumsByPath(strPath, albums))
		{
			if (albums.size()==1)
				bSaveDirThumb=true;

			bSaveDb=true;
		}
		else if (!pItem->m_bIsFolder)	//	handle files
		{
			set<CStdString> albums;

			//	Get album names found in directory
			for (int i=0; i<m_vecItems.Size(); i++)
			{
				CFileItem* pItem=m_vecItems[i];
				if (pItem->m_musicInfoTag.Loaded() && !pItem->m_musicInfoTag.GetAlbum().IsEmpty())
				{
					CStdString strAlbum=pItem->m_musicInfoTag.GetAlbum();
					albums.insert(strAlbum);
				}
			}

			//	the only album in this directory?
			if (albums.size()==1)
			{
				CStdString strAlbum=*albums.begin();
				strLabel=strAlbum;
				bSaveDirThumb=true;
			}
		}
	}
	else if (pItem->m_bIsFolder && g_musicDatabase.GetAlbumsByPath(strPath, albums))
	{	//	Normal folder, query database for albums in this directory

		if (albums.size()==1)
		{
			CAlbum& album=albums[0];
			strLabel=album.strAlbum;
			bSaveDirThumb=true;
		}
		else
		{
			//	More then one album is found in this directory
			//	let the user choose
			CGUIDialogSelect *pDlg= (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
      if (pDlg)
      {
				pDlg->SetHeading(181);
				pDlg->Reset();
        pDlg->EnableButton(false);

				for (int i=0; i < (int)albums.size(); ++i)
				{
					CAlbum& album=albums[i];
					pDlg->Add(album.strAlbum);
				}
				pDlg->Sort();
				pDlg->DoModal(GetID());

				// and wait till user selects one
				int iSelectedAlbum= pDlg->GetSelectedLabel();
				if (iSelectedAlbum< 0)
				{
					if (m_dlgProgress) m_dlgProgress->Close();
					return;
				}

				strLabel=pDlg->GetSelectedLabelText();
			}
		}

		bSaveDb=true;
	}
	else if (pItem->m_bIsFolder)
	{
		//	No album name found for folder found in database. Look into
		//	the directory, but don't save albuminfo to database.
		CFileItemList items;
		GetDirectory(strPath, items);
		OnRetrieveMusicInfo(items);

		set<CStdString> albums;

		//	Get album names found in directory
		for (int i=0; i<items.Size(); i++)
		{
			CFileItem* pItem=items[i];
			if (pItem->m_musicInfoTag.Loaded() && !pItem->m_musicInfoTag.GetAlbum().IsEmpty())
			{
				CStdString strAlbum=pItem->m_musicInfoTag.GetAlbum();
				if (!strAlbum.IsEmpty())
					albums.insert(strAlbum);
			}
		}

		//	no album found in folder use the
		//	item label, we may find something?
		if (albums.size()==0)
		{
			if (m_dlgProgress) m_dlgProgress->Close();
			bSaveDirThumb=true;
		}

		if (albums.size()==1)
		{
			CStdString strAlbum=*albums.begin();
			strLabel=strAlbum;
			bSaveDirThumb=true;
		}

		if (albums.size()>1)
		{
			//	More then one album is found in this directory
			//	let the user choose
			CGUIDialogSelect *pDlg= (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
			if (pDlg)
			{
				pDlg->SetHeading(181);
				pDlg->Reset();
				pDlg->EnableButton(false);

				for (set<CStdString>::iterator it=albums.begin(); it != albums.end(); it++)
				{
					CStdString strAlbum=*it;
					pDlg->Add(strAlbum);
				}
				pDlg->Sort();
				pDlg->DoModal(GetID());

				// and wait till user selects one
				int iSelectedAlbum= pDlg->GetSelectedLabel();
				if (iSelectedAlbum< 0)
				{
					if (m_dlgProgress) m_dlgProgress->Close();
					return;
				}

				strLabel=pDlg->GetSelectedLabelText();
			}
		}
	}
	else
	{
		//	single file, not in database
		//	get correct tag parser
		CMusicInfoTagLoaderFactory factory;
		auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
		if (NULL != pLoader.get())
		{
			// get id3tag
			CMusicInfoTag& tag=pItem->m_musicInfoTag;
			if ( pLoader->Load(pItem->m_strPath,tag))
			{
				//	get album
				CStdString strAlbum=tag.GetAlbum();
				if (!strAlbum.IsEmpty())
				{
					strLabel=strAlbum;
				}
			}
		}
	}

	if (m_dlgProgress) m_dlgProgress->Close();

	ShowAlbumInfo(strLabel, strPath, bSaveDb, bSaveDirThumb, false);
}

void CGUIWindowMusicBase::ShowAlbumInfo(const CStdString& strAlbum, const CStdString& strPath, bool bSaveDb, bool bSaveDirThumb, bool bRefresh)
{
	bool bUpdate=false;
	// check cache
	CAlbum albuminfo;
	VECSONGS songs;
	if (!bRefresh && g_musicDatabase.GetAlbumInfo(strAlbum, strPath, albuminfo, songs))
	{
		vector<CMusicSong> vecSongs;
		for (int i=0; i<(int)songs.size(); i++)
		{
			CSong& song=songs[i];

			CMusicSong musicSong(song.iTrack, song.strTitle, song.iDuration);
			vecSongs.push_back(musicSong);
		}

		CMusicAlbumInfo album;
		album.Set(albuminfo);
		album.SetSongs(vecSongs);

		CGUIWindowMusicInfo *pDlgAlbumInfo= (CGUIWindowMusicInfo*)m_gWindowManager.GetWindow(WINDOW_MUSIC_INFO);
    if (pDlgAlbumInfo)
    {
			pDlgAlbumInfo->SetAlbum(album);
			pDlgAlbumInfo->DoModal(GetID());

			if (!pDlgAlbumInfo->NeedRefresh()) return;
			bRefresh=true;
    }
	}

	//	If we are scanning for music info in the background,
	//	other writing access to the database is prohibited.
	CGUIDialogMusicScan* dlgMusicScan = (CGUIDialogMusicScan*)m_gWindowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
	if (dlgMusicScan->IsRunning())
	{
		CGUIDialogOK *pDlg= (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
		if (pDlg)
		{
			pDlg->SetHeading(189);
			pDlg->SetLine(0, 14057);
			pDlg->SetLine(1, "");
			pDlg->SetLine(2, "");
			pDlg->DoModal(GetID());
			return;
		}
	}

	// find album info
	CMusicAlbumInfo album;
	if (FindAlbumInfo(strAlbum, album))
	{
		// download the album info
		bool bLoaded=album.Loaded();
		if ( bLoaded )
		{
			// set album title from musicinfotag, not the one we got from allmusic.com
			album.SetTitle(strAlbum);
			// set path, needed to store album in database
			album.SetAlbumPath(strPath);

			if (bSaveDb)
			{
				CAlbum albuminfo;
				albuminfo.strAlbum  = album.GetTitle();
				albuminfo.strArtist = album.GetArtist();
				albuminfo.strGenre  = album.GetGenre();
				albuminfo.strTones  = album.GetTones();
				albuminfo.strStyles = album.GetStyles();
				albuminfo.strReview = album.GetReview();
				albuminfo.strImage  = album.GetImageURL();
				albuminfo.iRating   = album.GetRating();
				albuminfo.iYear 		= atol( album.GetDateOfRelease().c_str() );
				albuminfo.strPath   = album.GetAlbumPath();

				for (int i=0; i<(int)album.GetNumberOfSongs(); i++)
				{
					CMusicSong musicSong=album.GetSong(i);

					CSong song;
					song.iTrack=musicSong.GetTrack();
					song.strTitle=musicSong.GetSongName();
					song.iDuration=musicSong.GetDuration();

					songs.push_back(song);
				}

				// save to database
				if (bRefresh)
					g_musicDatabase.UpdateAlbumInfo(albuminfo, songs);
				else
					g_musicDatabase.AddAlbumInfo(albuminfo, songs);
			}
			if (m_dlgProgress)
				m_dlgProgress->Close();

			// ok, show album info
			CGUIWindowMusicInfo *pDlgAlbumInfo= (CGUIWindowMusicInfo*)m_gWindowManager.GetWindow(WINDOW_MUSIC_INFO);
      if (pDlgAlbumInfo)
      {
				pDlgAlbumInfo->SetAlbum(album);
				pDlgAlbumInfo->DoModal(GetID());

				//	Save directory thumb
				if (bSaveDirThumb)
				{
					CStdString strThumb;
					CUtil::GetAlbumThumb(album.GetTitle(), album.GetAlbumPath(),strThumb);
					//	Was the download of the album art
					//	from allmusic.com successfull...
					if (CUtil::FileExists(strThumb))
					{
						//	...yes...
						CFileItem item(album.GetAlbumPath(), true);
						if (!item.IsCDDA())
						{
							//	...also save a copy as directory thumb,
							//	if the album isn't located on an audio cd
							CStdString strFolderThumb;
							CUtil::GetAlbumFolderThumb(album.GetAlbumPath(),strFolderThumb);
							::CopyFile(strThumb, strFolderThumb, false);
						}
					}
				}
				if (pDlgAlbumInfo->NeedRefresh())
				{
					ShowAlbumInfo(strAlbum, strPath, bSaveDb, bSaveDirThumb, true);
					return;
				}
      }
			bUpdate=true;
		}
		else
		{
			// failed 2 download album info
			CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
			if (pDlgOK)
			{
				pDlgOK->SetHeading(185);
				pDlgOK->SetLine(0,L"");
				pDlgOK->SetLine(1,500);
				pDlgOK->SetLine(2,L"");
				pDlgOK->DoModal(GetID());
			}
		}
	}

	if (bUpdate)
	{
		int iSelectedItem=GetSelectedItem();
		if (iSelectedItem >= 0 && m_vecItems[iSelectedItem] && m_vecItems[iSelectedItem]->m_bIsFolder)
		{
			//	refresh only the icon of
			//	the current folder 
			m_vecItems[iSelectedItem]->FreeIcons();
			m_vecItems[iSelectedItem]->SetMusicThumb();
			m_vecItems[iSelectedItem]->FillInDefaultIcon();
		}
		else
		{
			//	Refresh all items 
			for (int i=0; i<m_vecItems.Size(); ++i)
			{
				CFileItem* pItem=m_vecItems[i];
				pItem->FreeIcons();
			}

			m_vecItems.SetMusicThumbs();
			m_vecItems.FillInDefaultIcons();
		}

		//	HACK: If we are in files view
		//	autoswitch between list/thumb control
		if (GetID()==WINDOW_MUSIC_FILES && !m_Directory.IsVirtualDirectoryRoot() && g_guiSettings.GetBool("MusicLists.UseAutoSwitching"))
		{
			m_iViewAsIcons = CAutoSwitch::GetView(m_vecItems);

			int iFocusedControl=GetFocusedControl();

			ShowThumbPanel();
			UpdateButtons();

			if (iFocusedControl==CONTROL_LIST || iFocusedControl==CONTROL_THUMBS)
			{
				int iControl = CONTROL_LIST;
				if (m_iViewAsIcons != VIEW_AS_LIST) iControl = CONTROL_THUMBS;
				SET_CONTROL_FOCUS(iControl, 0);
			}
		}
	}

	if (m_dlgProgress)
    m_dlgProgress->Close();

}

/// \brief Can be overwritten to implement an own tag filling function.
/// \param items File items to fill
void CGUIWindowMusicBase::OnRetrieveMusicInfo(CFileItemList& items)
{

}

/// \brief Retrieve tag information for \e m_vecItems
void CGUIWindowMusicBase::RetrieveMusicInfo()
{
	DWORD dwTick=timeGetTime();

	OnRetrieveMusicInfo(m_vecItems);

	dwTick = timeGetTime() - dwTick;
	CStdString strTmp;
	strTmp.Format("RetrieveMusicInfo() took %imsec\n",dwTick);
	OutputDebugString(strTmp.c_str());
}

/// \brief Add selected list/thumb control item to playlist and start playing
/// \param iItem Selected Item in list/thumb control
void CGUIWindowMusicBase::OnQueueItem(int iItem)
{
	if ( iItem < 0 || iItem >= m_vecItems.Size() ) return;
	// add item 2 playlist
	const CFileItem* pItem=m_vecItems[iItem];
	AddItemToPlayList(pItem);

	//move to next item
	CONTROL_SELECT_ITEM(CONTROL_LIST,iItem+1);
	CONTROL_SELECT_ITEM(CONTROL_THUMBS,iItem+1);
	if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() && !g_application.IsPlayingAudio() )
	{
		g_playlistPlayer.Reset();
		g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
		g_playlistPlayer.Play(0);
	}
}

/// \brief Add file or folder and its subfolders to playlist
/// \param pItem The file item to add
void CGUIWindowMusicBase::AddItemToPlayList(const CFileItem* pItem)
{
	if (pItem->m_bIsFolder)
	{
		//	Check if we add a locked share
		if ( pItem->m_bIsShareOrDrive ) 
		{
			CFileItem item=*pItem;
			if ( !CGUIPassword::IsItemUnlocked( &item, "music" ) )
				return;
		}

		// recursive
		if (pItem->GetLabel() == "..") return;
		CStdString strDirectory=m_Directory.m_strPath;
		m_Directory.m_strPath=pItem->m_strPath;
		CFileItemList items;
		GetDirectory(m_Directory.m_strPath, items);
		DoSort(items);
		for (int i=0; i < items.Size(); ++i)
		{
			AddItemToPlayList(items[i]);
		}
		m_Directory.m_strPath=strDirectory;
	}
	else
	{
		if (!pItem->IsNFO() && pItem->IsAudio() && !pItem->IsPlayList())
		{
			CPlayList::CPlayListItem playlistItem;
			CUtil::ConvertFileItemToPlayListItem(pItem, playlistItem);
			g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).Add(playlistItem);
		}
	}
}

/// \brief Make the actual search for the OnSearch function.
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowMusicBase::DoSearch(const CStdString& strSearch,CFileItemList& items)
{

}

/// \brief Search the current directory for a string got from the virtual keyboard
void CGUIWindowMusicBase::OnSearch()
{
	CStdString strSearch;
	if ( !GetKeyboard(strSearch) )
		return;

	strSearch.ToLower();
	if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(194);
	  m_dlgProgress->SetLine(0,strSearch);
	  m_dlgProgress->SetLine(1,L"");
	  m_dlgProgress->SetLine(2,L"");
	  m_dlgProgress->StartModal(GetID());
	  m_dlgProgress->Progress();
  }
	CFileItemList items;
	DoSearch(strSearch, items);

	if (items.Size())
	{
		CGUIDialogSelect* pDlgSelect=(CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
		pDlgSelect->Reset();
		pDlgSelect->SetHeading(283);
		CUtil::SortFileItemsByName(items);

		for (int i=0; i<(int)items.Size(); i++)
		{
			CFileItem* pItem=items[i];
			pDlgSelect->Add(pItem->GetLabel());
		}

		pDlgSelect->DoModal(GetID());

		int iItem=pDlgSelect->GetSelectedLabel();
		if (iItem < 0)
		{
			if (m_dlgProgress) m_dlgProgress->Close();
			return;
		}

		CFileItem* pSelItem=new CFileItem(*items[iItem]);

		OnSearchItemFound(pSelItem);

		delete pSelItem;
		if (m_dlgProgress) m_dlgProgress->Close();
	}
	else
	{
		if (m_dlgProgress) m_dlgProgress->Close();

		CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dlg)
    {
		  dlg->SetHeading( 194 );
		  dlg->SetLine( 0, 284 );
		  dlg->SetLine( 1, L"" );
		  dlg->SetLine( 2, L"" );
		  dlg->DoModal( GetID() );
    }
	}
}

/// \brief Display virtual keyboard
///	\param strInput Set as defaultstring in keyboard and retrieves the input from keyboard
bool CGUIWindowMusicBase::GetKeyboard(CStdString& strInput)
{
	CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
	if (!pKeyboard) return false;
	// setup keyboard
	pKeyboard->CenterWindow();
	pKeyboard->SetText(strInput);
	pKeyboard->DoModal(m_gWindowManager.GetActiveWindow());
	pKeyboard->Close();

	if (pKeyboard->IsDirty())
	{	// have text - update this.
		strInput = pKeyboard->GetText();
		if (strInput.IsEmpty())
			return false;
		return true;
	}
	return false;
}

/// \brief Is thumb or list control visible
/// \return Returns \e true, if thumb control is visible
bool CGUIWindowMusicBase::ViewByIcon()
{
  if ( m_Directory.IsVirtualDirectoryRoot() )
  {
    if (m_iViewAsIconsRoot != VIEW_AS_LIST) return true;
  }
  else
  {
    if (m_iViewAsIcons != VIEW_AS_LIST) return true;
  }
  return false;
}

/// \brief Is thumb control in large icons mode
/// \return Returns \e true, if thumb control is in large icons mode
bool CGUIWindowMusicBase::ViewByLargeIcon()
{
  if ( m_Directory.IsVirtualDirectoryRoot() )
  {
    if (m_iViewAsIconsRoot == VIEW_AS_LARGEICONS) return true;
  }
  else
  {
    if (m_iViewAsIcons== VIEW_AS_LARGEICONS) return true;
  }
  return false;
}

/// \brief Switch thumb control between large and normal icons
void CGUIWindowMusicBase::ShowThumbPanel()
{
  int iItem=GetSelectedItem();
  if ( ViewByLargeIcon() )
  {
    CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
		if (pControl)
		{
      pControl->ShowBigIcons(true);
		}
  }
  else
  {
    CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
		if (pControl)
		{
      pControl->ShowBigIcons(false);
		}
  }
  if (iItem>-1)
  {
    CONTROL_SELECT_ITEM(CONTROL_LIST,iItem);
    CONTROL_SELECT_ITEM(CONTROL_THUMBS,iItem);
  }
}

/// \brief Can be overwritten to build an own history string for \c m_history
/// \param pItem Item to build the history string from
/// \param strHistoryString History string build as return value
void CGUIWindowMusicBase::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
	strHistoryString=pItem->m_strPath;

	if (CUtil::HasSlashAtEnd(strHistoryString))
		strHistoryString.Delete(strHistoryString.size()-1);
}

void CGUIWindowMusicBase::UpdateButtons()
{
	//	Update window selection control

	//	Remove labels from the window selection
	CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_BTNTYPE);
	g_graphicsContext.SendMessage(msg);

	//	Add labels to the window selection
	CStdString strItem=g_localizeStrings.Get(744);	//	Files
	CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_BTNTYPE);
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(132);	//	Album
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(133);	//	Artist
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(135);	//	Genre
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(271);	//	Top 100
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	// new nav window
	strItem=g_localizeStrings.Get(15100);	//	Navigation Window
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	//	Select the current window as default item
	CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, g_stSettings.m_iMyMusicStartWindow-WINDOW_MUSIC_FILES);
}

///	\brief React on the selected search item
///	\param pItem Search result item
void CGUIWindowMusicBase::OnSearchItemFound(const CFileItem* pItem)
{

}

bool CGUIWindowMusicBase::FindAlbumInfo(const CStdString& strAlbum, CMusicAlbumInfo& album)
{
	// quietly return if Internet lookups are disabled
	if (!g_guiSettings.GetBool("Network.EnableInternet")) return false;

	CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);

	// show dialog box indicating we're searching the album
  if (m_dlgProgress)
  {
	  m_dlgProgress->SetHeading(185);
	  m_dlgProgress->SetLine(0,strAlbum);
	  m_dlgProgress->SetLine(1,"");
	  m_dlgProgress->SetLine(2,"");
	  m_dlgProgress->StartModal(GetID());
	  m_dlgProgress->Progress();
  }

  try
  {
	  CMusicInfoScraper scraper;
	  if (scraper.FindAlbuminfo(strAlbum))
	  {
		  // did we found at least 1 album?
		  int iAlbumCount=scraper.GetAlbumCount();
		  if (iAlbumCount >=1)
		  {
			  //yes
			  // if we found more then 1 album, let user choose one
			  int iSelectedAlbum=0;
			  if (iAlbumCount > 1)
			  {
				  //show dialog with all albums found
				  const WCHAR* szText=g_localizeStrings.Get(181).c_str();
				  CGUIDialogSelect *pDlg= (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
          if (pDlg)
          {
				    pDlg->SetHeading(szText);
				    pDlg->Reset();
            pDlg->EnableButton(true);
            pDlg->SetButtonLabel(413); // manual

				    for (int i=0; i < iAlbumCount; ++i)
				    {
					    CMusicAlbumInfo& info = scraper.GetAlbum(i);
					    pDlg->Add(info.GetTitle2());
				    }
				    pDlg->DoModal(GetID());

				    // and wait till user selects one
				    iSelectedAlbum= pDlg->GetSelectedLabel();
				    if (iSelectedAlbum< 0)
					  {
						  if (!pDlg->IsButtonPressed()) return false;
						  CStdString strNewAlbum=strAlbum;
						  if (!GetKeyboard(strNewAlbum)) return false;
						  if (strNewAlbum=="") return false;
						  if (m_dlgProgress)
						  {
							  m_dlgProgress->SetLine(0,strNewAlbum);
							  m_dlgProgress->Progress();
						  }

						  return FindAlbumInfo(strNewAlbum, album);
					  }
          }
			  }

			  // ok, downloading the album info
			  album = scraper.GetAlbum(iSelectedAlbum);

			  if (m_dlgProgress)
			  {
				  m_dlgProgress->SetHeading(185);
				  m_dlgProgress->SetLine(0,album.GetTitle2());
				  m_dlgProgress->SetLine(1,"");
				  m_dlgProgress->SetLine(2,"");
				  m_dlgProgress->Progress();
			  }

			  // download the album info
			  bool bLoaded=album.Loaded();
			  if (!bLoaded)
				  bLoaded=album.Load();

			  return true;
		  }
		  else
		  {
			  // no albums found
			  if (pDlgOK)
			  {
				  pDlgOK->SetHeading(185);
				  pDlgOK->SetLine(0,L"");
				  pDlgOK->SetLine(1,187);
				  pDlgOK->SetLine(2,L"");
				  pDlgOK->DoModal(GetID());
			  }
		  }
	  }
	  else
	  {
		  // unable 2 connect to www.allmusic.com
		  if (pDlgOK)
		  {
			  pDlgOK->SetHeading(185);
			  pDlgOK->SetLine(0,L"");
			  pDlgOK->SetLine(1,499);
			  pDlgOK->SetLine(2,L"");
			  pDlgOK->DoModal(GetID());
		  }
	  }
  }
  catch(...)
  {
    if (m_dlgProgress && m_dlgProgress->IsRunning())
      m_dlgProgress->Close();

    CLog::Log(LOGERROR, "Exception while downloading album info");
  }
	return false;
}

void CGUIWindowMusicBase::DisplayEmptyDatabaseMessage(bool bDisplay)
{
	m_bDisplayEmptyDatabaseMessage = bDisplay;
}

void CGUIWindowMusicBase::Render()
{
	CGUIWindow::Render();
	if (m_bDisplayEmptyDatabaseMessage)
	{
		CGUIListControl *pControl = (CGUIListControl *)GetControl(CONTROL_LIST);
		int iX = pControl->GetXPosition()+pControl->GetWidth()/2;
		int iY = pControl->GetYPosition()+pControl->GetHeight()/2;
		CGUIFont *pFont = g_fontManager.GetFont(pControl->GetFontName());
		if (pFont)
		{
			float fWidth, fHeight;
			CStdStringW wszText = g_localizeStrings.Get(745);	// "No scanned information for this view"
			CStdStringW wszText2 = g_localizeStrings.Get(746); // "Switch back to Files view"
			pFont->GetTextExtent(wszText, &fWidth, &fHeight);
			pFont->DrawText((float)iX, (float)iY-fHeight, 0xffffffff, wszText.c_str(), XBFONT_CENTER_X|XBFONT_CENTER_Y);
			pFont->DrawText((float)iX, (float)iY+fHeight, 0xffffffff, wszText2.c_str(), XBFONT_CENTER_X|XBFONT_CENTER_Y);
		}
	}
}

void CGUIWindowMusicBase::OnPopupMenu(int iItem)
{
	if ( iItem < 0 || iItem >= m_vecItems.Size() ) return;
	// calculate our position
	int iPosX=200;
	int iPosY=100;
	CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
	if (pList)
	{
		iPosX = pList->GetXPosition()+pList->GetWidth()/2;
		iPosY = pList->GetYPosition()+pList->GetHeight()/2;
	}	
	// mark the item
	bool bSelected=m_vecItems[iItem]->IsSelected(); //	item maybe selected (playlistitem)
	m_vecItems[iItem]->Select(true);
	// popup the context menu
	CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
	if (!pMenu) return;
	// clean any buttons not needed
	pMenu->ClearButtons();
	// add the needed buttons
	pMenu->AddButton(13351);	// Music Information
	pMenu->AddButton(13347);	// Queue Item
	pMenu->AddButton(13350);	// Now Playing...
	pMenu->AddButton(137);		// Search...
	if (g_application.m_guiDialogMusicScan.IsRunning())
		pMenu->AddButton(13353);	// Stop Scanning
	else
		pMenu->AddButton(13352);	// Scan Folder to Database
	pMenu->AddButton(600);		// Rip CD Audio
	pMenu->AddButton(5);			// Settings...

  bool bIsGotoParent = m_vecItems[iItem]->GetLabel() == "..";
  //turn of info/queue if the current item is goto parent ..
  if (bIsGotoParent)
  {
		pMenu->EnableButton(1, false);
		pMenu->EnableButton(2, false);
  }
	// turn off Rip CD Audio button if we don't have a CDDA disk in
	CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
	if (!CDetectDVDMedia::IsDiscInDrive() || !pCdInfo || !pCdInfo->IsAudio(1))
		pMenu->EnableButton(6, false);
	// turn off the now playing button if nothing is playing
	if (!g_application.IsPlayingAudio())
		pMenu->EnableButton(3, false);
	// turn off the Scan button if we're not in files view or a internet stream
	if (GetID() != WINDOW_MUSIC_FILES || m_Directory.IsInternetStream())
		pMenu->EnableButton(5, false);
	// position it correctly
	pMenu->SetPosition(iPosX-pMenu->GetWidth()/2, iPosY-pMenu->GetHeight()/2);
	pMenu->DoModal(GetID());
	switch (pMenu->GetButton())
	{
	case 1:	// Music Information
		OnInfo(iItem);
		break;
	case 2:	// Queue Item
		OnQueueItem(iItem);
		break;
	case 3:	// Now Playing...
		m_gWindowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST);
		return;
		break;
	case 4:	// Search
		OnSearch();
		break;
	case 5:	// Scan...
		OnScan();
		break;
	case 6:	// Rip CD...
		OnRipCD();
		break;
	case 7:	// Settings
		m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYMUSIC);
		return;
		break;
	}
	m_vecItems[iItem]->Select(bSelected);
}

void CGUIWindowMusicBase::OnRipCD()
{
	CCdInfo *pCdInfo = CDetectDVDMedia::GetCdInfo();
	if (CDetectDVDMedia::IsDiscInDrive() && pCdInfo && pCdInfo->IsAudio(1))
	{
		if (!g_application.CurrentFileItem().IsCDDA())
		{
			CCDDARipper ripper;
			ripper.RipCD();
		}
		else
		{
			CGUIDialogOK*	pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
			pDlgOK->SetHeading(257); // Error
			pDlgOK->SetLine(0, "Can't rip CD or Track while playing from CD"); // 
			pDlgOK->SetLine(1, ""); // 
			pDlgOK->SetLine(2, "");
			pDlgOK->DoModal(GetID());
		}
	}
}

void CGUIWindowMusicBase::SetLabelFromTag(CFileItem *pItem)
{
	CStdString strFormat = g_guiSettings.GetString("MusicLists.TrackFormat");
	CMusicInfoTag& tag=pItem->m_musicInfoTag;
	int iPos1 = 0;
	int iPos2 = strFormat.Find('%', iPos1);
	CStdString strLabel;
	while (iPos2 >= 0)
	{
		if (iPos2 > iPos1)
			strLabel += strFormat.Mid(iPos1,iPos2-iPos1);
		CStdString str;
		if (strFormat[iPos2+1] == 'N' && tag.GetTrackNumber()>0)
		{	// number
			str.Format("%02.2i",tag.GetTrackNumber());
		}
		else if (strFormat[iPos2+1] == 'A' && tag.GetArtist().size())
		{	// artist
			str = tag.GetArtist();
		}
		else if (strFormat[iPos2+1] == 'T' && tag.GetTitle().size())
		{	// title
			str = tag.GetTitle();
		}
		else if (strFormat[iPos2+1] == 'B' && tag.GetAlbum().size())
		{	// album
			str = tag.GetAlbum();
		}
		else if (strFormat[iPos2+1] == 'G' && tag.GetGenre().size())
		{	// genre
			str = tag.GetGenre();
		}
		else if (strFormat[iPos2+1] == 'Y')
		{	// year
			str = tag.GetYear();
		}
		else if (strFormat[iPos2+1] == 'F')
		{	// filename
			str = CUtil::GetTitleFromPath(pItem->m_strPath);
		}
		else if (strFormat[iPos2+1] == '%')
		{	// %% to print %
			str = '%';
		}
		strLabel+=str;
		iPos1 = iPos2+2;
		iPos2 = strFormat.Find('%', iPos1);
	}
	if (iPos1 < (int)strFormat.size())
		strLabel += strFormat.Right(strFormat.size()-iPos1);
	pItem->SetLabel( strLabel );

	//	set label 2
	int nDuration=tag.GetDuration();
	if (nDuration > 0)
	{
		CUtil::SecondsToHMSString(nDuration, strLabel);
		pItem->SetLabel2(strLabel);
	}
}