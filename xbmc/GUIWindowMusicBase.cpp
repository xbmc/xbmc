#include "guiwindowmusicbase.h"
#include "settings.h"
#include "sectionloader.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "GUIDialogSelect.h"
#include "utils/MusicInfoScraper.h"
#include "musicInfoTagLoaderFactory.h"
#include "GUIWindowMusicInfo.h"
#include "GUIDialogOK.h"
#include "filesystem/HDdirectory.h"
#include "PlayListFactory.h"
#include "util.h"
#include "url.h"
#include "keyboard/virtualkeyboard.h"
#include "PlayListM3U.h"
#include "application.h"
#include "playlistplayer.h"
#include <algorithm>
#include "GuiUserMessages.h"

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
	m_nSelectedItem=0;
}

CGUIWindowMusicBase::~CGUIWindowMusicBase ()
{

}

bool CGUIWindowMusicBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_PLAYBACK_ENDED:
		case GUI_MSG_PLAYBACK_STOPPED:
		{
			if ((m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory.Find(m_strDirectory) > -1)
					|| (GetID()==WINDOW_MUSIC_PLAYLIST && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC))
			{
				for (int i=0; i < (int)m_vecItems.size(); ++i)
				{
					CFileItem* pItem=m_vecItems[i];
					if (pItem && pItem->IsSelected())
					{
						pItem->Select(false);
						break;
					}
				}
			}
		}
		break;

		case GUI_MSG_PLAYLIST_PLAY_NEXT_PREV:
		{
			// started playing another song...
			int nCurrentPlaylist=message.GetParam1();
			if ((nCurrentPlaylist==PLAYLIST_MUSIC_TEMP && m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory.Find(m_strDirectory) > -1 )
					|| (GetID()==WINDOW_MUSIC_PLAYLIST && nCurrentPlaylist==PLAYLIST_MUSIC))
			{
				int nCurrentItem=LOWORD(message.GetParam2());
				int nPreviousItem=(int)HIWORD(message.GetParam2());

				int nFolderCount=0;
				for (int i=0; i < (int)m_vecItems.size(); ++i)
				{
					CFileItem* pItem=m_vecItems[i];
					if (pItem && pItem->m_bIsFolder)
					{
						nFolderCount++;
					}
					else
						break;
				}

				//	is the previous item in this directory
				if (nFolderCount+nPreviousItem<(int)m_vecItems.size())
				{
					CFileItem* pItem=m_vecItems[nFolderCount+nPreviousItem];
					if (pItem)
						pItem->Select(false);
				}

				CFileItem* pItem=m_vecItems[nFolderCount+nCurrentItem];
				if (pItem)
					pItem->Select(true);
			}

		}
		break;

		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		{
			if ( !m_strDirectory.IsEmpty() ) 
			{
				if (CUtil::IsCDDA(m_strDirectory) || CUtil::IsDVD(m_strDirectory) || CUtil::IsISO9660(m_strDirectory)) 
				{
					//	Disc has changed and we are inside a DVD Drive share, get out of here :)
					Update("");
				}
			}
			else 
			{
				int iItem = GetSelectedItem();
				Update(m_strDirectory);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			}
		}
		break;

		case GUI_MSG_DVDDRIVE_CHANGED_CD:
		{
			if (m_strDirectory.IsEmpty()) 
			{
				int iItem = GetSelectedItem();
				Update(m_strDirectory);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			}
		}
		break;

    case GUI_MSG_WINDOW_DEINIT:
		{
      ClearFileItems();
			CSectionLoader::Unload("LIBID3");
			m_database.Close();
			CUtil::RemoveTempFiles();
		}
		break;

    case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);

			CSectionLoader::Load("LIBID3");

			m_database.Open();

			m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(101);

			m_rootDir.SetMask(g_stSettings.m_szMyMusicExtensions);
			m_rootDir.SetShares(g_settings.m_vecMyMusicShares);

			return true;
		}
		break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
			if (iControl==CONTROL_BTNTYPE)
			{
				g_stSettings.m_iMyMusicStartWindow++;
				if (g_stSettings.m_iMyMusicStartWindow>WINDOW_MUSIC_TOP100) g_stSettings.m_iMyMusicStartWindow=WINDOW_MUSIC_FILES;
				g_settings.Save();

				m_gWindowManager.ActivateWindow(g_stSettings.m_iMyMusicStartWindow);
				SET_CONTROL_FOCUS(g_stSettings.m_iMyMusicStartWindow, CONTROL_BTNTYPE);
			}
			else if (iControl==CONTROL_BTNSEARCH)
			{
				OnSearch();
			}
      else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
      {
        int iItem=GetSelectedItem();
				int iAction=message.GetParam1();

				if (iAction == ACTION_QUEUE_ITEM)
        {
					OnQueueItem(iItem);
        }
        else if (iAction==ACTION_SHOW_INFO)
        {
          OnInfo(iItem);
        }
        else if (iAction==ACTION_SELECT_ITEM)
        {
          OnClick(iItem);
        }
      }
    }
	}

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowMusicBase::ClearFileItems()
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg1);         

  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

	CFileItemList itemlist(m_vecItems); // will clean up everything
}


