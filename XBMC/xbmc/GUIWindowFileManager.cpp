
#include "stdafx.h"
#include "application.h"
#include "guiwindowfilemanager.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "util.h"
#include "url.h"
#include "filesystem/file.h"
#include "filesystem/directory.h"
#include "picture.h"
#include <algorithm>
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "DetectDVDType.h"
#include "texturemanager.h"
#include "GUIDialogContextMenu.h"
#include "GUIListControl.h"

using namespace XFILE;

#define ACTION_COPY   1
#define ACTION_MOVE   2
#define ACTION_DELETE 3
#define ACTION_CREATEFOLDER 4
#define ACTION_DELETEFOLDER 5
#define ACTION_RENAME       6

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4
#define CONTROL_BTNNEWFOLDER      6
#define CONTROL_BTNSELECTALL			7
#define CONTROL_BTNCOPY           10
#define CONTROL_BTNMOVE           11
#define CONTROL_BTNDELETE         8
#define CONTROL_BTNRENAME         9

#define CONTROL_NUMFILES_LEFT			12
#define CONTROL_NUMFILES_RIGHT		13

#define CONTROL_LEFT_LIST					20
#define CONTROL_RIGHT_LIST				21

#define CONTROL_CURRENTDIRLABEL_LEFT		101
#define CONTROL_CURRENTDIRLABEL_RIGHT		102

struct SSortFilesByName
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


CGUIWindowFileManager::CGUIWindowFileManager(void)
:CGUIWindow(0)
{
  m_iItemSelected=-1;
	m_iLastControl=-1;
  m_strDirectory[0]="?";
  m_strDirectory[1]="?";
}

CGUIWindowFileManager::~CGUIWindowFileManager(void)
{
}

void CGUIWindowFileManager::OnAction(const CAction &action)
{
  if (action.wID == ACTION_DELETE_ITEM)
  {
    OnDelete(GetFocusedControl());
    return;
  }
  if (action.wID == ACTION_COPY_ITEM)
  {
		OnCopy(GetFocusedControl());
    return;
  }
  if (action.wID == ACTION_MOVE_ITEM)
  {
		OnMove(GetFocusedControl());
    return;
  }

	if (action.wID == ACTION_PARENT_DIR)
	{
		GoParentFolder(GetFocusedControl());
		return;
	}

  if (action.wID == ACTION_PREVIOUS_MENU)
	{
		m_gWindowManager.PreviousWindow();
		return;
	}
	CGUIWindow::OnAction(action);
}

