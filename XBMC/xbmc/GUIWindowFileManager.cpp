
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
	int item;
	int list = GetFocusedList();
  if (action.wID == ACTION_DELETE_ITEM)
  {
		if (CanDelete(list))
		{
			bool bDeselect = SelectItem(list, item);
			OnDelete(list);
			if (bDeselect) m_vecItems[list][item]->Select(false);
		}
    return;
  }
  if (action.wID == ACTION_COPY_ITEM)
  {
		if (CanCopy(list))
		{
			bool bDeselect = SelectItem(list, item);
			OnCopy(list);
			if (bDeselect) m_vecItems[list][item]->Select(false);
		}
    return;
  }
  if (action.wID == ACTION_MOVE_ITEM)
  {
		if (CanMove(list))
		{
			bool bDeselect = SelectItem(list, item);
			OnMove(list);
			if (bDeselect) m_vecItems[list][item]->Select(false);
		}
    return;
  }
	if (action.wID == ACTION_RENAME_ITEM)
	{
		if (CanRename(list))
		{
			bool bDeselect = SelectItem(list, item);
			OnRename(list);
			if (bDeselect) m_vecItems[list][item]->Select(false);
		}
		return;
	}
	if (action.wID == ACTION_PARENT_DIR)
	{
		GoParentFolder(list);
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
					int iItem = GetSelectedItem(i);
					Update(i, "");
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
					int iItem = GetSelectedItem(i);
					Update(i, "");
					CONTROL_SELECT_ITEM(GetID(), CONTROL_LEFT_LIST+i, iItem)
				}
			}
		}
		break;
    case GUI_MSG_WINDOW_DEINIT:
		{
			m_iLastControl=GetFocusedControl();
			m_iItemSelected=GetSelectedItem(m_iLastControl-CONTROL_LEFT_LIST);
			Clear();
			g_application.EnableOverlay();
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
				int iItem = GetSelectedItem(i);
				Update(i, m_strDirectory[i]);
				CONTROL_SELECT_ITEM(GetID(), CONTROL_LEFT_LIST+i, iItem)
			}

			g_application.DisableOverlay();
			return true;
		}
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();

      if (iControl==CONTROL_LEFT_LIST || iControl==CONTROL_RIGHT_LIST)  // list/thumb control
      {
				// get selected item
				int list = iControl - CONTROL_LEFT_LIST;
				int iItem = GetSelectedItem(list);
				int iAction=message.GetParam1();
				if (iAction==ACTION_HIGHLIGHT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
				{
					OnMark(list, iItem);
					if (!g_Mouse.IsActive())
					{
							//move to next item
							CGUIMessage msg(GUI_MSG_ITEM_SELECT,GetID(),iControl,iItem+1,0,NULL);
							g_graphicsContext.SendMessage(msg);         
					}
				}
        else if (iAction==ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_DOUBLE_CLICK)
        {
					OnClick(list, iItem);
				}
				else if (iAction==ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
				{
					bool bDeselect = SelectItem(list, iItem);
					if (m_strDirectory[list] == "")
					{
						OnBookmarksPopupMenu(list, iItem);
						return true;
					}
					// popup the context menu
					CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
					if (pMenu)
					{
						// clean any buttons not needed
						pMenu->ClearButtons();
						// add the needed buttons
						pMenu->AddButton(188);	// SelectAll
						pMenu->AddButton(118);	// Rename
						pMenu->AddButton(117);	// Delete
						pMenu->AddButton(115);	// Copy
						pMenu->AddButton(116);	// Move
						pMenu->AddButton(119);	// New Folder
						pMenu->EnableButton(1, true);
						pMenu->EnableButton(2, CanRename(list));
						pMenu->EnableButton(3, CanDelete(list));
						pMenu->EnableButton(4, CanCopy(list));
						pMenu->EnableButton(5, CanMove(list));
						pMenu->EnableButton(6, CanNewFolder(list));
						// get the position we need...
						int iPosX=200;
						int iPosY=100;
						CGUIListControl *pList = (CGUIListControl *)GetControl(list+CONTROL_LEFT_LIST);
						if (pList)
						{
							iPosX = pList->GetXPosition()+(pList->GetWidth()-pMenu->GetWidth())/2;
							iPosY = pList->GetYPosition()+(pList->GetHeight()-pMenu->GetHeight())/2;
						}
						// ok, now figure out if it should be above or below this point...
						pMenu->SetPosition(iPosX,iPosY);
						pMenu->DoModal(GetID());
						switch (pMenu->GetButton())
						{
						case 1:
							OnSelectAll(list);
							break;
						case 2:
							OnRename(list);
							break;
						case 3:
							OnDelete(list);
							break;
						case 4:
							OnCopy(list);
							break;
						case 5:
							OnMove(list);
							break;
						case 6:
							OnNewFolder(list);
							break;
						default:
							if (bDeselect)
							{	// deselect item as we didn't do anything
								m_vecItems[list][iItem]->Select(false);
							}
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
	if (m_strDirectory[iList].IsEmpty())
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyFilesSourceRootSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyFilesSourceRootSortAscending;
	}
	else
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyFilesSourceSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyFilesSourceSortAscending;
	}

	for (int i=0; i < (int)m_vecItems[iList].size(); i++)
	{
		CFileItem* pItem=m_vecItems[iList][i];
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

	sort(m_vecItems[iList].begin(), m_vecItems[iList].end(), sortmethod);

//	UpdateControl(iList);
}

void CGUIWindowFileManager::Clear()
{
	CFileItemList itemleftlist(m_vecItems[0]); // will clean up everything
	CFileItemList itemrightlist(m_vecItems[1]); // will clean up everything
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
	for (int i=0; i<2; i++)
	{
		WCHAR wszText[20];
		const WCHAR* szText=g_localizeStrings.Get(127).c_str();
		int iItems=m_vecItems[i].size();
		if (iItems)
		{
			CFileItem* pItem=m_vecItems[i][0];
			if (pItem->GetLabel()=="..") iItems--;
		}
		swprintf(wszText,L"%i %s", iItems,szText);
		SET_CONTROL_LABEL(GetID(), CONTROL_NUMFILES_LEFT+i, wszText);
	}
}

void CGUIWindowFileManager::Update(int iList, const CStdString &strDirectory)
{
	// get selected item
	int iItem=GetSelectedItem(iList);
	CStdString strSelectedItem="";

	if (iItem >=0 && iItem < (int)m_vecItems[iList].size())
	{
		CFileItem* pItem=m_vecItems[iList][iItem];
		if (pItem->m_bIsFolder && pItem->GetLabel() != "..")
		{
			GetDirectoryHistoryString(pItem, strSelectedItem);
			m_history[iList].Set(strSelectedItem,m_strDirectory[iList]);
		}
	}

	{
		CFileItemList itemleftlist(m_vecItems[iList]); // will clean up everything
	}

	GetDirectory(iList, strDirectory, m_vecItems[iList]);
	m_strDirectory[iList]=strDirectory;
	// if we have a .tbn file, use itself as the thumb
	for (int i=0; i<(int)m_vecItems[iList].size(); i++)
	{
		CFileItem *pItem = m_vecItems[iList][i];
		CStdString strExtension;
		CUtil::GetExtension(pItem->m_strPath, strExtension);
		if (strExtension == ".tbn")
		{
			CUtil::SetThumb(pItem);
		}
	}
//	CUtil::SetThumbs(m_vecItems[iList]);
	CUtil::FillInDefaultIcons(m_vecItems[iList]);


	OnSort(iList);
  UpdateButtons();
	UpdateControl(iList);

	strSelectedItem=m_history[iList].Get(m_strDirectory[iList]);
	for (int i=0; i < (int)m_vecItems[iList].size(); ++i)
	{
		CFileItem* pItem=m_vecItems[iList][i];
		CStdString strHistory;
		GetDirectoryHistoryString(pItem, strHistory);
		if (strHistory==strSelectedItem)
		{
			CONTROL_SELECT_ITEM(GetID(), iList+CONTROL_LEFT_LIST, i);
			break;
		}
	}
}


void CGUIWindowFileManager::OnClick(int iList, int iItem)
{
	CFileItem *pItem=m_vecItems[iList][iItem];

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
 // UpdateButtons();
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
	if (CUtil::IsPicture(pItem->m_strPath))
	{
		CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
		if (!pSlideShow)
			return;
		if (g_application.IsPlayingVideo())
			g_application.StopPlaying();

		pSlideShow->Reset();
		pSlideShow->Add(pItem->m_strPath);
		pSlideShow->Select(pItem->m_strPath);
		m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
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
			int iList = GetFocusedList();
			int iItem = GetSelectedItem(iList);
			Update(iList, "");
			CONTROL_SELECT_ITEM(GetID(), iList+CONTROL_LEFT_LIST, iItem)
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
  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),iList+CONTROL_LEFT_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg);         

	for (int i=0; i < (int)m_vecItems[iList].size(); i++)
	{
		CFileItem* pItem=m_vecItems[iList][i];
		CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),iList+CONTROL_LEFT_LIST,0,0,(void*)pItem);
		g_graphicsContext.SendMessage(msg);
	}
}

void CGUIWindowFileManager::OnMark(int iList, int iItem)
{
	CFileItem* pItem = m_vecItems[iList][iItem];

  if (!pItem->m_bIsShareOrDrive)
  {
		if (pItem->GetLabel() != "..")
		{
			// MARK file
			pItem->Select(!pItem->IsSelected());
		}
  }

//	UpdateButtons();
}

bool CGUIWindowFileManager::DoProcessFile(int iAction, const CStdString& strFile, const CStdString& strDestFile)
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
	return !m_dlgProgress->IsCanceled();
}

