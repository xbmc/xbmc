
#include "stdafx.h"
#include "guiwindowpictures.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "util.h"
#include "url.h"
#include "picture.h"
#include "utils/log.h"
#include "application.h"
#include <algorithm>
#include "GUIDialogOK.h"
#include "DetectDVDType.h"
#include "sectionloader.h"
#include "GUIThumbnailPanel.h"
#include "AutoSwitch.h"

#define VIEW_AS_LIST           0
#define VIEW_AS_ICONS          1
#define VIEW_AS_LARGEICONS     2

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4

#define CONTROL_BTNSLIDESHOW			6
#define CONTROL_BTNSLIDESHOW_RECURSIVE			7

#define CONTROL_BTNCREATETHUMBS		8
#define CONTROL_SHUFFLE						9
#define CONTROL_LIST							10
#define CONTROL_THUMBS						11
#define CONTROL_LABELFILES         12

struct SSortPicturesByName
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
    CFileItem& rpStart=*pStart;
    CFileItem& rpEnd=*pEnd;
		if (rpStart.GetLabel()=="..") return true;
		if (rpEnd.GetLabel()=="..") return false;
		bool bGreater=true;
		if (m_bSortAscending) bGreater=false;
    if ( rpStart.m_bIsFolder   == rpEnd.m_bIsFolder)
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

        case 2:
          if ( rpStart.m_dwSize > rpEnd.m_dwSize) return bGreater;
					if ( rpStart.m_dwSize < rpEnd.m_dwSize) return !bGreater;
					return true;
        break;

        case 3:	//	Sort by share type
					if ( rpStart.m_iDriveType > rpEnd.m_iDriveType) return bGreater;
					if ( rpStart.m_iDriveType < rpEnd.m_iDriveType) return !bGreater;
 					strcpy(szfilename1, rpStart.GetLabel());
					strcpy(szfilename2, rpEnd.GetLabel());
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

			if (m_bSortAscending)
				return (strcmp(szfilename1,szfilename2)<0);
			else
				return (strcmp(szfilename1,szfilename2)>=0);
		}
    if (!rpStart.m_bIsFolder) return false;
		return true;
	}
	bool m_bSortAscending;
	int m_iSortMethod;
};

CGUIWindowPictures::CGUIWindowPictures(void)
:CGUIWindow(0)
{
	m_strDirectory="?";
  m_iItemSelected=-1;
	m_iLastControl=-1;

	m_iViewAsIcons=-1;
	m_iViewAsIconsRoot=-1;
}

CGUIWindowPictures::~CGUIWindowPictures(void)
{
}