bool CGUIWindowFileManager::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		{
			for (int i=0; i<2; i++)
			{
				if ( CUtil::IsCDDA( m_strDirectory[i] ) || CUtil::IsDVD( m_strDirectory[i] ) || CUtil::IsISO9660( m_strDirectory[i] ) )
				{
					//	Disc has changed and we are inside a DVD Drive share, get out of here :)
					m_strDirectory[i] = "";
				}
				if (m_strDirectory[i].IsEmpty())
				{
					int iItem = GetSelectedItem(CONTROL_LEFT_LIST+i);
					Update(CONTROL_LEFT_LIST+i, "");
					CONTROL_SELECT_ITEM(GetID(), CONTROL_LEFT_LIST+i, iItem)
				}
			}
		}
		break;
		case GUI_MSG_DVDDRIVE_CHANGED_CD:
		{
			for (int i=0; i<2; i++)
			{
				if (m_strDirectory[i].IsEmpty())
				{
					int iItem = GetSelectedItem(CONTROL_LEFT_LIST+i);
					Update(CONTROL_LEFT_LIST+i, "");
					CONTROL_SELECT_ITEM(GetID(), CONTROL_LEFT_LIST+i, iItem)
				}
			}
		}
		break;
    case GUI_MSG_WINDOW_DEINIT:
		{
			m_iLastControl=GetFocusedControl();
			m_iItemSelected=GetSelectedItem(m_iLastControl);
			Clear();
		}	
    break;

    case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);

			CGUIListControl *pControl = (CGUIListControl *)GetControl(CONTROL_LEFT_LIST);
			if (pControl) pControl->SetPageControlVisible(false);
			pControl = (CGUIListControl *)GetControl(CONTROL_RIGHT_LIST);
			if (pControl) pControl->SetPageControlVisible(false);

			m_rootDir.SetShares(g_settings.m_vecMyFilesShares);

			if (m_strDirectory[0]=="?")
			{
				m_strDirectory[0]=g_stSettings.m_szDefaultFiles;
			}
			if (m_strDirectory[1]=="?") m_strDirectory[1]="";

      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);      
      if (m_dlgProgress) m_dlgProgress->SetHeading(126);

			for (int i=0; i<2; i++)
			{
				int iItem = GetSelectedItem(CONTROL_LEFT_LIST+i);
				Update(CONTROL_LEFT_LIST+i, m_strDirectory[i]);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LEFT_LIST+i, iItem)
			}
			return true;
		}
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
/*      if (iControl==CONTROL_BTNVIEWASICONS)
      {

				if ( m_bViewSource ) 
				{
					if ( m_strSourceDirectory.IsEmpty() )
								g_stSettings.m_bMyFilesSourceRootViewAsIcons=!g_stSettings.m_bMyFilesSourceRootViewAsIcons;
					else
								g_stSettings.m_bMyFilesSourceViewAsIcons=!g_stSettings.m_bMyFilesSourceViewAsIcons;
				}
				else 
				{
					if ( m_strDestDirectory.IsEmpty() )
								g_stSettings.m_bMyFilesDestRootViewAsIcons=!g_stSettings.m_bMyFilesDestRootViewAsIcons;
					else
								g_stSettings.m_bMyFilesDestViewAsIcons=!g_stSettings.m_bMyFilesDestViewAsIcons;
				}
				g_settings.Save();
				
        UpdateButtons();
      }
      else if (iControl==CONTROL_BTNSORTBY) // sort by
      {
				if ( m_bViewSource ) 
				{
					if (m_strSourceDirectory.IsEmpty())
					{
						if (g_stSettings.m_iMyFilesSourceRootSortMethod==0)
							g_stSettings.m_iMyFilesSourceRootSortMethod=3;
						else
							g_stSettings.m_iMyFilesSourceRootSortMethod=0;
					}
					else
					{
						g_stSettings.m_iMyFilesSourceSortMethod++;
						if (g_stSettings.m_iMyFilesSourceSortMethod >=3) g_stSettings.m_iMyFilesSourceSortMethod=0;
					}
				}
				else
				{
					if (m_strDestDirectory.IsEmpty())
					{
						if (g_stSettings.m_iMyFilesDestRootSortMethod==0)
							g_stSettings.m_iMyFilesDestRootSortMethod=3;
						else
							g_stSettings.m_iMyFilesDestRootSortMethod=0;
					}
					else
					{
						g_stSettings.m_iMyFilesDestSortMethod++;
						if (g_stSettings.m_iMyFilesDestSortMethod >=3) g_stSettings.m_iMyFilesDestSortMethod=0;
					}
				}

				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
				if ( m_bViewSource ) 
				{
					if (m_strSourceDirectory.IsEmpty())
						g_stSettings.m_bMyFilesSourceRootSortAscending=!g_stSettings.m_bMyFilesSourceRootSortAscending;
					else
						g_stSettings.m_bMyFilesSourceSortAscending=!g_stSettings.m_bMyFilesSourceSortAscending;
				}
				else
				{
					if (m_strDestDirectory.IsEmpty())
						g_stSettings.m_bMyFilesDestRootSortAscending=!g_stSettings.m_bMyFilesDestRootSortAscending;
					else
						g_stSettings.m_bMyFilesDestRootSortAscending=!g_stSettings.m_bMyFilesDestRootSortAscending;
				}

				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNCOPY)
      {
        OnCopy();
      }
      else if (iControl==CONTROL_BTNMOVE)
      {
        OnMove();
      }
      else if (iControl==CONTROL_BTNDELETE)
      {
        OnDelete();
      }
      else if (iControl==CONTROL_BTNRENAME)
      {
        OnRename();
      }
      else if (iControl==CONTROL_BTNNEWFOLDER)
      {
        OnNewFolder();
      }
			else if (iControl==CONTROL_BTNSELECTALL)
			{
				if (m_bViewSource)
				{
					for (int iItem=0;iItem < (int)m_vecSourceItems.size(); ++iItem)
					{
						CFileItem* pItem=m_vecSourceItems[iItem];
						if (!pItem->m_bIsShareOrDrive)
						{
							if (pItem->GetLabel() != "..")
							{
								// MARK file
								pItem->Select(!pItem->IsSelected());
							}
						}
					}
				}
				else
				{
					for (int iItem=0;iItem < (int)m_vecDestItems.size(); ++iItem)
					{
						CFileItem* pItem=m_vecDestItems[iItem];
						if (!pItem->m_bIsShareOrDrive)
						{
							if (pItem->GetLabel() != "..")
							{
								// MARK file
								pItem->Select(!pItem->IsSelected());
							}
						}
					}
				}
				UpdateButtons();
			}
      */
      if (iControl==CONTROL_LEFT_LIST || iControl==CONTROL_RIGHT_LIST)  // list/thumb control
      {
				// get selected item
				int iItem = GetSelectedItem(iControl);
				int iAction=message.GetParam1();
				if (iAction==ACTION_HIGHLIGHT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
				{
					OnMark(iControl, iItem);
					if (!g_Mouse.IsActive())
					{
							//move to next item
							CGUIMessage msg(GUI_MSG_ITEM_SELECT,GetID(),iControl,iItem+1,0,NULL);
							g_graphicsContext.SendMessage(msg);         
					}
				}
        else if (iAction==ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_DOUBLE_CLICK)
        {
					OnClick(iControl, iItem);
				}
				else if (iAction==ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
				{
					if (iControl == CONTROL_LEFT_LIST)
						m_vecLeftItems[iItem]->Select(true);
					else
						m_vecRightItems[iItem]->Select(true);
					// popup the context menu
					CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
					if (pMenu)
					{
						// clean any buttons not needed
						pMenu->ClearButtons();
						// add the needed buttons
						pMenu->AddButton(118);	// Rename
						pMenu->AddButton(117);	// Delete
						pMenu->AddButton(115);	// Copy
						pMenu->AddButton(116);	// Move
						pMenu->AddButton(119);	// New Folder
						pMenu->EnableButton(1, CanRename(iControl));
						pMenu->EnableButton(2, CanDelete(iControl));
						pMenu->EnableButton(3, CanCopy(iControl));
						pMenu->EnableButton(4, CanMove(iControl));
						pMenu->EnableButton(5, CanNewFolder(iControl));
						// get the position we need...
						int iPosX=200;
						int iPosY=100;
						CGUIListControl *pList = (CGUIListControl *)GetControl(iControl);
						if (pList)
						{
							pList->GetPointFromItem(iPosX, iPosY);
							if (iPosY + pMenu->GetHeight() > pList->GetHeight())
								iPosY -= pMenu->GetHeight();
							iPosX += pList->GetXPosition();
							iPosX -= pMenu->GetWidth()/2;
							iPosY += pList->GetYPosition();
						}
						// ok, now figure out if it should be above or below this point...
						pMenu->SetPosition(iPosX,iPosY);
						pMenu->DoModal(GetID());
						switch (pMenu->GetButton())
						{
						case 1:
							OnRename(iControl);
							break;
						case 2:
							OnDelete(iControl);
							break;
						case 3:
							OnCopy(iControl);
							break;
						case 4:
							OnMove(iControl);
							break;
						case 5:
							OnNewFolder(iControl);
							break;
						}
					}
        }
      }
    }
    break;
	}
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowFileManager::OnSort(int iList)
{
  SSortFilesByName sortmethod;
	if (iList == CONTROL_LEFT_LIST)
	{
		if (m_strDirectory[0].IsEmpty())
		{
			sortmethod.m_iSortMethod=g_stSettings.m_iMyFilesSourceRootSortMethod;
			sortmethod.m_bSortAscending=g_stSettings.m_bMyFilesSourceRootSortAscending;
		}
		else
		{
			sortmethod.m_iSortMethod=g_stSettings.m_iMyFilesSourceSortMethod;
			sortmethod.m_bSortAscending=g_stSettings.m_bMyFilesSourceSortAscending;
		}

		for (int i=0; i < (int)m_vecLeftItems.size(); i++)
		{
			CFileItem* pItem=m_vecLeftItems[i];
			if (sortmethod.m_iSortMethod==0 || sortmethod.m_iSortMethod==2)
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

		sort(m_vecLeftItems.begin(), m_vecLeftItems.end(), sortmethod);
	}
	else
	{
		if (m_strDirectory[1].IsEmpty())
		{
			sortmethod.m_iSortMethod=g_stSettings.m_iMyFilesSourceRootSortMethod;
			sortmethod.m_bSortAscending=g_stSettings.m_bMyFilesSourceRootSortAscending;
		}
		else
		{
			sortmethod.m_iSortMethod=g_stSettings.m_iMyFilesSourceSortMethod;
			sortmethod.m_bSortAscending=g_stSettings.m_bMyFilesSourceSortAscending;
		}

		for (int i=0; i < (int)m_vecRightItems.size(); i++)
		{
			CFileItem* pItem=m_vecRightItems[i];
			if (sortmethod.m_iSortMethod==0 || sortmethod.m_iSortMethod==2)
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
		sort(m_vecRightItems.begin(), m_vecRightItems.end(), sortmethod);
	}
  UpdateControl(iList);
}

void CGUIWindowFileManager::Clear()
{
	CFileItemList itemleftlist(m_vecLeftItems); // will clean up everything
	CFileItemList itemrightlist(m_vecRightItems); // will clean up everything
}

void CGUIWindowFileManager::UpdateButtons()
{

/*
	//	Update sorting control
	bool bSortAscending=false;
	if ( m_bViewSource ) 
	{
		if (m_strSourceDirectory.IsEmpty())
			bSortAscending=g_stSettings.m_bMyFilesSourceRootSortAscending;
		else
			bSortAscending=g_stSettings.m_bMyFilesSourceSortAscending;
	}
	else
	{
		if (m_strDestDirectory.IsEmpty())
			bSortAscending=g_stSettings.m_bMyFilesDestRootSortAscending;
		else
			bSortAscending=g_stSettings.m_bMyFilesDestSortAscending;
	}

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

*/
	// update our current directory labels
	CStdString strDir;
	CUtil::GetFileAndProtocol(m_strDirectory[0],strDir);
	SET_CONTROL_LABEL(GetID(), CONTROL_CURRENTDIRLABEL_LEFT,strDir);
	CUtil::GetFileAndProtocol(m_strDirectory[1],strDir);
	SET_CONTROL_LABEL(GetID(), CONTROL_CURRENTDIRLABEL_RIGHT,strDir);

	// update the number of items in each list
	{
		WCHAR wszText[20];
		const WCHAR* szText=g_localizeStrings.Get(127).c_str();
		int iItems=m_vecLeftItems.size();
		if (iItems)
		{
			CFileItem* pItem=m_vecLeftItems[0];
			if (pItem->GetLabel()=="..") iItems--;
		}
		swprintf(wszText,L"%i %s", iItems,szText);
		SET_CONTROL_LABEL(GetID(), CONTROL_NUMFILES_LEFT,wszText);
	}
	{
		WCHAR wszText[20];
		const WCHAR* szText=g_localizeStrings.Get(127).c_str();
		int iItems=m_vecRightItems.size();
		if (iItems)
		{
			CFileItem* pItem=m_vecRightItems[0];
			if (pItem->GetLabel()=="..") iItems--;
		}
		swprintf(wszText,L"%i %s", iItems,szText);
		SET_CONTROL_LABEL(GetID(), CONTROL_NUMFILES_RIGHT,wszText);
	}
/*
	//	Have we used a button for copy, moving...?
	int nID=GetFocusedControl();
	if (nID==CONTROL_BTNCOPY||nID==CONTROL_BTNMOVE||nID==CONTROL_BTNDELETE||nID==CONTROL_BTNRENAME)
	{
		const CGUIControl* pControl=GetControl(nID);
		if (pControl)
		{
			//	After selected list control items are
			//	deselected the previous control must get 
			//	the focus, since the button is not visible
			//	anymore.
			if (!pControl->IsVisible())
				SelectPreviousControl();
		}
	}*/
}

void CGUIWindowFileManager::Update(int iList, const CStdString &strDirectory)
{
	// get selected item
	int iItem=GetSelectedItem(iList);
	CStdString strSelectedItem="";
	if (iList == CONTROL_LEFT_LIST)
	{
		if (iItem >=0 && iItem < (int)m_vecLeftItems.size())
		{
			CFileItem* pItem=m_vecLeftItems[iItem];
			if (pItem->m_bIsFolder && pItem->GetLabel() != "..")
			{
				GetDirectoryHistoryString(pItem, strSelectedItem);
				m_history[0].Set(strSelectedItem,m_strDirectory[0]);
			}
		}
	}
	else
	{
		if (iItem >=0 && iItem < (int)m_vecRightItems.size())
		{
			CFileItem* pItem=m_vecRightItems[iItem];
			if (pItem->m_bIsFolder && pItem->GetLabel() != "..")
			{
				GetDirectoryHistoryString(pItem, strSelectedItem);
				m_history[1].Set(strSelectedItem,m_strDirectory[1]);
			}
		}
	}

	if (iList == CONTROL_LEFT_LIST)
		CFileItemList itemleftlist(m_vecLeftItems); // will clean up everything
	else
		CFileItemList itemrightlist(m_vecRightItems); // will clean up everything

	if (iList == CONTROL_LEFT_LIST)
	{
		GetDirectory(strDirectory, m_vecLeftItems);
		m_strDirectory[0]=strDirectory;
		CUtil::SetThumbs(m_vecLeftItems);
		CUtil::FillInDefaultIcons(m_vecLeftItems);
	}
	else
	{
	 	GetDirectory(strDirectory, m_vecRightItems);
    m_strDirectory[1]=strDirectory;
		CUtil::SetThumbs(m_vecRightItems);
		CUtil::FillInDefaultIcons(m_vecRightItems);
	}

	OnSort(iList);
  UpdateButtons();

	if (iList == CONTROL_LEFT_LIST)
	{
		strSelectedItem=m_history[0].Get(m_strDirectory[0]);
		for (int i=0; i < (int)m_vecLeftItems.size(); ++i)
		{
			CFileItem* pItem=m_vecLeftItems[i];
			CStdString strHistory;
			GetDirectoryHistoryString(pItem, strHistory);
			if (strHistory==strSelectedItem)
			{
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LEFT_LIST, i);
				break;
			}
		}
	}
	else
	{
		strSelectedItem=m_history[1].Get(m_strDirectory[1]);	
		for (int i=0; i < (int)m_vecRightItems.size(); ++i)
		{
			CFileItem* pItem=m_vecRightItems[i];
			CStdString strHistory;
			GetDirectoryHistoryString(pItem, strHistory);
			if (strHistory==strSelectedItem)
			{
				CONTROL_SELECT_ITEM(GetID(), CONTROL_RIGHT_LIST, i);
				break;
			}
		}
	}
}


void CGUIWindowFileManager::OnClick(int iList, int iItem)
{
	CFileItem *pItem;
  if (iList == CONTROL_LEFT_LIST)
    pItem=m_vecLeftItems[iItem];
	else
		pItem=m_vecRightItems[iItem];
	
  CStdString strPath=pItem->m_strPath;
  if (pItem->m_bIsFolder)
  {
    m_iItemSelected=-1;
    if ( pItem->m_bIsShareOrDrive ) 
		{
      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
          return;
		}
		Update(iList, strPath);
  }
  else
  {
    m_iItemSelected=GetSelectedItem(iList);
    OnStart(pItem);
    return;
  }
  UpdateButtons();
}

void CGUIWindowFileManager::OnStart(CFileItem *pItem)
{
	if (CUtil::IsAudio(pItem->m_strPath) || CUtil::IsVideo(pItem->m_strPath))
  {
    g_application.PlayFile(*pItem);
    return;
  }
  if (CUtil::IsPythonScript(pItem->m_strPath))
  {
		g_pythonParser.evalFile(pItem->m_strPath.c_str());
    return;
  }
  if (CUtil::IsXBE(pItem->m_strPath))
  {
		CUtil::RunXBE(pItem->m_strPath.c_str());
  }

}

bool CGUIWindowFileManager::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
	if ( iDriveType == SHARE_TYPE_DVD )
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
			int iList = GetFocusedControl();
			int iItem = GetSelectedItem(iList);
			Update(iList, "");
			CONTROL_SELECT_ITEM(GetID(), iList, iItem)
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

void CGUIWindowFileManager::UpdateControl(int iList)
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),iList,0,0,NULL);
  g_graphicsContext.SendMessage(msg);         

	if (iList == CONTROL_LEFT_LIST)
	{
		for (int i=0; i < (int)m_vecLeftItems.size(); i++)
		{
			CFileItem* pItem=m_vecLeftItems[i];
			CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LEFT_LIST,0,0,(void*)pItem);
			g_graphicsContext.SendMessage(msg);    
		}
	}
	else
	{
		for (int i=0; i < (int)m_vecRightItems.size(); i++)
		{
			CFileItem* pItem=m_vecRightItems[i];
			CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_RIGHT_LIST,0,0,(void*)pItem);
			g_graphicsContext.SendMessage(msg);    
		}
	}
}