void CGUIWindowMusicBase::UpdateListControl()
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg1);         

  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

	for (int i=0; i < (int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];

		OnFileItemFormatLabel(pItem);
	}

	DoSort(m_vecItems);

	for (int i=0; i < (int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];

    CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg);

    CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg2);
	}
}

int CGUIWindowMusicBase::GetSelectedItem()
{
	int iControl;
	bool bViewAsIcons = false;
	if ( m_strDirectory.IsEmpty() )
		bViewAsIcons = m_bViewAsIconsRoot;
	else
		bViewAsIcons = m_bViewAsIcons;

	if ( bViewAsIcons ) 
		iControl=CONTROL_THUMBS;
	else
		iControl=CONTROL_LIST;

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
  g_graphicsContext.SendMessage(msg);

  int iItem=msg.GetParam1();
	return iItem;
}

void CGUIWindowMusicBase::Update(const CStdString &strDirectory)
{
	// get selected item
	int iItem=GetSelectedItem();
	CStdString strSelectedItem="";
	if (iItem >=0 && iItem < (int)m_vecItems.size())
	{
		CFileItem* pItem=m_vecItems[iItem];
		if (pItem->m_bIsFolder && pItem->GetLabel() != "..")
		{
			strSelectedItem=pItem->m_strPath;
		}
	}

	ClearFileItems();

	m_history.Set(strSelectedItem,m_strDirectory);
	m_strDirectory=strDirectory;
	GetDirectory(m_strDirectory, m_vecItems);

	RetrieveMusicInfo();
  UpdateListControl();
	UpdateButtons();

	strSelectedItem=m_history.Get(m_strDirectory);

	bool bViewAsIcons = false;
	if ( m_strDirectory.IsEmpty() )
		bViewAsIcons = m_bViewAsIconsRoot;
	else
		bViewAsIcons = m_bViewAsIcons;

	if ( bViewAsIcons ) {	
		SET_CONTROL_FOCUS(GetID(), CONTROL_THUMBS);
	}
	else {
		SET_CONTROL_FOCUS(GetID(), CONTROL_LIST);
	}

	CUtil::SetThumbs(m_vecItems);
	CUtil::FillInDefaultIcons(m_vecItems);

	CStdString strFileName;
	//	Search current playlist item
	if ((m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory.Find(m_strDirectory) > -1 && g_application.IsPlayingAudio() 
			&& g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC_TEMP) 
			|| (GetID()==WINDOW_MUSIC_PLAYLIST && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_MUSIC && g_application.IsPlayingAudio()) )
	{
		int iCurrentSong=g_playlistPlayer.GetCurrentSong();
    if (iCurrentSong>=0)
    {
		  CPlayList& playlist=g_playlistPlayer.GetPlaylist(g_playlistPlayer.GetCurrentPlaylist());
      if (iCurrentSong < playlist.size())
      {
		    const CPlayList::CPlayListItem& item=playlist[iCurrentSong];
		    strFileName = item.GetFileName();
			}
		}
	}

	bool bSelectedFound=false;
	for (int i=0; i < (int)m_vecItems.size(); ++i)
	{
		CFileItem* pItem=m_vecItems[i];

		//	Update selected item
		if (!bSelectedFound && pItem->m_strPath==strSelectedItem)
		{
			CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,i);
			CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,i);
			bSelectedFound=true;
		}

		//	Set album thumb
		CStdString strAlbum=pItem->m_musicInfoTag.GetAlbum();
		if (strAlbum !="")
		{
			CStdString strThumb;
			CUtil::GetAlbumThumb(strAlbum,strThumb);
			if (CUtil::FileExists(strThumb) )
			{
				pItem->SetIconImage(strThumb);
				pItem->SetThumbnailImage(strThumb);
			}
			else
			{
				pItem->SetIconImage("music.jpg");
			}
		}

		//	syncronize playlist with current directory
		if (!strFileName.IsEmpty() && pItem->m_strPath == strFileName)
		{
			pItem->Select(true);
		}
	}
}

