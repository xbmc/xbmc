// Todo: 
//  - directory history
//  - if movie does not exists when play movie is called then show dialog asking to insert the correct CD
//  - show if movie has subs

#include "stdafx.h"
#include "GUIWindowVideoTitle.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "texturemanager.h"
#include "util.h"
#include "url.h"
#include "utils/imdb.h"
#include "GUIDialogOK.h"
#include "GUIDialogprogress.h"
#include "GUIDialogSelect.h" 
#include "GUIWindowVideoInfo.h" 
#include "application.h" 
#include <algorithm>
#include "DetectDVDType.h"
#include "nfofile.h"
#include "filesystem/file.h"
#include "xbox/iosupport.h"
#include "playlistplayer.h"
#include "GUIThumbnailPanel.h"
#include "GUIDialogYesNo.h"
#define VIEW_AS_LIST           0
#define VIEW_AS_ICONS          1
#define VIEW_AS_LARGEICONS     2

#define CONTROL_BTNVIEWASICONS		 2
#define CONTROL_BTNSORTBY					 3
#define CONTROL_BTNSORTASC				 4
#define CONTROL_BTNTYPE            5
#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_LIST							10
#define CONTROL_THUMBS						11
#define CONTROL_LABELFILES        12
#define LABEL_TITLE              100

//****************************************************************************************************************************
struct SSortVideoTitleByTitle
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
    CFileItem& rpStart=*pStart;
    CFileItem& rpEnd=*pEnd;
		if (rpStart.GetLabel()=="..") return true;
		if (rpEnd.GetLabel()=="..") return false;
		bool bGreater=true;
		if (g_stSettings.m_bMyVideoTitleSortAscending) bGreater=false;
    if ( rpStart.m_bIsFolder   == rpEnd.m_bIsFolder)
		{
			char szfilename1[1024];
			char szfilename2[1024];

			switch ( g_stSettings.m_iMyVideoTitleSortMethod ) 
			{
				case 0:	//	Sort by name
					strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
					break;

				case 1: // Sort by year
          if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear  ) return bGreater;
					if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear ) return !bGreater;
					
					strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
				break;

        case 2: // sort by rating
          if ( rpStart.m_fRating < rpEnd.m_fRating) return bGreater;
					if ( rpStart.m_fRating > rpEnd.m_fRating) return !bGreater;
					
          
					strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
        
        case 3: // sort dvdLabel
        {
          int iLabel1=0,iLabel2=0;
          char szTmp[20];
          int  pos=0;
          strcpy(szTmp,"");
          for (int x=0; x < (int)rpStart.m_strDVDLabel.size(); x++)
          {
            char k=rpStart.m_strDVDLabel.GetAt(x);
            if (k >='0'&& k <= '9') 
            {
              if ( (k=='0' && pos > 0) || (k != '0' ) )  
              {
                szTmp[pos++] = k;
                szTmp[pos]=0;
              }
            }
          }
          sscanf(szTmp,"%i", &iLabel1);
          strcpy(szTmp,"");
          pos=0;
          for (int x=0; x < (int)rpEnd.m_strDVDLabel.size(); x++)
          {
            char k=rpEnd.m_strDVDLabel.GetAt(x);
            if (k >='0'&& k <= '9') 
            {
              if ( (k=='0' && pos > 0) || (k != '0' ) )  
              {
                szTmp[pos++] = k;
                szTmp[pos]=0;
              }
            }
          }
          sscanf(szTmp,"%i", &iLabel2);

          if ( iLabel1 < iLabel2) return bGreater;
					if ( iLabel1 > iLabel2) return !bGreater;
          
          strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
          
        }
        break;

				default:	//	Sort by Filename by default
					strcpy(szfilename1, rpStart.GetLabel().c_str());
					strcpy(szfilename2, rpEnd.GetLabel().c_str());
					break;
			}


			for (int i=0; i < (int)strlen(szfilename1); i++)
				szfilename1[i]=tolower((unsigned char)szfilename1[i]);
			
			for (i=0; i < (int)strlen(szfilename2); i++)
				szfilename2[i]=tolower((unsigned char)szfilename2[i]);
			//return (rpStart.strPath.compare( rpEnd.strPath )<0);

			if (g_stSettings.m_bMyVideoTitleSortAscending)
				return (strcmp(szfilename1,szfilename2)<0);
			else
				return (strcmp(szfilename1,szfilename2)>=0);
		}
    if (!rpStart.m_bIsFolder) return false;
		return true;
	}
};