void CGUIWindowFileManager::OnMark(int iList, int iItem)
{
	CFileItem* pItem;
	if (iList == CONTROL_LEFT_LIST)
		pItem = m_vecLeftItems[iItem];
	else
		pItem = m_vecRightItems[iItem];

  if (!pItem->m_bIsShareOrDrive)
  {
		if (pItem->GetLabel() != "..")
		{
			// MARK file
			pItem->Select(!pItem->IsSelected());
		}
  }

	UpdateButtons();
}

void CGUIWindowFileManager::DoProcessFile(int iAction, const CStdString& strFile, const CStdString& strDestFile)
{
	CStdString strShortSourceFile;
	CStdString strShortDestFile;
	CUtil::GetFileAndProtocol(strFile, strShortSourceFile);
	CUtil::GetFileAndProtocol(strDestFile, strShortDestFile);
  switch (iAction)
  {
    case ACTION_COPY:
    {
      const WCHAR *szText=g_localizeStrings.Get(115).c_str();
      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0,115);
        m_dlgProgress->SetLine(1,strShortSourceFile);
        m_dlgProgress->SetLine(2,strShortDestFile);
			  m_dlgProgress->Progress();
      }
      CStdString strLog;
      strLog.Format("copy %s->%s\n", strFile.c_str(), strDestFile.c_str());
      OutputDebugString(strLog.c_str());

			CStdString strDestFileShortened = strDestFile;


			// shorten file if filename length > 42 chars
			CUtil::ShortenFileName(strDestFileShortened);
			for (int i=0; i < (int)strDestFileShortened.size(); ++i)
			{
				if (strDestFileShortened.GetAt(i) == ',') strDestFileShortened.SetAt(i,'_');
			}
      CFile file;
      file.Cache(strFile.c_str(), strDestFileShortened.c_str(), this, NULL);
    }
    break;

    
    case ACTION_MOVE:
    {
      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0,116);
        m_dlgProgress->SetLine(1,strShortSourceFile);
        m_dlgProgress->SetLine(2,strShortDestFile);
			  m_dlgProgress->Progress();
      }
      CFile file;
			if (strFile[1] == ':' && strFile[0] == strDestFile[0])
			{
				// quick move on same drive
				CFile::Rename(strFile.c_str(), strDestFile.c_str());
			}
			else
			{
				if (file.Cache(strFile.c_str(), strDestFile.c_str(), this, NULL ) )
				{
					CFile::Delete(strFile.c_str());
				}
			}
    }
    break;

    
    case ACTION_DELETE:
    {
      CStdString strLog;
      strLog.Format("delete %s\n", strFile.c_str());
      OutputDebugString(strLog.c_str());

      CFile::Delete(strFile.c_str());
      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0,117);
        m_dlgProgress->SetLine(1,strShortSourceFile);
        m_dlgProgress->SetLine(2,L"");
			  m_dlgProgress->Progress();
      }
    }
    break;

    case ACTION_DELETEFOLDER:
    {
      CStdString strLog;
      strLog.Format("delete folder %s\n", strFile.c_str());
      OutputDebugString(strLog.c_str());
      CDirectory::Remove(strFile);
      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0,117);
        m_dlgProgress->SetLine(1,strShortSourceFile);
        m_dlgProgress->SetLine(2,L"");
			  m_dlgProgress->Progress();
      }
    }
    break;
    case ACTION_CREATEFOLDER:
    {
      CStdString strLog;
      strLog.Format("create %s\n", strFile.c_str());
      OutputDebugString(strLog.c_str());
      CDirectory::Create(strFile);

      if (m_dlgProgress)
      {
        m_dlgProgress->SetLine(0,119);
        m_dlgProgress->SetLine(1,strShortSourceFile);
        m_dlgProgress->SetLine(2,L"");
			  m_dlgProgress->Progress();
      }
    }
    break;

    case ACTION_RENAME:
    {
		  RenameFile(strFile);
    }
    break;
  }
  if (m_dlgProgress) m_dlgProgress->Progress();
}