void CGUIWindowMusicBase::GoParentFolder()
{
	if (m_vecItems.size()==0) return;
	CFileItem* pItem=m_vecItems[0];
	if (pItem->m_bIsFolder)
	{
		if (pItem->GetLabel()=="..")
		{
			CStdString strPath=pItem->m_strPath;
			Update(strPath);
		}
	}
}

bool CGUIWindowMusicBase::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
	if ( iDriveType==SHARE_TYPE_DVD ) 
	{
		CDetectDVDMedia::WaitMediaReady();
		if ( !CDetectDVDMedia::IsDiscInDrive() ) 
		{
			CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(2002);
			dlg->SetHeading( 218 );
			dlg->SetLine( 0, 219 );
			dlg->SetLine( 1, L"" );
			dlg->SetLine( 2, L"" );
			dlg->DoModal( GetID() );
			//	Update listcontrol, maybe share 
			//	was selected while disc change
			int iItem = GetSelectedItem();
			Update( m_strDirectory );
			CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
			CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			return false;
		}
	}
	else if (iDriveType==SHARE_TYPE_REMOTE) 
	{
		// TODO: Handle not connected to a remote share
		if ( !CUtil::IsEthernetConnected() ) 
		{
			CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(2002);
			dlg->SetHeading( 220 );
			dlg->SetLine( 0, 221 );
			dlg->SetLine( 1, L"" );
			dlg->SetLine( 2, L"" );
			dlg->DoModal( GetID() );
			return false;
		}
	}

	return true;
}