//****************************************************************************************************************************
CGUIWindowVideoTitle::CGUIWindowVideoTitle()
{
	m_strDirectory="";
  m_iItemSelected=-1;
	m_iLastControl=-1;
}

//****************************************************************************************************************************
CGUIWindowVideoTitle::~CGUIWindowVideoTitle()
{
}

//****************************************************************************************************************************
void CGUIWindowVideoTitle::OnAction(const CAction &action)
{
  if (action.wID == ACTION_DELETE_ITEM)
  {
    int iItem=GetSelectedItem();
    if (iItem < 0|| iItem >= (int)m_vecItems.size()) return;
	  
		CFileItem* pItem=m_vecItems[iItem];
    if (pItem->m_bIsFolder) return;
      
    CGUIDialogYesNo* pDialog= (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog) return;
    pDialog->SetHeading(432);
    pDialog->SetLine(0,433);
    pDialog->SetLine(1,434);
    pDialog->SetLine(2,L"");
    pDialog->DoModal(GetID());
    if (!pDialog->IsConfirmed()) return;
		VECMOVIESFILES movies;
    m_database.GetFiles(atol(pItem->m_strPath),movies);
		if (movies.size() <=0) return;
    m_database.DeleteMovie(movies[0]);
    Update( m_strDirectory );
    return;

  }

  if (action.wID == ACTION_PARENT_DIR)
	{
		GoParentFolder();
		return;
	}

 	if (action.wID == ACTION_PREVIOUS_MENU)
	{
		m_gWindowManager.ActivateWindow(WINDOW_HOME);
		return;
	}
	CGUIWindow::OnAction(action);
}

