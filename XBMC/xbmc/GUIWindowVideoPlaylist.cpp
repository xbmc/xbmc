#include "GUIWindowVideoPlayList.h"
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
#include "GUIThumbnailPanel.h"
#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4

#define CONTROL_BTNCLEAR					22

#define CONTROL_BTNPLAY						23
#define CONTROL_BTNNEXT						24
#define CONTROL_BTNPREVIOUS				25

#define CONTROL_LABELFILES        12

#define CONTROL_LIST							50
#define CONTROL_THUMBS						51

static int m_nTempPlayListWindow=0;
static CStdString m_strTempPlayListDirectory="";


CGUIWindowVideoPlaylist::CGUIWindowVideoPlaylist(void)
:CGUIWindow(0)
{
  m_strDirectory="";
}
CGUIWindowVideoPlaylist::~CGUIWindowVideoPlaylist(void)
{

}

bool CGUIWindowVideoPlaylist::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_PLAYBACK_STOPPED:
		{
			if (g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO)
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

		case GUI_MSG_WINDOW_DEINIT:
		{
      OutputDebugString("deinit guiwindowvideoplaylist!\n");
			m_iItemSelected=GetSelectedItem();
		}
		break;

    case GUI_MSG_WINDOW_INIT:
		{
      OutputDebugString("init guiwindowvideoplaylist!\n");
			CGUIWindow::OnMessage(message);

			Update("");

			if ((m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST) && m_vecItems.size()<=0)
			{
				m_iLastControl=CONTROL_BTNVIEWASICONS;
				SET_CONTROL_FOCUS(GetID(), m_iLastControl, 0);
			}

			if (m_iItemSelected>-1)
			{
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,m_iItemSelected);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,m_iItemSelected);
			}

			if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO)
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
				CGUIWindow::OnMessage(message);
        g_stSettings.m_iMyVideoPlaylistViewAsIcons++;
        if (g_stSettings.m_iMyVideoPlaylistViewAsIcons > VIEW_AS_LARGEICONS) g_stSettings.m_iMyVideoPlaylistViewAsIcons=VIEW_AS_LIST;
				
        ShowThumbPanel();

				g_settings.Save();
        UpdateButtons();

			}
			else if (iControl==CONTROL_BTNCLEAR)
			{
				ClearPlayList();
			}
			else if (iControl==CONTROL_BTNPLAY)
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
				g_playlistPlayer.Play(GetSelectedItem());
				UpdateButtons();
			}
			else if (iControl==CONTROL_BTNNEXT)
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
				g_playlistPlayer.PlayNext();
			}
			else if (iControl==CONTROL_BTNPREVIOUS)
			{
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
				g_playlistPlayer.PlayPrevious();
			}
      else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
      {
        int iItem=GetSelectedItem();
				int iAction=message.GetParam1();

				if (iAction == ACTION_QUEUE_ITEM)
        {
					OnQueueItem(iItem);
        }
        else if (iAction==ACTION_SELECT_ITEM)
        {
          OnClick(iItem);
        }
      }
		}
		break;

	}
	return CGUIWindow::OnMessage(message);
}

void CGUIWindowVideoPlaylist::OnAction(const CAction &action)
{
	if (action.wID==ACTION_PARENT_DIR)
	{
		//	Playlist has no parent dirs
		return;
	}

  if (action.wID==ACTION_PREVIOUS_MENU)
  {
		m_gWindowManager.ActivateWindow(WINDOW_HOME);
		return;
  }

	if (action.wID==ACTION_SHOW_PLAYLIST)
	{
    OutputDebugString("leave videplaylist!\n");
		m_gWindowManager.PreviousWindow();
		return;
	}

	CGUIWindow::OnAction(action);
}

void CGUIWindowVideoPlaylist::ClearPlayList()
{
	ClearFileItems();
	g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).Clear();
	UpdateListControl();
	UpdateButtons();
	SET_CONTROL_FOCUS(GetID(), CONTROL_BTNVIEWASICONS, 0);
}

void CGUIWindowVideoPlaylist::ClearFileItems()
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg1);         

  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

	CFileItemList itemlist(m_vecItems); // will clean up everything

}

void CGUIWindowVideoPlaylist::UpdateListControl()
{
  CGUIMessage msg1(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg1);         

  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

	for (int i=0; i < (int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];

		//	Format label for listcontrol
		//	and set thumb/icon for item
		OnFileItemFormatLabel(pItem);
	}

	DoSort(m_vecItems);

	ShowThumbPanel();

	for (int i=0; i < (int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];

    CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg);

    CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg2);
	}
}


void CGUIWindowVideoPlaylist::OnFileItemFormatLabel(CFileItem* pItem)
{
  CUtil::SetThumb(pItem);
	CUtil::FillInDefaultIcon(pItem);
}

void CGUIWindowVideoPlaylist::DoSort(VECFILEITEMS& items)
{

}