bool CGUIWindowFileManager::DoProcessFolder(int iAction, const CStdString& strPath, const CStdString& strDestFile)
{
  VECFILEITEMS items;
	CFileItemList itemlist(items); 
  m_rootDir.GetDirectory(strPath,items);
  for (int i=0; i < (int)items.size(); i++)
  {
    CFileItem* pItem=items[i];
		pItem->Select(true);
  }

  if (!DoProcess(iAction,items,strDestFile)) return false;

	if (iAction==ACTION_MOVE)
	{
		CDirectory::Remove(strPath.c_str());
	}
	return true;
}

bool CGUIWindowFileManager::DoProcess(int iAction,VECFILEITEMS & items, const CStdString& strDestFile)
{
	bool bCancelled(false);
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
          if (!DoProcessFile(ACTION_CREATEFOLDER, strnewDestFile, strnewDestFile)) return false;
        }
        if (!DoProcessFolder(iAction,strCorrectedPath,strnewDestFile)) return false;
        if (iAction == ACTION_DELETE)
        {
           if (!DoProcessFile(ACTION_DELETEFOLDER, strCorrectedPath, pItem->m_strPath)) return false;
        }
      }
      else
      {
        if (!DoProcessFile(iAction,strCorrectedPath,strnewDestFile)) return false;
      }
    }
  }
	return true;
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
	
	DoProcess(ACTION_COPY, m_vecItems[iList], m_strDirectory[1-iList]);

  if (m_dlgProgress) m_dlgProgress->Close();
	
	Refresh(1-iList);
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
	
	DoProcess(ACTION_MOVE, m_vecItems[iList], m_strDirectory[1-iList]);

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

	DoProcess(ACTION_DELETE,m_vecItems[iList], m_strDirectory[iList]);

	if (m_dlgProgress) m_dlgProgress->Close();

  Refresh(iList);
}