void CGUIWindowPictures::OnAction(const CAction &action)
{
	if (action.wID == ACTION_PARENT_DIR)
	{
		GoParentFolder();
		return;
	}
	if (action.wID == ACTION_PREVIOUS_MENU)
	{
		m_gWindowManager.PreviousWindow();
		return;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowPictures::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_START_SLIDESHOW:
		{
			CStdString* pUrl = (CStdString*) message.GetLPVOID();
			Update( *pUrl );
			OnSlideShow();
		}
		break;
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
		{
			m_iLastControl=GetFocusedControl();
			m_iItemSelected=GetSelectedItem();

			Clear();
			if (message.GetParam1() != WINDOW_SLIDESHOW)
			{
				CSectionLoader::Unload("CXIMAGE");
			}
		}
    break;

    case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);
			if (message.GetParam1() != WINDOW_SLIDESHOW)
			{
				CSectionLoader::Load("CXIMAGE");
			}
			m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

			m_rootDir.SetMask(g_stSettings.m_szMyPicturesExtensions);
			m_rootDir.SetShares(g_settings.m_vecMyPictureShares);

			if (m_strDirectory=="?")
			{
				m_strDirectory=g_stSettings.m_szDefaultPictures;
				SetHistoryForPath(m_strDirectory);
			}

			if (m_iLastControl>-1)
				SET_CONTROL_FOCUS(GetID(), m_iLastControl, 0);

			if (m_iViewAsIcons==-1 && m_iViewAsIconsRoot==-1)
			{
				m_iViewAsIcons=g_stSettings.m_iMyPicturesViewAsIcons;
				m_iViewAsIconsRoot=g_stSettings.m_iMyPicturesRootViewAsIcons;
			}

			Update(m_strDirectory);

			UpdateThumbPanel();

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
				// cycle LIST->ICONS->LARGEICONS
				int iViewMode = VIEW_AS_ICONS;
				if (ViewByIcon()) iViewMode = VIEW_AS_LARGEICONS;
				if (ViewByLargeIcon()) iViewMode = VIEW_AS_LIST;
				SetViewMode(iViewMode);
				g_settings.Save();
				UpdateThumbPanel();
        UpdateButtons();
      }
      else if (iControl==CONTROL_BTNSORTBY) // sort by
      {
				if (m_strDirectory.IsEmpty())
				{
					if (g_stSettings.m_iMyPicturesRootSortMethod==0)
						g_stSettings.m_iMyPicturesRootSortMethod=3;
					else
						g_stSettings.m_iMyPicturesRootSortMethod=0;
				}
				else
				{
					g_stSettings.m_iMyPicturesSortMethod++;
					if (g_stSettings.m_iMyPicturesSortMethod >=3) g_stSettings.m_iMyPicturesSortMethod=0;
				}

				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
				if (m_strDirectory.IsEmpty())
					g_stSettings.m_bMyPicturesRootSortAscending=!g_stSettings.m_bMyPicturesRootSortAscending;
				else
					g_stSettings.m_bMyPicturesSortAscending=!g_stSettings.m_bMyPicturesSortAscending;

				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSLIDESHOW) // Slide Show
      {
				OnSlideShow();
      }
      else if (iControl==CONTROL_BTNSLIDESHOW_RECURSIVE) // Recursive Slide Show
      {
        OnSlideShowRecursive();
      }
      else if (iControl==CONTROL_BTNCREATETHUMBS) // Create Thumbs
      {
				OnCreateThumbs();
      }
			else if (iControl==CONTROL_SHUFFLE)
			{
				g_guiSettings.ToggleBool("Slideshow.Shuffle");
				g_settings.Save();
			}
			else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
      {
         // get selected item
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
        g_graphicsContext.SendMessage(msg);         
        int iItem=msg.GetParam1();
				int iAction=message.GetParam1();
				if (iAction==ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
				{
					OnClick(iItem);
				}
				if (iAction==ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
				{
					OnPopupMenu(iItem);
				}
      }
    }
    break;
	}
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowPictures::OnSort()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg);         

  
  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

  
  
  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    if (g_stSettings.m_iMyPicturesSortMethod==0||g_stSettings.m_iMyPicturesSortMethod==2)
    {
			if (pItem->m_bIsFolder) pItem->SetLabel2("");
      else 
			{
				CStdString strFileSize;
				CUtil::GetFileSize(pItem->m_dwSize, strFileSize);
				pItem->SetLabel2(strFileSize);
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

	SortItems(m_vecItems);

  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg);    
    CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg2);         
  }
}

void CGUIWindowPictures::Clear()
{
	CFileItemList itemlist(m_vecItems); // will clean up everything
}

void CGUIWindowPictures::UpdateButtons()
{
	SET_CONTROL_HIDDEN(GetID(), CONTROL_THUMBS);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_LIST);

	int iString = 101;	// view as list
	if (ViewByIcon())
		iString = 100;		// view as icon
	if (ViewByLargeIcon())
		iString = 417;		// view as large icon

	SET_CONTROL_LABEL(GetID(), CONTROL_BTNVIEWASICONS,iString);
	UpdateThumbPanel();

	if (ViewByIcon()) 
	{
		SET_CONTROL_VISIBLE(GetID(), CONTROL_THUMBS);
		SET_CONTROL_HIDDEN(GetID(), CONTROL_LIST);
	}
	else
	{
		SET_CONTROL_VISIBLE(GetID(), CONTROL_LIST);
		SET_CONTROL_HIDDEN(GetID(), CONTROL_THUMBS);
	}

	if (g_guiSettings.GetBool("Slideshow.Shuffle"))
	{
		CGUIMessage msg2(GUI_MSG_SELECTED,GetID(),CONTROL_SHUFFLE,0,0,NULL);
		g_graphicsContext.SendMessage(msg2);  
	}
	else
	{
		CGUIMessage msg2(GUI_MSG_DESELECTED,GetID(),CONTROL_SHUFFLE,0,0,NULL);
		g_graphicsContext.SendMessage(msg2);  
	}

  UpdateThumbPanel();
	SET_CONTROL_LABEL(GetID(), CONTROL_BTNVIEWASICONS,iString);

	//	Update sort by button
	SET_CONTROL_LABEL(GetID(), CONTROL_BTNSORTBY,SortMethod());

	//	Update sorting control
	if (SortAscending())
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