void CGUIWindowMusicBase::OnInfo(int iItem)
{
	int iSelectedItem=GetSelectedItem();
	bool bUpdate=false;
	CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(2002);
  CFileItem* pItem;
	pItem=m_vecItems[iItem];

  CStdString strPath=pItem->m_strPath;
	CMusicInfoScraper scraper;
	CStdString strExtension;
	CStdString strLabel=pItem->GetLabel();
	if ( pItem->m_musicInfoTag.Loaded() )
	{
		CStdString strAlbum=pItem->m_musicInfoTag.GetAlbum();
		if (	strAlbum.size() )
		{
			strLabel=strAlbum;
		}
	}

	{
		// check cache
		CAlbum albuminfo;
		if ( m_database.GetAlbumInfo(strLabel, albuminfo) )
		{
			CMusicAlbumInfo album ;
			album.Set(albuminfo);
			CGUIWindowMusicInfo *pDlgAlbumInfo= (CGUIWindowMusicInfo*)m_gWindowManager.GetWindow(2001);
			pDlgAlbumInfo->SetAlbum(album);
			pDlgAlbumInfo->DoModal(GetID());
			return;
		}
	}

	// show dialog box indicating we're searching the album
	m_dlgProgress->SetHeading(185);
	m_dlgProgress->SetLine(0,strLabel);
	m_dlgProgress->SetLine(1,"");
	m_dlgProgress->SetLine(2,"");
	m_dlgProgress->StartModal(GetID());
	m_dlgProgress->Progress();
	bool bDisplayErr=false;
	
	// find album info
	if (	scraper.FindAlbuminfo(strLabel) )
	{
		m_dlgProgress->Close();
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
				CGUIDialogSelect *pDlg= (CGUIDialogSelect*)m_gWindowManager.GetWindow(2000);
				pDlg->SetHeading(szText);
				pDlg->Reset();
				for (int i=0; i < iAlbumCount; ++i)
				{
					CMusicAlbumInfo& info = scraper.GetAlbum(i);
					pDlg->Add(info.GetTitle2());
				}
				pDlg->DoModal(GetID());

				// and wait till user selects one
				iSelectedAlbum= pDlg->GetSelectedLabel();
				if (iSelectedAlbum< 0) return;
			}

			// ok, now show dialog we're downloading the album info
			CMusicAlbumInfo& album = scraper.GetAlbum(iSelectedAlbum);
			m_dlgProgress->SetHeading(185);
			m_dlgProgress->SetLine(0,album.GetTitle2());
			m_dlgProgress->SetLine(1,"");
			m_dlgProgress->SetLine(2,"");
			m_dlgProgress->StartModal(GetID());
			m_dlgProgress->Progress();

			// download the album info
			bool bLoaded=album.Loaded();
			if (!bLoaded) 
				bLoaded=album.Load();
			if ( bLoaded )
			{
				// ok, show album info
				m_dlgProgress->Close();
				{
					CAlbum albuminfo;
					
					albuminfo.strAlbum  = strLabel;//album.GetTitle();
					albuminfo.strArtist = album.GetArtist();
					albuminfo.strGenre  = album.GetGenre();
					albuminfo.strTones  = album.GetTones();
					albuminfo.strStyles = album.GetStyles();
					albuminfo.strReview = album.GetReview();
					albuminfo.strImage  = album.GetImageURL();
					albuminfo.iRating   = album.GetRating();
					albuminfo.iYear 		= atol( album.GetDateOfRelease().c_str() );
					m_database.AddAlbumInfo(albuminfo);
				}
				CGUIWindowMusicInfo *pDlgAlbumInfo= (CGUIWindowMusicInfo*)m_gWindowManager.GetWindow(2001);
				pDlgAlbumInfo->SetAlbum(album);
				pDlgAlbumInfo->DoModal(GetID());
				bUpdate=true;
			}
			else
			{
				// failed 2 download album info
				bDisplayErr=true;
			}
		}
		else 
		{
			// no albums found
			bDisplayErr=true;
		}
	}
	else
	{
		// unable 2 connect to www.allmusic.com
		bDisplayErr=true;
	}
	// if an error occured, then notice the user
	if (bDisplayErr)
	{
		m_dlgProgress->Close();
		pDlgOK->SetHeading(187);
		pDlgOK->SetLine(0,L"");
		pDlgOK->SetLine(2,L"");
		pDlgOK->SetLine(1,187);
		pDlgOK->DoModal(GetID());
	}
	if (bUpdate)
	{
		Update(m_strDirectory);
	}
	CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iSelectedItem);
	CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iSelectedItem);
}