void CGUIWindowFileManager::DoProcessFolder(int iAction, const CStdString& strPath, const CStdString& strDestFile)
{
  VECFILEITEMS items;
	CFileItemList itemlist(items); 
  m_rootDir.GetDirectory(strPath,items);
  for (int i=0; i < (int)items.size(); i++)
  {
    CFileItem* pItem=items[i];
		pItem->Select(true);
  }

  DoProcess(iAction,items,strDestFile);

	if (iAction==ACTION_MOVE)
	{
		CDirectory::Remove(strPath.c_str());
	}
}

void CGUIWindowFileManager::DoProcess(int iAction,VECFILEITEMS & items, const CStdString& strDestFile)
{
  for (int iItem=0; iItem < (int)items.size(); ++iItem)
  {
    CFileItem* pItem=items[iItem];
		if (pItem->IsSelected())
    {
			CStdString strCorrectedPath=pItem->m_strPath;
			if (CUtil::HasSlashAtEnd(strCorrectedPath)) 
			{
				strCorrectedPath=strCorrectedPath.Left(strCorrectedPath.size()-1);
			}
			char* pFileName=CUtil::GetFileName(strCorrectedPath);
			CStdString strnewDestFile=strDestFile;
			if (!CUtil::HasSlashAtEnd(strnewDestFile) )
			{
				strnewDestFile+="\\";
			}
      strnewDestFile+=pFileName;
      if (pItem->m_bIsFolder)
      {
        // create folder on dest. drive
        
        if (iAction != ACTION_DELETE)
        {
          DoProcessFile(ACTION_CREATEFOLDER, strnewDestFile, strnewDestFile);
        }
        DoProcessFolder(iAction,strCorrectedPath,strnewDestFile);
        if (iAction == ACTION_DELETE)
        {
           DoProcessFile(ACTION_DELETEFOLDER, strCorrectedPath, pItem->m_strPath);
        }
      }
      else
      {
        DoProcessFile(iAction,strCorrectedPath,strnewDestFile);
      }
    }
  }
}