void CGUIWindowPictures::Update(const CStdString &strDirectory)
{
	UpdateDir(strDirectory);
	if (!m_strDirectory.IsEmpty() && g_guiSettings.GetBool("Pictures.UseAutoSwitching"))
	{
		m_iViewAsIcons = CAutoSwitch::GetView(m_vecItems);

		UpdateThumbPanel();
		UpdateButtons();

		int iControl = CONTROL_LIST;
		if (m_iViewAsIcons != VIEW_AS_LIST) iControl = CONTROL_THUMBS;
		SET_CONTROL_FOCUS(GetID(), iControl, 0);
	}
}

void CGUIWindowPictures::UpdateDir(const CStdString &strDirectory)
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
			m_history.Set(strSelectedItem,m_strDirectory);
		}
	}
  Clear();

	GetDirectory(strDirectory, m_vecItems);

	m_iLastControl=GetFocusedControl();

	m_strDirectory=strDirectory;
	CUtil::SetThumbs(m_vecItems);
	if (g_guiSettings.GetBool("Pictures.HideExtensions"))
		CUtil::RemoveExtensions(m_vecItems);
	CUtil::FillInDefaultIcons(m_vecItems);
	OnSort();
	UpdateButtons();

	strSelectedItem=m_history.Get(m_strDirectory);	

	if (m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST)
	{
		if ( ViewByIcon() ) 
		{	
			SET_CONTROL_FOCUS(GetID(), CONTROL_THUMBS, 0);
		}
		else {
			SET_CONTROL_FOCUS(GetID(), CONTROL_LIST, 0);
		}
	}
	UpdateThumbPanel();

	for (int i=0; i < (int)m_vecItems.size(); ++i)
	{
		CFileItem* pItem=m_vecItems[i];
		CStdString strHistory;
		GetDirectoryHistoryString(pItem, strHistory);
		if (strHistory==strSelectedItem)
		{
			CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,i);
			CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,i);
			break;
		}
	}

}

void CGUIWindowPictures::OnClick(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
  CStdString strPath=pItem->m_strPath;
	if (pItem->m_bIsFolder)
	{
    m_iItemSelected=-1;
		if ( pItem->m_bIsShareOrDrive ) {
			if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
				return;
		}
		Update(strPath);
	}
	else
	{
		// show picture
    m_iItemSelected=GetSelectedItem();
		OnShowPicture(strPath);
	}
}