//****************************************************************************************************************************
bool CGUIWindowVideoTitle::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		{
			if ( !m_strDirectory.IsEmpty() ) {
				if ( CUtil::IsCDDA( m_strDirectory ) || CUtil::IsDVD( m_strDirectory ) || CUtil::IsISO9660( m_strDirectory ) ) {
					//	Disc has changed and we are inside a DVD Drive share, get out of here :)
					m_strDirectory = "";
					Update( m_strDirectory );
				}
			}
			else {
				int iItem = GetSelectedItem();
				Update( m_strDirectory );
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			}
		}
		break;

		case GUI_MSG_DVDDRIVE_CHANGED_CD:
		{
			if ( m_strDirectory.IsEmpty() ) {
				int iItem = GetSelectedItem();
				Update( m_strDirectory );
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
				CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			}
		}
		break;
		case GUI_MSG_WINDOW_DEINIT:
			m_iLastControl=GetFocusedControl();
			m_iItemSelected=GetSelectedItem();
			Clear();
      m_database.Close();
		break;

    case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);
      m_database.Open();
			m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
			m_rootDir.SetMask(g_stSettings.m_szMyVideoExtensions);
			m_rootDir.SetShares(g_settings.m_vecMyVideoShares);

			if (m_iLastControl>-1)
				SET_CONTROL_FOCUS(GetID(), m_iLastControl, 0);

			Update(m_strDirectory);

      ShowThumbPanel();
      if (m_iItemSelected >=0)
      {
			  CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,m_iItemSelected)
			  CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,m_iItemSelected)
      }
		
			return true;
		}
		break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if (iControl==CONTROL_BTNVIEWASICONS)
      {
        bool bLargeIcons(false);
		    if ( m_strDirectory.IsEmpty() )
        {
		      g_stSettings.m_iMyVideoTitleRootViewAsIcons++;
          if (g_stSettings.m_iMyVideoTitleRootViewAsIcons > VIEW_AS_LARGEICONS) g_stSettings.m_iMyVideoTitleRootViewAsIcons=VIEW_AS_LIST;
        }
		    else
        {
		      g_stSettings.m_iMyVideoTitleViewAsIcons++;
          if (g_stSettings.m_iMyVideoTitleViewAsIcons > VIEW_AS_LARGEICONS) g_stSettings.m_iMyVideoTitleViewAsIcons=VIEW_AS_LIST;
        }
        ShowThumbPanel();

				g_settings.Save();
        UpdateButtons();
      }
      else if (iControl==CONTROL_BTNSORTBY) // sort by
      {
        g_stSettings.m_iMyVideoTitleSortMethod++;
        if (g_stSettings.m_iMyVideoTitleSortMethod>=4) g_stSettings.m_iMyVideoTitleSortMethod=0;
				g_settings.Save();
        
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
        g_stSettings.m_bMyVideoTitleSortAscending=!g_stSettings.m_bMyVideoTitleSortAscending;
				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_PLAY_DVD)
      {
          // play movie...
        CUtil::PlayDVD();
      }
      else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
      {
         // get selected item
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
        g_graphicsContext.SendMessage(msg);         
        int iItem=msg.GetParam1();
        int iAction=message.GetParam1();
        if (iAction == ACTION_SHOW_INFO) 
        {
					  OnInfo(iItem);
        }
        if (iAction == ACTION_SELECT_ITEM)
				{
					OnClick(iItem);
				}
      }
      else if (iControl==CONTROL_BTNTYPE)
      {
				CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(),CONTROL_BTNTYPE);
				m_gWindowManager.SendMessage(msg);

				int nSelected=msg.GetParam1();
				int nNewWindow=WINDOW_VIDEOS;
				switch (nSelected)
				{
				case 0:	//	Movies
					nNewWindow=WINDOW_VIDEOS;
					break;
				case 1:	//	Genre
					nNewWindow=WINDOW_VIDEO_GENRE;
					break;
				case 2:	//	Actors
					nNewWindow=WINDOW_VIDEO_ACTOR;
					break;
				case 3:	//	Year
					nNewWindow=WINDOW_VIDEO_YEAR;
					break;
				case 4:	//	Titel
					nNewWindow=WINDOW_VIDEO_TITLE;
					break;
				}

				if (nNewWindow!=GetID())
				{
					g_stSettings.m_iVideoStartWindow=nNewWindow;
					g_settings.Save();
					m_gWindowManager.ActivateWindow(nNewWindow);
				}

        return true;
      }
    }
	}
  return CGUIWindow::OnMessage(message);
}