void CGUIWindowFileManager::OnRename(int iList)
{
  CStdString strFile;
  for (int i=0; i < (int)m_vecItems[iList].size();++i)
  {
    CFileItem* pItem=m_vecItems[iList][i];
    if (pItem->IsSelected())
    {
      strFile=pItem->m_strPath;
      break;
    }
  }

  RenameFile(strFile);

  Refresh(iList);
}

void CGUIWindowFileManager::OnSelectAll(int iList)
{
  for (int i=0; i < (int)m_vecItems[iList].size();++i)
  {
    CFileItem* pItem=m_vecItems[iList][i];
    if (pItem->GetLabel() != "..")
    {
      pItem->Select(true);
    }
  }
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
		CStdString strNewPath = m_strDirectory[iList];
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
	Update(iList, m_strDirectory[iList]);

//	UpdateButtons();
 // UpdateControl(iList);

	while (nSel>(int)m_vecItems[iList].size())
		nSel--;

	CONTROL_SELECT_ITEM(GetID(), iList+CONTROL_LEFT_LIST, nSel);
}


void CGUIWindowFileManager::Refresh()
{
	int iList = GetFocusedList();
	int nSel=GetSelectedItem(iList);
	// update the list views
	Update(0, m_strDirectory[0]);
  Update(1, m_strDirectory[1]);

//	UpdateButtons();
//  UpdateControl(0);
//	UpdateControl(1);

	while (nSel>(int)m_vecItems[iList].size())
		nSel--;

	CONTROL_SELECT_ITEM(GetID(), iList+CONTROL_LEFT_LIST, nSel);
}