void CGUIWindowFileManager::OnCopy(int iList)
{
	CGUIDialogYesNo* pDialog= (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
	if (pDialog)
	{
		pDialog->SetHeading(120);
		pDialog->SetLine(0,123);
		pDialog->SetLine(1,L"");
		pDialog->SetLine(2,L"");
		pDialog->DoModal(GetID());
		if (!pDialog->IsConfirmed()) return;
	}

	if (m_dlgProgress)
	{
		m_dlgProgress->StartModal(GetID());
		m_dlgProgress->SetPercentage(0);
		m_dlgProgress->ShowProgressBar(true);
	}
	if (iList == CONTROL_LEFT_LIST)
		DoProcess(ACTION_COPY, m_vecLeftItems, m_strDirectory[1]);
	else
		DoProcess(ACTION_COPY, m_vecRightItems, m_strDirectory[0]);
  if (m_dlgProgress) m_dlgProgress->Close();
	
	int list = (iList == CONTROL_LEFT_LIST) ? CONTROL_RIGHT_LIST : CONTROL_LEFT_LIST;
	Refresh(list);
}

void CGUIWindowFileManager::OnMove(int iList)
{
  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(121);
    pDialog->SetLine(0,124);
    pDialog->SetLine(1,L"");
    pDialog->SetLine(2,L"");
    pDialog->DoModal(GetID());
    if (!pDialog->IsConfirmed()) return;
  }
  
  if (m_dlgProgress)
  {
    m_dlgProgress->StartModal(GetID());
	  m_dlgProgress->SetPercentage(0);
	  m_dlgProgress->ShowProgressBar(true);
  }
	if (iList == CONTROL_LEFT_LIST)
		DoProcess(ACTION_MOVE, m_vecLeftItems, m_strDirectory[1]);
	else
		DoProcess(ACTION_MOVE, m_vecRightItems, m_strDirectory[0]);

  if (m_dlgProgress) m_dlgProgress->Close();

  Refresh();
}