bool CGUIWindowPictures::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
	if ( iDriveType == SHARE_TYPE_DVD ) {
		CDetectDVDMedia::WaitMediaReady();
		if ( !CDetectDVDMedia::IsDiscInDrive() ) {
			CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
      if (dlg)
      {
			  dlg->SetHeading( 218 );
			  dlg->SetLine( 0, 219 );
			  dlg->SetLine( 1, L"" );
			  dlg->SetLine( 2, L"" );
			  dlg->DoModal( GetID() );
      }
			int iItem = GetSelectedItem();
			Update( m_strDirectory );
			CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
			CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
			return false;
		}
	}
	else if ( iDriveType == SHARE_TYPE_REMOTE ) {
			// TODO: Handle not connected to a remote share
		if ( !CUtil::IsEthernetConnected() ) {
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
	else
		return true;
  return true;
}

void CGUIWindowPictures::OnShowPicture(const CStdString& strPicture)
{
	CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
	if (!pSlideShow)
		return;
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

	pSlideShow->Reset();
  for (int i=0; i < (int)m_vecItems.size();++i)
  {
    CFileItem* pItem=m_vecItems[i];
    if (!pItem->m_bIsFolder)
    {
		  pSlideShow->Add(pItem->m_strPath);
    }
  }
	pSlideShow->Select(strPicture);
	m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowPictures::AddDir(CGUIWindowSlideShow *pSlideShow,const CStdString& strPath)
{
  if (!pSlideShow) return;
  VECFILEITEMS items;
  m_rootDir.GetDirectory(strPath,items);
	SortItems(items);

  for (int i=0; i < (int)items.size();++i)
  {
    CFileItem* pItem=items[i];
    if (!pItem->m_bIsFolder)
    {
		  pSlideShow->Add(pItem->m_strPath);
    }
    else
    {
      AddDir(pSlideShow,pItem->m_strPath);
    }
  }
  CFileItemList itemlist(items); // will clean up everything
}

void  CGUIWindowPictures::OnSlideShowRecursive(const CStdString &strPicture)
{
	CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
	if (!pSlideShow)
		return;
	
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  AddDir(pSlideShow,m_strDirectory);
  pSlideShow->StartSlideShow();
	if (!strPicture.IsEmpty())
		pSlideShow->Select(strPicture);
	m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowPictures::OnSlideShowRecursive()
{
	CStdString strEmpty = "";
	OnSlideShowRecursive(strEmpty);
}

void CGUIWindowPictures::OnSlideShow()
{
	CStdString strEmpty = "";
	OnSlideShow(strEmpty);
}

void CGUIWindowPictures::OnSlideShow(const CStdString &strPicture)
{
	CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
	if (!pSlideShow)
		return;
  if (g_application.IsPlayingVideo())
      g_application.StopPlaying();

	pSlideShow->Reset();
  for (int i=0; i < (int)m_vecItems.size();++i)
  {
    CFileItem* pItem=m_vecItems[i];
    if (!pItem->m_bIsFolder)
    {
		  pSlideShow->Add(pItem->m_strPath);
    }
  }
	pSlideShow->StartSlideShow();
	if (!strPicture.IsEmpty())
		pSlideShow->Select(strPicture);
	m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

bool CGUIWindowPictures::OnCreateThumbs()
{
  if (m_dlgProgress) 
  {
    m_dlgProgress->SetHeading(110);
	  m_dlgProgress->StartModal(GetID());
  }
  CSectionLoader::Load("CXIMAGE");
  // calculate the number of items to take thumbs of
  int iTotalItems = m_vecItems.size()-CUtil::GetFolderCount(m_vecItems);
  int iCurrentItem = 0;
  
  if (m_dlgProgress->IsCanceled()) return false;
  
  bool bCancel=false;

  // now run through and create the thumbs
  for (int i=0; i < (int)m_vecItems.size();++i)
  {
    CFileItem* pItem=m_vecItems[i];
	
		if (m_dlgProgress->IsCanceled())
		{
			bCancel=true;
			break;
		}

    if (!pItem->m_bIsFolder)
    {	
			iCurrentItem++;
			WCHAR wstrProgress[128];
			WCHAR wstrFile[128];
      swprintf(wstrProgress,L"   progress:%i/%i", iCurrentItem,iTotalItems );
      swprintf(wstrFile,L"   picture:%S", pItem->GetLabel().c_str() );

			if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0, wstrFile);
			  m_dlgProgress->SetLine(1, wstrProgress);
			  m_dlgProgress->SetLine(2, L"");
			  m_dlgProgress->Progress();
			  if ( m_dlgProgress->IsCanceled() ) break;
      }
			CPicture picture;
      picture.CreateThumnail(pItem->m_strPath);
	  
			if (bCancel) break;
    }
  }
  CSectionLoader::Unload("CXIMAGE");
	if (m_dlgProgress) m_dlgProgress->Close();
  Update(m_strDirectory);
  return !bCancel;
}

void CGUIWindowPictures::CreateFolderThumbs(bool bRecurse)
{
  if (m_dlgProgress) 
  {
    m_dlgProgress->SetHeading(110);
	  m_dlgProgress->StartModal(GetID());
  }
	int iTotalItems = 0;
	int iCurrentItem = 0;
	CSectionLoader::Load("CXIMAGE");
	DoCreateFolderThumbs(m_strDirectory, &iTotalItems, &iCurrentItem, bRecurse);
  CSectionLoader::Unload("CXIMAGE");
	if (m_dlgProgress) m_dlgProgress->Close();
  Update(m_strDirectory);
}

bool CGUIWindowPictures::DoCreateFolderThumbs(CStdString &strFolder, int *iTotalItems, int *iCurrentItem, bool bRecurse)
{
	// we have to grab the new folder recursively, and run through it, generating the thumbs
	// calculate the number of items to take thumbs of
	VECFILEITEMS items;
	GetDirectory(strFolder, items);
	SortItems(items);
  CFileItemList itemlist(items); // will clean up everything

	*iTotalItems += CUtil::GetFileCount(items);

	// now run through and create the thumbs
	for (int i=0; i < (int)items.size();++i)
	{
		CFileItem* pItem=items[i];
	
		if (m_dlgProgress->IsCanceled())
		{
			items.erase(items.begin(), items.end());
			return false;
		}

		if (!pItem->m_bIsFolder)
		{	
			(*iCurrentItem)++;
			WCHAR wstrProgress[128];
			WCHAR wstrFile[128];
			swprintf(wstrProgress,L"   progress:%i/%i", *iCurrentItem,*iTotalItems );
			swprintf(wstrFile,L"   picture:%S", pItem->GetLabel().c_str() );

			if (m_dlgProgress)
			{
				m_dlgProgress->SetLine(0, wstrFile);
				m_dlgProgress->SetLine(1, wstrProgress);
				m_dlgProgress->SetLine(2, L"");
				m_dlgProgress->Progress();
				if ( m_dlgProgress->IsCanceled() ) 
				{
					items.erase(items.begin(), items.end());
					return false;
				}
			}
			CPicture picture;
			picture.CreateThumnail(pItem->m_strPath);
		}
		else
		{	// a folder, let's call us again recursively
			if (pItem->GetLabel() != "..")
			{
				if (bRecurse && !DoCreateFolderThumbs(pItem->m_strPath, iTotalItems, iCurrentItem, bRecurse))
				{
					items.erase(items.begin(), items.end());
					return false;
				}
			}
		}
  }
	// create the folder thumb by choosing 4 random thumbs within the folder and putting
	// them into one thumb.
	int iNumFiles = CUtil::GetFileCount(items);
	if (iNumFiles == 0)
	{	// no thumbs in the folder
		return true;
	}
	int thumbs[4];
	if (iNumFiles > 4)
	{	// choose 4 random thumbs
		int i = 0;
		while (i<4)
		{
			int thumbnum = rand() % iNumFiles;
			bool bFoundNew = true;
			for (int j=0; j<=i; j++)
			{
				if (thumbnum == thumbs[j])
				{
					bFoundNew = false;
				}
			}
			if (bFoundNew)
				thumbs[i++] = thumbnum;
		}
	}
	else
	{
		for (int i=0; i<iNumFiles; i++)
			thumbs[i] = i;
		for (int i=iNumFiles; i<4; i++)
			thumbs[i] = -1;
	}
	// ok, now we've got the files to get the thumbs from, lets create it...
	// we basically load the 4 thumbs, resample to 62x62 pixels, and add them
	CStdString strFiles[4];
	int n = 0;
	for (int i=0; i < (int)items.size(); i++)
	{
		if (n>=4) break;
		if (!items[i]->m_bIsFolder)
		{
			if (thumbs[n] >= 0)
				strFiles[n] = items[i]->m_strPath;
			else
				strFiles[n] = "";
			n++;
		}
	}
	CPicture pic;
	pic.CreateFolderThumb(strFolder, strFiles);
	return true;
}

void CGUIWindowPictures::Render()
{
	CGUIWindow::Render();
}


int CGUIWindowPictures::GetSelectedItem()
{
	int iControl;
	bool bViewAsIcon=false;

  if ( ViewByIcon()) 
		iControl=CONTROL_THUMBS;
  else
		iControl=CONTROL_LIST;

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
  g_graphicsContext.SendMessage(msg);         
  int iItem=msg.GetParam1();
	return iItem;
}


void CGUIWindowPictures::GoParentFolder()
{
	CStdString strPath=m_strParentPath;
	Update(strPath);
}

bool CGUIWindowPictures::ViewByIcon()
{
  if ( m_strDirectory.IsEmpty() )
  {
	  if (m_iViewAsIconsRoot != VIEW_AS_LIST) return true;
  }
  else
  {
 	  if (m_iViewAsIcons != VIEW_AS_LIST) return true;
  }
  return false;
}

bool CGUIWindowPictures::ViewByLargeIcon()
{
  if ( m_strDirectory.IsEmpty() )
  {
	  if (m_iViewAsIconsRoot == VIEW_AS_LARGEICONS) return true;
  }
  else
  {
	  if (m_iViewAsIcons== VIEW_AS_LARGEICONS) return true;
  }
  return false;
}

void CGUIWindowPictures::UpdateThumbPanel()
{
  //int iItem=GetSelectedItem(); 

	CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
	if (pControl)
    pControl->ShowBigIcons(ViewByLargeIcon());

  //if (iItem>-1)
  //{
  //  CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem);
  //  CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem);
  //}
}

/// \brief Build a directory history string
/// \param pItem Item to build the history string from
/// \param strHistoryString History string build as return value
void CGUIWindowPictures::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
	if (pItem->m_bIsShareOrDrive)
	{
		//	We are in the virual directory

		//	History string of the DVD drive
		//	must be handel separately
		if (pItem->m_iDriveType==SHARE_TYPE_DVD)
		{
			//	Remove disc label from item label
			//	and use as history string, m_strPath
			//	can change for new discs
			CStdString strLabel=pItem->GetLabel();
			int nPosOpen=strLabel.Find('(');
			int nPosClose=strLabel.ReverseFind(')');
			if (nPosOpen>-1 && nPosClose>-1 && nPosClose>nPosOpen)
			{
				strLabel.Delete(nPosOpen+1, (nPosClose)-(nPosOpen+1));
				strHistoryString=strLabel;
			}
			else
				strHistoryString=strLabel;
		}
		else
		{
			//	Other items in virual directory
			CStdString strPath=pItem->m_strPath;
			while (CUtil::HasSlashAtEnd(strPath))
				strPath.Delete(strPath.size()-1);

			strHistoryString=pItem->GetLabel()+strPath;
		}
	}
	else
	{
		//	Normal directory items
		strHistoryString=pItem->m_strPath;

		if (CUtil::HasSlashAtEnd(strHistoryString))
			strHistoryString.Delete(strHistoryString.size()-1);
	}
}

void CGUIWindowPictures::SetHistoryForPath(const CStdString& strDirectory)
{
	if (!strDirectory.IsEmpty())
	{
		//	Build the directory history for default path
		CStdString strPath, strParentPath;
		strPath=strDirectory;
		VECFILEITEMS items;
		CFileItemList itemlist(items);
		GetDirectory("", items);

		while (CUtil::GetParentPath(strPath, strParentPath))
		{
			bool bSet=false;
			for (int i=0; i<(int)items.size(); ++i)
			{
				CFileItem* pItem=items[i];
				while (CUtil::HasSlashAtEnd(pItem->m_strPath))
					pItem->m_strPath.Delete(pItem->m_strPath.size()-1);
				if (pItem->m_strPath==strPath)
				{
					CStdString strHistory;
					GetDirectoryHistoryString(pItem, strHistory);
					m_history.Set(strHistory, "");
					return;
				}
			}

			m_history.Set(strPath, strParentPath);
			strPath=strParentPath;
			while (CUtil::HasSlashAtEnd(strPath))
				strPath.Delete(strPath.size()-1);
		}
	}
}

void CGUIWindowPictures::GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)
{
	if (items.size() )
	{
		// cleanup items
		CFileItemList itemlist(items);
	}

	CStdString strParentPath;
	bool bParentExists=CUtil::GetParentPath(strDirectory, strParentPath);

	// check if current directory is a root share
	if ( !m_rootDir.IsShare(strDirectory) )
	{
		// no, do we got a parent dir?
		if ( bParentExists )
		{
			// yes
			if (!g_guiSettings.GetBool("Pictures.HideParentDirItems"))
			{
				CFileItem *pItem = new CFileItem("..");
				pItem->m_strPath=strParentPath;
				pItem->m_bIsFolder=true;
				pItem->m_bIsShareOrDrive=false;
				items.push_back(pItem);
			}
			m_strParentPath = strParentPath;
		}
	}
	else
	{
		// yes, this is the root of a share
		// add parent path to the virtual directory
		if (!g_guiSettings.GetBool("Pictures.HideParentDirItems"))
		{
			CFileItem *pItem = new CFileItem("..");
			pItem->m_strPath="";
			pItem->m_bIsShareOrDrive=false;
			pItem->m_bIsFolder=true;
			items.push_back(pItem);
		}
		m_strParentPath = "";
	}
	m_rootDir.GetDirectory(strDirectory,items);

}