//****************************************************************************************************************************
void CGUIWindowVideoTitle::UpdateButtons()
{
	//	Remove labels from the window selection
	CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_BTNTYPE);
	g_graphicsContext.SendMessage(msg);

	//	Add labels to the window selection
	CStdString strItem=g_localizeStrings.Get(342);	//	Movies
	CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_BTNTYPE);
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(135);	//	Genre
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(344);	//	Actors
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(345);	//	Year
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	strItem=g_localizeStrings.Get(369);	//	Titel
	msg2.SetLabel(strItem);
	g_graphicsContext.SendMessage(msg2);

	//	Select the current window as default item 
	CONTROL_SELECT_ITEM(GetID(), CONTROL_BTNTYPE, 4);


	SET_CONTROL_HIDDEN(GetID(), CONTROL_LIST);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_THUMBS);
	bool bViewIcon = false;
  int iString;
	if ( m_strDirectory.IsEmpty() ) 
  {
		switch (g_stSettings.m_iMyVideoTitleRootViewAsIcons)
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
		switch (g_stSettings.m_iMyVideoTitleViewAsIcons)
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
      SET_CONTROL_VISIBLE(GetID(), CONTROL_THUMBS);
    }
    else
    {
      SET_CONTROL_VISIBLE(GetID(), CONTROL_LIST);
    }

    ShowThumbPanel();
		SET_CONTROL_LABEL(GetID(), CONTROL_BTNVIEWASICONS,iString);
    
    switch (g_stSettings.m_iMyVideoTitleSortMethod)
    {
      case 0:
        iString=365;
      break;
      case 1:
        iString=366;
      break;
      case 2:
        iString=367;
      break;
      case 3:
        iString=430;
      break;
    }
		SET_CONTROL_LABEL(GetID(), CONTROL_BTNSORTBY,iString);

    if ( g_stSettings.m_bMyVideoTitleSortAscending)
    {
      CGUIMessage msg(GUI_MSG_DESELECTED,GetID(), CONTROL_BTNSORTASC);
      g_graphicsContext.SendMessage(msg);
    }
    else
    {
      CGUIMessage msg(GUI_MSG_SELECTED,GetID(), CONTROL_BTNSORTASC);
      g_graphicsContext.SendMessage(msg);
    }

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

//****************************************************************************************************************************
void CGUIWindowVideoTitle::Clear()
{
	CFileItemList itemlist(m_vecItems); // will clean up everything
}

//****************************************************************************************************************************
void CGUIWindowVideoTitle::OnSort()
{
 CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg);         

  
  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

  
  
  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    if (g_stSettings.m_iMyVideoTitleSortMethod==0||g_stSettings.m_iMyVideoTitleSortMethod==2)
    {
			if (pItem->m_bIsFolder) pItem->SetLabel2("");
      else 
			{
        CStdString strRating;
        strRating.Format("%2.2f", pItem->m_fRating);
				pItem->SetLabel2(strRating);
			}
    }
    else if (g_stSettings.m_iMyVideoTitleSortMethod==3)
    {
      pItem->SetLabel2(pItem->m_strDVDLabel);
    }
    else
    {
      if (pItem->m_stTime.wYear )
			{
				CStdString strDateTime;
        strDateTime.Format("%i",pItem->m_stTime.wYear ); 
        pItem->SetLabel2(strDateTime);
			}
      else
        pItem->SetLabel2("");
    }
  }

  
  sort(m_vecItems.begin(), m_vecItems.end(), SSortVideoTitleByTitle());

  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg);    
    CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg2);         
  }
}

//****************************************************************************************************************************
void CGUIWindowVideoTitle::Update(const CStdString &strDirectory)
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
			m_history.Set(strSelectedItem,m_strDirectory);
		}
	}
  Clear();
  m_strDirectory=strDirectory;
  VECMOVIES movies;
  m_database.GetMovies(movies);
  for (int i=0; i < (int)movies.size(); ++i)
  {
    CIMDBMovie movie=movies[i];
    CFileItem *pItem = new CFileItem(movie.m_strTitle);
    pItem->m_strPath=movie.m_strSearchString;
		pItem->m_bIsFolder=false;
    pItem->m_bIsShareOrDrive=false;

    CStdString strThumb;
    CUtil::GetVideoThumbnail(movie.m_strIMDBNumber,strThumb);
		if (CUtil::FileExists(strThumb))
			pItem->SetThumbnailImage(strThumb);
    pItem->m_fRating     = movie.m_fRating; 
    pItem->m_stTime.wYear= movie.m_iYear;
    pItem->m_strDVDLabel = movie.m_strDVDLabel;
		m_vecItems.push_back(pItem);
  }
  SET_CONTROL_LABEL(GetID(), LABEL_TITLE,m_strDirectory);
  
	CUtil::SetThumbs(m_vecItems);
  SetIMDBThumbs(m_vecItems);

	// Fill in default icons
	CStdString strPath;
	for (int i=0; i<(int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];
		strPath=pItem->m_strPath;
		//	Fake a videofile
		pItem->m_strPath=pItem->m_strPath+".avi";

		CUtil::FillInDefaultIcon(pItem);
		pItem->m_strPath=strPath;
	}

  OnSort();
  UpdateButtons();
  strSelectedItem=m_history.Get(m_strDirectory);	

	m_iLastControl=GetFocusedControl();

	if (m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST)
	{
		if ( ViewByIcon() ) {	
			SET_CONTROL_FOCUS(GetID(), CONTROL_THUMBS, 0);
		}
		else {
			SET_CONTROL_FOCUS(GetID(), CONTROL_LIST, 0);
		}
	}

  for (int i=0; i < (int)m_vecItems.size(); ++i)
	{
		CFileItem* pItem=m_vecItems[i];
		if (pItem->m_strPath==strSelectedItem)
		{
			CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,i);
			CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,i);
			break;
		}
	}
}