void CGUIWindowFileManager::OnDelete(int iList)
{
  CGUIDialogYesNo* pDialog= (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(122);
    pDialog->SetLine(0,125);
    pDialog->SetLine(1,L"");
    pDialog->SetLine(2,L"");
    pDialog->DoModal(GetID());
    if (!pDialog->IsConfirmed()) return;
  }
 
  if (m_dlgProgress) m_dlgProgress->StartModal(GetID());

	if (iList == CONTROL_LEFT_LIST)
		DoProcess(ACTION_DELETE,m_vecLeftItems, m_strDirectory[0]);
	else
		DoProcess(ACTION_DELETE,m_vecRightItems, m_strDirectory[1]);
  if (m_dlgProgress) m_dlgProgress->Close();

  Refresh(iList);
}

void CGUIWindowFileManager::OnRename(int iList)
{
  CStdString strFile;
  if (iList == CONTROL_LEFT_LIST)
  {
    for (int i=0; i < (int)m_vecLeftItems.size();++i)
    {
      CFileItem* pItem=m_vecLeftItems[i];
      if (pItem->IsSelected())
      {
        strFile=pItem->m_strPath;
        break;
      }
    }
  }
  else
  {
    for (int i=0; i < (int)m_vecRightItems.size();++i)
    {
      CFileItem* pItem=m_vecRightItems[i];
      if (pItem->IsSelected())
      {
        strFile=pItem->m_strPath;
        break;
      }
    }
  }
  RenameFile(strFile);

  Refresh(iList);
}