void CGUIWindowPictures::OnPopupMenu(int iItem)
{
	// mark the item
	m_vecItems[iItem]->Select(true);
	// calculate our position
	int iPosX=200;
	int iPosY=100;
	const CGUIControl *pList = GetControl(CONTROL_LIST);
	if (pList)
	{
		iPosX = pList->GetXPosition()+pList->GetWidth()/2;
		iPosY = pList->GetYPosition()+pList->GetHeight()/2;
	}	
	if ( m_strDirectory.IsEmpty() )
	{
		// and do the popup menu
		if (CGUIDialogContextMenu::BookmarksMenu("pictures", m_vecItems[iItem]->GetLabel(), m_vecItems[iItem]->m_strPath, iPosX, iPosY))
		{
			m_rootDir.SetShares(g_settings.m_vecMyPictureShares);
			Update(m_strDirectory);
			return;
		}
	}
	else
	{
		// popup the context menu
		CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
		if (!pMenu) return;
		// clean any buttons not needed
		pMenu->ClearButtons();
		// add the needed buttons
		pMenu->AddButton(13315);	// Create Thumbnails
		pMenu->AddButton(13316);	// Recursive Thumbnails
		pMenu->AddButton(13317);	// View Slideshow
		if (CUtil::GetFileCount(m_vecItems)==0)
		{
			pMenu->EnableButton(1, false);
			pMenu->EnableButton(3, false);
		}
		pMenu->AddButton(13318);	// Recursive Slideshow
		pMenu->AddButton(5);			// "Settings"

		// position it correctly
		pMenu->SetPosition(iPosX-pMenu->GetWidth()/2, iPosY-pMenu->GetHeight()/2);
		pMenu->DoModal(GetID());
		switch (pMenu->GetButton())
		{
		case 1:	// Create thumb(s)
			CreateFolderThumbs();
			break;
		case 2:	// recursive thumb(s)
			CreateFolderThumbs(true);
			break;
		case 3:	// slideshow
			OnSlideShow(m_vecItems[iItem]->m_strPath);
			break;
		case 4:	// recursive slideshow
			OnSlideShowRecursive(m_vecItems[iItem]->m_strPath);
			break;
		case 5:	// go to pictures settings
			m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPICTURES);
			return;
			break;
		}
	}
	m_vecItems[iItem]->Select(false);
}