void CGUIWindowVideoPlaylist::UpdateButtons()
{
	//	Update playlist buttons
	if (m_vecItems.size() )
	{
		CONTROL_ENABLE(GetID(), CONTROL_BTNCLEAR);
		CONTROL_ENABLE(GetID(), CONTROL_BTNPLAY);

		if (g_application.IsPlayingVideo() && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO)
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
		CONTROL_DISABLE(GetID(), CONTROL_BTNCLEAR);
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
	switch (g_stSettings.m_iMyVideoPlaylistViewAsIcons)
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

bool CGUIWindowVideoPlaylist::ViewByIcon()
{
  if (g_stSettings.m_iMyVideoPlaylistViewAsIcons != VIEW_AS_LIST) return true;
  return false;
}

int CGUIWindowVideoPlaylist::GetSelectedItem()
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
	return iItem;
}

bool CGUIWindowVideoPlaylist::ViewByLargeIcon()
{
  if (g_stSettings.m_iMyVideoPlaylistViewAsIcons== VIEW_AS_LARGEICONS) return true;
  return false;
}


void CGUIWindowVideoPlaylist::ShowThumbPanel()
{
  int iItem=GetSelectedItem(); 
  if ( ViewByLargeIcon() )
  {
    CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
    pControl->SetThumbDimensions(10,16,100,100);
    pControl->SetTextureDimensions(128,128);
    pControl->SetItemHeight(150);
    pControl->SetItemWidth(150);
  }
  else
  {
    CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
    pControl->SetThumbDimensions(4,10,64,64);
    pControl->SetTextureDimensions(80,80);
    pControl->SetItemHeight(128);
    pControl->SetItemWidth(128);
  }
  if (iItem>-1)
  {
    CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,m_iItemSelected);
    CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,m_iItemSelected);
  }
}

void CGUIWindowVideoPlaylist::GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)
{
	if (items.size()) 
	{
		CFileItemList itemlist(items); // will clean up everything
	}

	CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
	/* copy playlist from general playlist*/
	int iCurrentSong=-1;
	if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
		iCurrentSong=g_playlistPlayer.GetCurrentSong();

	CStdString strPath,strFileName;
	for (int i=0; i < playlist.size(); ++i)
	{
		const CPlayList::CPlayListItem& item = playlist[i];

		CStdString strFileName   = item.GetFileName();
		CStdString strPath,strFName;
		CUtil::Split( strFileName, strPath, strFName);
		//m_Pathes.insert(strPath);
		
		CFileItem *pItem				 = new CFileItem(item.GetDescription());
		pItem->m_strPath			   = strFileName;
		pItem->m_bIsFolder		   = false;
		pItem->m_bIsShareOrDrive = false;

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
		}
		items.push_back(pItem);
	}
}

void CGUIWindowVideoPlaylist::Update(const CStdString &strDirectory)
{
	// get selected item
	int iItem=GetSelectedItem();
	CStdString strSelectedItem="";
	if (iItem >=0 && iItem < (int)m_vecItems.size())
	{
		CFileItem* pItem=m_vecItems[iItem];
		if (pItem->m_bIsFolder && pItem->GetLabel() != "..")
		{
			GetDirectoryHistoryString(pItem, strSelectedItem);
		}
	}

	m_iLastControl=GetFocusedControl();

	ClearFileItems();

	m_history.Set(strSelectedItem,m_strDirectory);
	m_strDirectory=strDirectory;

	GetDirectory(m_strDirectory, m_vecItems);


  UpdateListControl();
	UpdateButtons();

	strSelectedItem=m_history.Get(m_strDirectory);

	if (m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST)
	{
		if (ViewByIcon()) 
		{	
			SET_CONTROL_FOCUS(GetID(), CONTROL_THUMBS, 0);
		}
		else 
		{
			SET_CONTROL_FOCUS(GetID(), CONTROL_LIST, 0);
		}
	}

	CStdString strFileName;
	//	Search current playlist item
	if ((m_nTempPlayListWindow==GetID() && m_strTempPlayListDirectory.Find(m_strDirectory) > -1 && g_application.IsPlayingVideo() 
			&& g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO_TEMP) 
			|| (GetID()==WINDOW_VIDEO_PLAYLIST && g_playlistPlayer.GetCurrentPlaylist()==PLAYLIST_VIDEO && g_application.IsPlayingVideo()) )
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
		if (!bSelectedFound)
		{
			CStdString strHistory;
			GetDirectoryHistoryString(pItem, strHistory);
			if (strHistory==strSelectedItem)
			{
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,i);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,i);
				bSelectedFound=true;
			}
		}

		//	synchronize playlist with current directory
		if (!strFileName.IsEmpty() && pItem->m_strPath == strFileName)
		{
			pItem->Select(true);
		}

	}

}

void CGUIWindowVideoPlaylist::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
	strHistoryString=pItem->m_strPath;

	if (CUtil::HasSlashAtEnd(strHistoryString))
		strHistoryString.Delete(strHistoryString.size()-1);
}
void CGUIWindowVideoPlaylist::OnClick(int iItem)
{
	CFileItem* pItem=m_vecItems[iItem];
	CStdString strPath=pItem->m_strPath;
	g_playlistPlayer.SetCurrentPlaylist( PLAYLIST_VIDEO);
 	g_playlistPlayer.Play( iItem );
}

void CGUIWindowVideoPlaylist::OnQueueItem(int iItem)
{
	RemovePlayListItem(iItem);
}

void CGUIWindowVideoPlaylist::RemovePlayListItem(int iItem)
{
	const CFileItem* pItem=m_vecItems[iItem];
	CStdString strFileName=pItem->m_strPath;
	g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).Remove(strFileName);
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
	CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
	CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
}