void CGUIWindowFileManager::RenameFile(const CStdString &strFile)
{
	CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
	if (pKeyboard)
	{
		CStdString strFileName=CUtil::GetFileName(strFile);
		CStdString strPath=strFile.Left(strFile.size()-strFileName.size());

		// setup keyboard
		pKeyboard->CenterWindow();
		pKeyboard->SetText(strFileName);
		pKeyboard->DoModal(m_gWindowManager.GetActiveWindow());
		pKeyboard->Close();	

		if (pKeyboard->IsDirty())
		{	// have text - update this.
			CStdString strNewFileName = pKeyboard->GetText();
			if (!strNewFileName.IsEmpty())
			{
				// need 2 rename it
				strPath += strNewFileName;
				CFile::Rename(strFile.c_str(), strPath.c_str());
			}
		}
	}
}

void CGUIWindowFileManager::OnNewFolder(int iList)
{
	CStdString strNewFolder = "";
	CGUIDialogKeyboard *pKeyboard = (CGUIDialogKeyboard*)m_gWindowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
	if (!pKeyboard) return;

	// setup keyboard
	pKeyboard->CenterWindow();
	pKeyboard->SetText(strNewFolder);
	pKeyboard->DoModal(m_gWindowManager.GetActiveWindow());
	pKeyboard->Close();	

	if (pKeyboard->IsDirty())
	{	// have text - update this.
		CStdString strNewPath;
		if (iList == CONTROL_LEFT_LIST) 
			strNewPath= m_strDirectory[0];
		else
			strNewPath= m_strDirectory[1];
		if (!CUtil::HasSlashAtEnd(strNewPath) ) strNewPath+="\\";
		CStdString strNewFolderName = pKeyboard->GetText();
		if (!strNewFolderName.IsEmpty())
		{
			strNewPath+=strNewFolderName;
			CDirectory::Create(strNewPath.c_str());
		}
	}

  Refresh(iList);
}

void CGUIWindowFileManager::Refresh(int iList)
{
	int nSel=GetSelectedItem(iList);
	// update the list views
	Update(iList, m_strDirectory[iList - CONTROL_LEFT_LIST]);

	UpdateButtons();
  UpdateControl(iList);

	if (iList == CONTROL_LEFT_LIST)
	{
		while (nSel>(int)m_vecLeftItems.size())
			nSel--;
	}
	else
	{
		while (nSel>(int)m_vecRightItems.size())
			nSel--;
	}

	CONTROL_SELECT_ITEM(GetID(), iList, nSel);
}


void CGUIWindowFileManager::Refresh()
{
	int iList = GetFocusedControl();
	int nSel=GetSelectedItem(iList);
	// update the list views
	Update(CONTROL_LEFT_LIST, m_strDirectory[0]);
  Update(CONTROL_RIGHT_LIST, m_strDirectory[1]);

	UpdateButtons();
  UpdateControl(CONTROL_LEFT_LIST);
	UpdateControl(CONTROL_RIGHT_LIST);

	if (iList == CONTROL_LEFT_LIST)
	{
		while (nSel>(int)m_vecLeftItems.size())
			nSel--;
	}
	else
	{
		while (nSel>(int)m_vecRightItems.size())
			nSel--;
	}

	CONTROL_SELECT_ITEM(GetID(), iList, nSel);
}