void CGUIWindowPictures::SetViewMode(int iViewMode)
{
  if ( m_strDirectory.IsEmpty() )
	{
		m_iViewAsIconsRoot=iViewMode;
    g_stSettings.m_iMyPicturesRootViewAsIcons=iViewMode;
	}
  else
	{
    g_stSettings.m_iMyPicturesViewAsIcons=iViewMode;
		m_iViewAsIcons=iViewMode;
	}
}

int CGUIWindowPictures::SortMethod()
{
	if (m_strDirectory.IsEmpty())
	{
		if (g_stSettings.m_iMyPicturesRootSortMethod==0)
			return g_stSettings.m_iMyPicturesRootSortMethod+103;
		else
			return 498;	//	Sort by: Type
	}
	else
		return g_stSettings.m_iMyPicturesSortMethod+103;
}

bool CGUIWindowPictures::SortAscending()
{
	if (m_strDirectory.IsEmpty())
		return g_stSettings.m_bMyPicturesRootSortAscending;
	else
		return g_stSettings.m_bMyPicturesSortAscending;
}

void CGUIWindowPictures::SortItems(VECFILEITEMS& items)
{
	SSortPicturesByName sortmethod;
	if (m_strDirectory.IsEmpty())
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyPicturesRootSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyPicturesRootSortAscending;
	}
	else
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyPicturesSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyPicturesSortAscending;
	}
  sort(items.begin(), items.end(), sortmethod);
}