int CGUIWindowFileManager::GetSelectedItem(int iControl)
{
	CGUIListControl *pControl = (CGUIListControl *)GetControl(iControl+CONTROL_LEFT_LIST);
	if (!pControl) return -1;
	return pControl->GetSelectedItem();
}

void CGUIWindowFileManager::GoParentFolder(int iList)
{
	CStdString strPath=m_strParentPath[iList];
	Update(iList, strPath);
/*	CStdString strParentPath;
	if (CUtil::GetParentPath(m_strDirectory[iList], strParentPath))
	{
		Update(iList, strParentPath);
	}*/
/*	CFileItem *pItem;
	if (!m_vecItems[iList].size()) return;
	pItem=m_vecItems[iList][0];

	CStdString strPath=pItem->m_strPath;
	if (pItem->m_bIsFolder && pItem->GetLabel()=="..")
	{
		Update(iList, strPath);
	}*/
//	UpdateButtons();
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

void CGUIWindowFileManager::GetDirectory(int iList, const CStdString &strDirectory, VECFILEITEMS &items)
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
			if (!g_stSettings.m_bHideParentDirItems)
			{
				CFileItem *pItem = new CFileItem("..");
				pItem->m_strPath=strParentPath;
				pItem->m_bIsFolder=true;
				pItem->m_bIsShareOrDrive=false;
				items.push_back(pItem);
			}
			m_strParentPath[iList] = strParentPath;
		}
  }
  else
  {
		// yes, this is the root of a share
		// add parent path to the virtual directory
		if (!g_stSettings.m_bHideParentDirItems)
		{
			CFileItem *pItem = new CFileItem("..");
			pItem->m_strPath="";
			pItem->m_bIsFolder=true;
			pItem->m_bIsShareOrDrive=false;
			items.push_back(pItem);
		}
		m_strParentPath[iList] = "";
  }

  m_rootDir.GetDirectory(strDirectory, items);
}

bool CGUIWindowFileManager::CanRename(int iList)
{
	// TODO: Renaming of shares (requires writing to xboxmediacenter.xml)
	// this might be able to be done via the webserver code stuff...
	if (m_strDirectory[iList].IsEmpty()) return false;
	if (IsReadOnly(m_strDirectory[iList])) return false;

	return true;
}

bool CGUIWindowFileManager::CanCopy(int iList)
{
	// can't copy if the destination is not writeable, or if the source is a share!
	// TODO: Perhaps if the source is removeable media (DVD/CD etc.) we could
	// put ripping/backup in here.
	if (m_strDirectory[1-iList].IsEmpty()) return false;
	if (m_strDirectory[iList].IsEmpty()) return false;
	if (IsReadOnly(m_strDirectory[1-iList])) return false;
	return true;
}

bool CGUIWindowFileManager::CanMove(int iList)
{
	// can't move if the destination is not writeable, or if the source is a share or not writeable!
	if (m_strDirectory[0].IsEmpty() || IsReadOnly(m_strDirectory[0])) return false;
	if (m_strDirectory[1].IsEmpty() || IsReadOnly(m_strDirectory[1])) return false;
	return true;
}

bool CGUIWindowFileManager::CanDelete(int iList)
{
	if (m_strDirectory[iList].IsEmpty()) return false;
	if (IsReadOnly(m_strDirectory[iList])) return false;
	return true;
}