int CGUIWindowFileManager::GetSelectedItem(int iControl)
{
	CGUIListControl *pControl = (CGUIListControl *)GetControl(iControl);
	if (!pControl) return -1;
	return pControl->GetSelectedItem();
}

void CGUIWindowFileManager::GoParentFolder(int iList)
{
	CFileItem *pItem;
	if (iList == CONTROL_LEFT_LIST)
	{
		if (!m_vecLeftItems.size()) return;
		pItem=m_vecLeftItems[0];
	}
	else
	{
		if (!m_vecRightItems.size()) return;
		pItem=m_vecRightItems[0];
	}
	CStdString strPath=pItem->m_strPath;
	if (pItem->m_bIsFolder && pItem->GetLabel()=="..")
	{
		Update(iList, strPath);
	}
	UpdateButtons();
}

void CGUIWindowFileManager::Render()
{
	CGUIWindow::Render();
}

bool CGUIWindowFileManager::OnFileCallback(void* pContext, int ipercent)
{	
	if (m_dlgProgress) 
  {
    m_dlgProgress->SetPercentage(ipercent);
	  m_dlgProgress->Progress();
	  if (m_dlgProgress->IsCanceled()) return false;
  }
	return true;
}

/// \brief Build a directory history string
/// \param pItem Item to build the history string from
/// \param strHistoryString History string build as return value
void CGUIWindowFileManager::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
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

void CGUIWindowFileManager::GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)
{
  CStdString strParentPath;
	bool bParentExists=CUtil::GetParentPath(strDirectory, strParentPath);

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
	  pItem->m_bIsFolder=true;
    pItem->m_bIsShareOrDrive=false;
		items.push_back(pItem);
  }

  m_rootDir.GetDirectory(strDirectory, items);
}

bool CGUIWindowFileManager::CanRename(int iList)
{
	// TODO: Renaming of shares (requires writing to xboxmediacenter.xml)
	// this might be able to be done via the webserver code stuff...
	if (iList == CONTROL_LEFT_LIST)
	{
		if (m_strDirectory[0].IsEmpty()) return false;
		if (IsReadOnly(m_strDirectory[0])) return false;
	}
	else
	{
		if (m_strDirectory[1].IsEmpty()) return false;
		if (IsReadOnly(m_strDirectory[1])) return false;
	}
	return true;
}

bool CGUIWindowFileManager::CanCopy(int iList)
{
	// can't copy if the destination is not writeable, or if the source is a share!
	// TODO: Perhaps if the source is removeable media (DVD/CD etc.) we could
	// put ripping/backup in here.
	if (iList == CONTROL_LEFT_LIST)
	{
		if (m_strDirectory[1].IsEmpty()) return false;
		if (m_strDirectory[0].IsEmpty()) return false;
		if (IsReadOnly(m_strDirectory[1])) return false;
	}
	else
	{
		if (m_strDirectory[0].IsEmpty()) return false;
		if (m_strDirectory[1].IsEmpty()) return false;
		if (IsReadOnly(m_strDirectory[0])) return false;
	}
	return true;
}

bool CGUIWindowFileManager::CanMove(int iList)
{
	// can't move if the destination is not writeable, or if the source is a share or not writeable!
	if (m_strDirectory[1].IsEmpty()) return false;
	if (m_strDirectory[0].IsEmpty()) return false;
	if (IsReadOnly(m_strDirectory[1])) return false;
	if (IsReadOnly(m_strDirectory[0])) return false;
	return true;
}

bool CGUIWindowFileManager::CanDelete(int iList)
{
	if (iList == CONTROL_LEFT_LIST)
	{
		if (m_strDirectory[0].IsEmpty()) return false;
		if (IsReadOnly(m_strDirectory[0])) return false;
	}
	else
	{
		if (m_strDirectory[1].IsEmpty()) return false;
		if (IsReadOnly(m_strDirectory[1])) return false;
	}
	return true;
}

bool CGUIWindowFileManager::CanNewFolder(int iList)
{
	if (iList == CONTROL_LEFT_LIST)
	{
		if (m_strDirectory[0].IsEmpty()) return false;
		if (IsReadOnly(m_strDirectory[0])) return false;
	}
	else
	{
		if (m_strDirectory[1].IsEmpty()) return false;
		if (IsReadOnly(m_strDirectory[1])) return false;
	}
	return true;
}

bool CGUIWindowFileManager::IsReadOnly(const CStdString &strFile) const
{
	// check for dvd/cd media
	if (CUtil::IsDVD(strFile) || CUtil::IsCDDA(strFile) || CUtil::IsISO9660(strFile)) return true;
	// now check for remote shares (smb is writeable??)
	if (CUtil::IsSmb(strFile)) return false;
	// no other protocols are writeable??
	if (CUtil::IsRemote(strFile)) return true;
	return false;
}