void CGUIWindowMusicBase::OnRetrieveMusicInfo(VECFILEITEMS& items, bool bScan)
{
	CStdString strItem;

	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem* pItem=items[i];
		CStdString strExtension;
		CUtil::GetExtension(pItem->m_strPath,strExtension);

		if (bScan)
		{
			strItem.Format("%i/%i", i+1, items.size());
			m_dlgProgress->SetLine(0,strItem);
			m_dlgProgress->SetLine(1,CUtil::GetFileName(pItem->m_strPath) );
			m_dlgProgress->Progress();
			if (m_dlgProgress->IsCanceled()) return;
		}
		if (!pItem->m_bIsFolder && !CUtil::IsPlayList(pItem->m_strPath) )
		{
			bool bNewFile=false;
			CMusicInfoTag& tag=pItem->m_musicInfoTag;
			if (!tag.Loaded() )
			{
				if (strExtension!=".cdda" )
				{
					CSong song;
					if ( !m_database.GetSongByFileName(pItem->m_strPath, song) )
					{
						if (g_stSettings.m_bUseID3 || bScan)
						{

							CMusicInfoTagLoaderFactory factory;
							auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
							if (NULL != pLoader.get())
							{						
								if ( pLoader->Load(pItem->m_strPath,tag))
								{
									bNewFile=true;
								}
							}
						}
					}
					else
					{
						tag.SetAlbum(song.strAlbum);
						tag.SetArtist(song.strArtist);
						tag.SetGenre(song.strGenre);
						tag.SetDuration(song.iDuration);
						tag.SetTitle(song.strTitle);
						tag.SetTrackNumber(song.iTrack);
						tag.SetLoaded(true);
					}
				}
			}

			if (tag.Loaded() && bScan)
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

				m_database.AddSong(song);
			}
		}//if (!pItem->m_bIsFolder)
	}
}

void CGUIWindowMusicBase::RetrieveMusicInfo()
{
	DWORD dwTick=timeGetTime();

	OnRetrieveMusicInfo(m_vecItems);

	dwTick = timeGetTime() - dwTick;
	CStdString strTmp;
	strTmp.Format("RetrieveMusicInfo() took %imsec\n",dwTick); 
	OutputDebugString(strTmp.c_str());
}

void CGUIWindowMusicBase::OnQueueItem(int iItem)
{
	// add item 2 playlist
	const CFileItem* pItem=m_vecItems[iItem];
	AddItemToPlayList(pItem);
	
	//move to next item
	CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem+1);
	CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem+1);
	if (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() && !g_application.IsPlayingAudio() )
	{
		g_playlistPlayer.Play(0);
	}
}

void CGUIWindowMusicBase::AddItemToPlayList(const CFileItem* pItem) 
{
	if (pItem->m_bIsFolder)
	{
		// recursive
		if (pItem->GetLabel() == "..") return;
		CStdString strDirectory=pItem->m_strPath;
		VECFILEITEMS items;
		CFileItemList itemlist(items);
		GetDirectory(strDirectory, items);
		for (int i=0; i < (int) items.size(); ++i)
		{
			AddItemToPlayList(items[i]);
		}
	}
	else
	{
		if (!CUtil::IsPlayList(pItem->m_strPath))
		{
			CPlayList::CPlayListItem playlistItem ;
			playlistItem.SetFileName(pItem->m_strPath);
			playlistItem.SetDescription(pItem->GetLabel());
			playlistItem.SetDuration(pItem->m_musicInfoTag.GetDuration());
			g_playlistPlayer.GetPlaylist( PLAYLIST_MUSIC ).Add(playlistItem);
		}
	}
}