//****************************************************************************************************************************
void CGUIWindowVideoTitle::Render()
{
	CGUIWindow::Render();
}

//****************************************************************************************************************************
void CGUIWindowVideoTitle::OnClick(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
  CStdString strPath=pItem->m_strPath;

	CStdString strExtension;
	CUtil::GetExtension(pItem->m_strPath,strExtension);

  if (pItem->m_bIsFolder)
  {
    m_iItemSelected=-1;
		if ( pItem->m_bIsShareOrDrive ) 
		{
			if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
				return;
		}
    Update(strPath);
  }
  else
	{
    m_iItemSelected=GetSelectedItem();
    int iSelectedFile=1;
		VECMOVIESFILES movies;
    m_database.GetFiles(atol(pItem->m_strPath),movies);
		if (movies.size() <=0) return;
    if (!CheckMovie(movies[0])) return;
    if (movies.size() >1)
    {
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
		  if (dlg)
      {
        dlg->SetNumberOfFiles(movies.size());
		    dlg->DoModal(GetID());
		    iSelectedFile = dlg->GetSelectedFile();
      }
      if (iSelectedFile < 1) return;
    }

    g_playlistPlayer.Reset();
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
    CPlayList& playlist=g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO_TEMP);
    playlist.Clear();
    for (int i=iSelectedFile-1; i < (int)movies.size(); ++i)
    {
      CStdString strFileName=movies[i];
      CPlayList::CPlayListItem item;
      item.SetFileName(strFileName);
      playlist.Add(item);
    }

    // play movie...
		g_playlistPlayer.PlayNext();
	}
}

void CGUIWindowVideoTitle::OnInfo(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
 	
  VECMOVIESFILES movies;
  m_database.GetFiles(atol(pItem->m_strPath),movies);
	if (movies.size() <=0) return;
  CStdString strFilePath=movies[0];
  CStdString strFile=CUtil::GetFileName(strFilePath);
  ShowIMDB(strFile,strFilePath, "" ,false);
}


int CGUIWindowVideoTitle::GetSelectedItem()
{
	int iControl;
	bool bViewIcon = false;
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
void CGUIWindowVideoTitle::SetIMDBThumbs(VECFILEITEMS& items)
{
}

bool CGUIWindowVideoTitle::ViewByIcon()
{
  if ( m_strDirectory.IsEmpty() )
  {
    if (g_stSettings.m_iMyVideoTitleRootViewAsIcons != VIEW_AS_LIST) return true;
  }
  else
  {
    if (g_stSettings.m_iMyVideoTitleViewAsIcons != VIEW_AS_LIST) return true;
  }
  return false;
}

bool CGUIWindowVideoTitle::ViewByLargeIcon()
{
  if ( m_strDirectory.IsEmpty() )
  {
    if (g_stSettings.m_iMyVideoTitleRootViewAsIcons == VIEW_AS_LARGEICONS) return true;
  }
  else
  {
    if (g_stSettings.m_iMyVideoTitleViewAsIcons== VIEW_AS_LARGEICONS) return true;
  }
  return false;
}