bool CGUIWindowFileManager::CanNewFolder(int iList)
{
	if (m_strDirectory[iList].IsEmpty()) return false;
	if (IsReadOnly(m_strDirectory[iList])) return false;
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

int CGUIWindowFileManager::NumSelected(int iList)
{
	int iSelectedItems = 0;
	for (int iItem=0; iItem < (int)m_vecItems[iList].size(); ++iItem)
  {
    if (m_vecItems[iList][iItem]->IsSelected()) iSelectedItems++;
	}
	return iSelectedItems;
}

int CGUIWindowFileManager::GetFocusedList() const
{
	return GetFocusedControl()-CONTROL_LEFT_LIST;
}

void CGUIWindowFileManager::OnBookmarksPopupMenu(int list, int item)
{
	// popup the context menu
	CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
	if (pMenu)
	{
		// clean any buttons not needed
		pMenu->ClearButtons();
		// add the needed buttons
		pMenu->AddButton(118);	// Rename
		pMenu->AddButton(748);	// Edit Path
		pMenu->AddButton(117);	// Delete
		pMenu->AddButton(749);	// Add Share
		// get the position we need...
		int iPosX=200;
		int iPosY=100;
		CGUIListControl *pList = (CGUIListControl *)GetControl(list+CONTROL_LEFT_LIST);
		if (pList)
		{
			iPosX = pList->GetXPosition()+(pList->GetWidth()-pMenu->GetWidth())/2;
			iPosY = pList->GetYPosition()+(pList->GetHeight()-pMenu->GetHeight())/2;
		}

		// ok, now figure out if it should be above or below this point...
		pMenu->SetPosition(iPosX,iPosY);
		pMenu->DoModal(GetID());
		switch (pMenu->GetButton())
		{
		case 1:
			{
				CStdString strNewLabel = m_vecItems[list][item]->GetLabel();
				CStdString strHeading = g_localizeStrings.Get(753);
				if (CGUIDialogKeyboard::ShowAndGetInput(strNewLabel, strHeading, false))
				{
					g_settings.UpdateBookmark("files", m_vecItems[list][item]->GetLabel(), strNewLabel, m_vecItems[list][item]->m_strPath);
					m_rootDir.SetShares(g_settings.m_vecMyFilesShares);
					if (m_strDirectory[1-list] == "")
						Refresh();
					else
						Refresh(list);
					return;
				}
			}
			break;
		case 2:
			{
				CStdString strNewPath = m_vecItems[list][item]->m_strPath;
				CStdString strHeading = g_localizeStrings.Get(752);
				if (CGUIDialogKeyboard::ShowAndGetInput(strNewPath, strHeading, false))
				{
					g_settings.UpdateBookmark("files", m_vecItems[list][item]->GetLabel(), m_vecItems[list][item]->GetLabel(), strNewPath);
					m_rootDir.SetShares(g_settings.m_vecMyFilesShares);
					if (m_strDirectory[1-list] == "")
						Refresh();
					else
						Refresh(list);
					return;
				}
			}
			break;
		case 3:
			{
				// prompt user
				CGUIDialogYesNo *pDlg = (CGUIDialogYesNo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
				if (pDlg)
				{
					pDlg->SetHeading(751);
					pDlg->SetLine(1,750);
					pDlg->DoModal(GetID());
					if (pDlg->IsConfirmed())
					{
						// delete this share
						g_settings.DeleteBookmark("files", m_vecItems[list][item]->GetLabel(), m_vecItems[list][item]->m_strPath);
						if (m_strDirectory[1-list] == "")
							Refresh();
						else
							Refresh(list);
						return;
					}
				}
			}
			break;
		case 4:
			{	// Add new share
				CStdString strNewPath;
				CStdString strHeading = g_localizeStrings.Get(752);	// Share Path
				if (CGUIDialogKeyboard::ShowAndGetInput(strNewPath, strHeading, false))
				{	// got a valid path
					CStdString strNewName;
					strHeading = g_localizeStrings.Get(753);	// Share Name
					if (CGUIDialogKeyboard::ShowAndGetInput(strNewName, strHeading, false))
					{
						g_settings.AddBookmark("files", strNewName, strNewPath);
						if (m_strDirectory[1-list] == "")
							Refresh();
						else
							Refresh(list);
						return;
					}
				}
			}
			break;
		}
	}
	m_vecItems[list][item]->Select(false);
}

// Highlights the item in the list under the cursor
// returns true if we should deselect the item, false otherwise
bool CGUIWindowFileManager::SelectItem(int list, int &item)
{
	// get the currently selected item in the list
	item = GetSelectedItem(list);
	// select the item if we need to
	if (!NumSelected(list) && m_vecItems[list][item]->GetLabel() != "..")
	{
		m_vecItems[list][item]->Select(true);
		return true;
	}
	return false;
}