bool CGUIWindowMusicBase::DoSearch(const CStdString strDir,const CStdString& strSearch,VECFILEITEMS& items)
{
	m_dlgProgress->SetLine(0,strSearch);
	m_dlgProgress->SetLine(2,strDir );
	m_dlgProgress->Progress();

	if (m_dlgProgress->IsCanceled()) return false;
	VECFILEITEMS subDirItems;
	CFileItemList itemlist(subDirItems);
	GetDirectory(strDir,subDirItems);
	OnRetrieveMusicInfo(subDirItems);
	DoSort(subDirItems);

	bool bOpen=true;	
	bool bCancel=false;
	for (int i=0; i < (int)subDirItems.size(); ++i)
	{
		CFileItem *pItem= subDirItems[i];
		if ( pItem->m_bIsFolder)
		{
			if (pItem->GetLabel() != "..")
			{
				// search subfolder
				if (m_dlgProgress->IsCanceled()) 
				{
					bCancel=true;
					break;
				}
				if (!DoSearch(pItem->m_strPath,strSearch, items))
				{
					bCancel=true;
					break;
				}
				
				if (m_dlgProgress->IsCanceled()) 
				{
					bCancel=true;
					break;
				}
			}
		}
		else
		{
			bool bFound(false);
			CMusicInfoTag& tag=pItem->m_musicInfoTag;

			if (tag.Loaded())
			{
				// search title,artist,album...
				CStdString strTitle=tag.GetTitle();
				CStdString strAlbum=tag.GetAlbum();
				CStdString strArtist=tag.GetArtist();
				strTitle.ToLower();
				strAlbum.ToLower();
				strArtist.ToLower();
				if ( strTitle.Find(strSearch) >=0)
				{
					bFound=true;
				}
				if ( strAlbum.Find(strSearch) >=0)
				{
					bFound=true;
				}
				if ( strArtist.Find(strSearch) >=0)
				{
					bFound=true;
				}
			}

			if (!bFound)
			{
				// search path name
				CStdString strFileName=CUtil::GetFileName(pItem->m_strPath);
				strFileName.ToLower();
				if ( strFileName.Find(strSearch)>=0 )
				{
					bFound=true;
				}
			}

			if (bFound)
			{
				if (items.size()==0)
				{
					CFileItem *pItem = new CFileItem("..");
					pItem->m_strPath=m_strDirectory;
					pItem->m_bIsFolder=true;
					pItem->m_bIsShareOrDrive=false;
					items.push_back(pItem);
				}

				CFileItem* pNewItem = new CFileItem(*pItem);
				items.push_back(pNewItem);
				CStdString strFormat=g_localizeStrings.Get(282);
				CStdString strResult;
				strResult.Format(strFormat, items.size());
				m_dlgProgress->SetLine(1,strResult);
				m_dlgProgress->Progress();
				if (m_dlgProgress->IsCanceled()) 
				{
					bCancel=true;
					break;
				}
			}
		}
	}
	return !bCancel;
}

void CGUIWindowMusicBase::OnSearch()
{
	CXBVirtualKeyboard* pKeyboard = (CXBVirtualKeyboard*)m_gWindowManager.GetWindow(1000);
	pKeyboard->Reset();
	WCHAR wsFile[1024];
	wcscpy(wsFile,L"");
	pKeyboard->SetText(wsFile);
	pKeyboard->DoModal(GetID());
	if (!pKeyboard->IsConfirmed()) return;
	
	CStdString strSearch;
	const WCHAR* pSearchString=pKeyboard->GetText();
	CUtil::Unicode2Ansi(pSearchString,strSearch);
	
	strSearch.ToLower();
	m_dlgProgress->SetHeading(194);
	
	CStdString strResult;
	CStdString strFormat=g_localizeStrings.Get(282);
	strResult.Format(strFormat, 0);
	m_dlgProgress->SetLine(0,strSearch);
	m_dlgProgress->SetLine(1,strResult);
	m_dlgProgress->SetLine(2,m_strDirectory );
	m_dlgProgress->StartModal(GetID());
	m_dlgProgress->Progress();

	VECFILEITEMS items;
	DoSearch(m_strDirectory,	strSearch, items);
	m_dlgProgress->Close();

	if (items.size())
	{
		m_strDirectory=g_localizeStrings.Get(283);
		ClearFileItems();
		for (int i=0; i < (int)items.size(); i++)
		{
			CFileItem* pItem=items[i];
			CFileItem* pNewItem=new CFileItem(*pItem);
			m_vecItems.push_back(pNewItem);
		}
		CUtil::SetThumbs(m_vecItems);
		CUtil::FillInDefaultIcons(m_vecItems);
		UpdateListControl();
		UpdateButtons();
	}
	else
	{
		CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(2002);
		dlg->SetHeading( 194 );
		dlg->SetLine( 0, 284 );
		dlg->SetLine( 1, L"" );
		dlg->SetLine( 2, L"" );
		dlg->DoModal( GetID() );
	}
}
