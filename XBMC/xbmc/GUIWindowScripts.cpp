#include "guiwindowscripts.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "util.h"
#include "url.h"
#include "utils/imdb.h"
#include "GUIDialogOK.h"
#include "GUIDialogprogress.h"
#include "GUIDialogSelect.h" 
#include "application.h"
#include <algorithm>
#include "DetectDVDType.h"
#include "nfofile.h"
#include "filesystem/file.h"
#include "sectionLoader.h"
#include "lib/libPython/XBPython.h"

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4

#define CONTROL_LIST							10
#define CONTROL_THUMBS						11
#define CONTROL_LABELFILES        12

struct SSortScriptsByName
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
    CFileItem& rpStart=*pStart;
    CFileItem& rpEnd=*pEnd;
		if (rpStart.GetLabel()=="..") return true;
		if (rpEnd.GetLabel()=="..") return false;
		bool bGreater=true;
		if (g_stSettings.m_bScriptsSortAscending) bGreater=false;
    if ( rpStart.m_bIsFolder   == rpEnd.m_bIsFolder)
		{
			char szfilename1[1024];
			char szfilename2[1024];

			switch ( g_stSettings.m_bScriptsSortMethod ) 
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

			if (g_stSettings.m_bScriptsSortAscending)
				return (strcmp(szfilename1,szfilename2)<0);
			else
				return (strcmp(szfilename1,szfilename2)>=0);
		}
    if (!rpStart.m_bIsFolder) return false;
		return true;
	}
};


CGUIWindowScripts::CGUIWindowScripts()
:CGUIWindow(0)
{
	m_bDVDDiscChanged = false;
	m_bDVDDiscEjected = false;
	m_strDirectory="?";
	scriptSize = 0;
}

CGUIWindowScripts::~CGUIWindowScripts()
{
}


void CGUIWindowScripts::OnKey(const CKey& key)
{

  if (key.IsButton())
  {
		if ( key.GetButtonCode() == KEY_BUTTON_B )
		{
			GoParentFolder();
			return;
		}

    if ( key.GetButtonCode() == KEY_BUTTON_BACK  || key.GetButtonCode() == KEY_REMOTE_BACK)
    {
      m_gWindowManager.ActivateWindow(0); // back 2 home
      return;
    }
  }
  CGUIWindow::OnKey(key);
}

bool CGUIWindowScripts::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
	case GUI_MSG_DVDDRIVE_EJECTED_CD:
		//	Message send by a thread.
		//	We have to process it later
		//	(in Render()), if we have shares
		//	visible
		m_bDVDDiscEjected = true;
		break;
	case GUI_MSG_DVDDRIVE_CHANGED_CD:
		//	Message send by a thread.
		//	We have to process it later
		//	(in Render()), if we have shares
		//	visible
		if ( m_strDirectory.IsEmpty() )
			m_bDVDDiscChanged = true;
		break;
    case GUI_MSG_WINDOW_DEINIT:
			Clear();
    break;

    case GUI_MSG_WINDOW_INIT:
		{
			if (m_strDirectory=="?") m_strDirectory="";//g_stSettings.m_szDefaultScripts;

			m_rootDir.SetMask("*.py");
			
			CShare share;
			share.strName = "Q Drive";
			share.strPath = "Q:\\";
			share.m_iBufferSize = 0;
			share.m_iDriveType = SHARE_TYPE_LOCAL;
			shares.push_back(share);

			m_rootDir.SetShares(shares);//g_settings.m_vecScriptShares);
			Update(m_strDirectory);
		}
		break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if (iControl==CONTROL_BTNVIEWASICONS)
      {
		  if ( m_strDirectory.IsEmpty() )
		    g_stSettings.m_bScriptsRootViewAsIcons=!g_stSettings.m_bScriptsRootViewAsIcons;
		  else
		    g_stSettings.m_bScriptsViewAsIcons=!g_stSettings.m_bScriptsViewAsIcons;

				g_settings.Save();
        UpdateButtons();
      }
      else if (iControl==CONTROL_BTNSORTBY) // sort by
      {
        g_stSettings.m_bScriptsSortMethod++;
        if (g_stSettings.m_bScriptsSortMethod >=3) g_stSettings.m_bScriptsSortMethod=0;
				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
        g_stSettings.m_bScriptsSortAscending=!g_stSettings.m_bScriptsSortAscending;
				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
      {
         // get selected item
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
        g_graphicsContext.SendMessage(msg);         
        int iItem=msg.GetParam1();
        int iKey=message.GetParam1();
        if (iKey==KEY_BUTTON_A || iKey==KEY_REMOTE_SELECT)
				{
					OnClick(iItem);
				}
      }
    }
		break;
	}
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowScripts::UpdateButtons()
{

	SET_CONTROL_HIDDEN(GetID(), CONTROL_LIST);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_THUMBS);
	bool bViewIcon = false;
	if ( m_strDirectory.IsEmpty() ) {
		bViewIcon = g_stSettings.m_bScriptsRootViewAsIcons;
	}
	else {
		bViewIcon = g_stSettings.m_bScriptsViewAsIcons;
	}
   if (bViewIcon) 
    {
      SET_CONTROL_VISIBLE(GetID(), CONTROL_THUMBS);
    }
    else
    {
      SET_CONTROL_VISIBLE(GetID(), CONTROL_LIST);
    }

    int iString=101;
    if (!bViewIcon) 
    {
      iString=100;
    }
		SET_CONTROL_LABEL(GetID(), CONTROL_BTNVIEWASICONS,iString);
		SET_CONTROL_LABEL(GetID(), CONTROL_BTNSORTBY,g_stSettings.m_bScriptsSortMethod+103);

    if ( g_stSettings.m_bScriptsSortAscending)
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

void CGUIWindowScripts::Clear()
{
  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    delete pItem;
  }
  m_vecItems.erase(m_vecItems.begin(),m_vecItems.end() );
}

void CGUIWindowScripts::OnSort()
{
	CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg);         

  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    if (g_stSettings.m_bScriptsSortMethod==0||g_stSettings.m_bScriptsSortMethod==2)
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

  sort(m_vecItems.begin(), m_vecItems.end(), SSortScriptsByName());

  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg);    
    CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg2);         
  }
}

void CGUIWindowScripts::Update(const CStdString &strDirectory)
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
			m_vecItems.push_back(pItem);
		}
  }
  else
  {
		// yes, this is the root of a share
		// add parent path to the virtual directory
	  CFileItem *pItem = new CFileItem("..");
	  pItem->m_strPath="";
    pItem->m_bIsShareOrDrive=false;
	  pItem->m_bIsFolder=true;
	  m_vecItems.push_back(pItem);
  }

  m_strDirectory=strDirectory;
	m_rootDir.GetDirectory(strDirectory,m_vecItems);
	CUtil::SetThumbs(m_vecItems);
	CUtil::FillInDefaultIcons(m_vecItems);

	/* check if any python scripts are running. If true, place "(Running)" after the item.
	 * since stopping a script can take up to 10 seconds or more,we display 'stopping'
	 * after the filename for now.
	 */
	if (g_sectionLoader.IsLoaded("PYTHON") &&	g_application.m_pPythonParser)
	{
		int iSize = g_application.ScriptsSize();
		for (int i=0; i < iSize; i++)
		{
			int id = g_application.GetScriptId(i);
			if (g_application.m_pPythonParser->isRunning(id))
			{
				char* filename = g_application.m_pPythonParser->getFileName(id);

				for (int i=0; i < (int)m_vecItems.size(); i++)
				{
					CFileItem* pItem=m_vecItems[i];
					if (pItem->m_strPath == filename)
					{
						char tstr[1024];
						strcpy(tstr, pItem->GetLabel());
						if (g_application.m_pPythonParser->isStopping(id))
							strcat(tstr, " (Stopping)");
						else
							strcat(tstr, " (Running)");
						pItem->SetLabel(tstr);
					}
				}
			}
		}
	}

  OnSort();
  UpdateButtons();

	strSelectedItem=m_history.Get(m_strDirectory);	

	bool bViewAsIcon = false;
	if ( m_strDirectory.IsEmpty() )
		bViewAsIcon = g_stSettings.m_bScriptsRootViewAsIcons;
	else
		bViewAsIcon = g_stSettings.m_bScriptsViewAsIcons;

	if ( bViewAsIcon ) {	
		SET_CONTROL_FOCUS(GetID(), CONTROL_THUMBS);
	}
	else {
		SET_CONTROL_FOCUS(GetID(), CONTROL_LIST);
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

int CGUIWindowScripts::GetSelectedItem()
{
	int iControl;
	bool bViewIcon = false;
	if ( m_strDirectory.IsEmpty() ) {
		bViewIcon = g_stSettings.m_bScriptsRootViewAsIcons;
	}
	else {
		bViewIcon = g_stSettings.m_bScriptsViewAsIcons;
	}
	if ( bViewIcon) 
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


void CGUIWindowScripts::OnClick(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
  CStdString strPath=pItem->m_strPath;

	CStdString strExtension;
	CUtil::GetExtension(pItem->m_strPath,strExtension);

  if (pItem->m_bIsFolder)
  {
		if ( pItem->m_bIsShareOrDrive ) 
		{
			if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
				return;
		}
    Update(strPath);
  }
	else
	{
		/* execute script...
		 * if script is already running do not run it again but stop it.
		 */
		if (g_application.m_pPythonParser && g_sectionLoader.IsLoaded("PYTHON"))
		{
			int id = g_application.m_pPythonParser->getScriptId(strPath);
			if (id != -1)
			{
				/* if we are here we already know that this script is running.
				 * But we will check it again to be sure :)
				 */
				if(g_application.m_pPythonParser->isRunning(id))
				{
					g_application.m_pPythonParser->stopScript(id);
					// update items
					int selectedItem = GetSelectedItem();
					Update(m_strDirectory);
					CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,selectedItem);
					CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,selectedItem);
					return;
				}
			}
		}
		g_application.ExecuteScript(strPath);
	}
}

bool CGUIWindowScripts::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
	if ( iDriveType == SHARE_TYPE_DVD ) {
		CDetectDVDMedia::WaitMediaReady();
		if ( !CDetectDVDMedia::IsDiscInDrive() ) {
			CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(2002);
			dlg->SetHeading( 218 );
			dlg->SetLine( 0, 219 );
			dlg->SetLine( 1, L"" );
			dlg->SetLine( 2, L"" );
			dlg->DoModal( GetID() );
			return false;
		}
	}
	else if ( iDriveType == SHARE_TYPE_REMOTE ) {
			// TODO: Handle not connected to a remote share
		if ( !CUtil::IsEthernetConnected() ) {
			CGUIDialogOK* dlg = (CGUIDialogOK*)m_gWindowManager.GetWindow(2002);
			dlg->SetHeading( 220 );
			dlg->SetLine( 0, 221 );
			dlg->SetLine( 1, L"" );
			dlg->SetLine( 2, L"" );
			dlg->DoModal( GetID() );
			return false;
		}
	}
	else
		return true;
  return true;
}

void CGUIWindowScripts::Render()
{
	//	Process GUI_MSG_DVDDRIVE_EJECTED_CD message, if we have shares
	if ( m_bDVDDiscEjected ) {
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
		m_bDVDDiscEjected = false;
	}

	//	Process GUI_MSG_DVDDRIVED_CHANGED_CD message, if we have shares
	if ( m_bDVDDiscChanged && m_strDirectory.IsEmpty() ) {
		int iItem = GetSelectedItem();
		Update( m_strDirectory );
		CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iItem)
		CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iItem)
		m_bDVDDiscChanged = false;
	}

	// update control_list / control_thumbs if one or more scripts have stopped / started
	if (g_sectionLoader.IsLoaded("PYTHON") && g_application.m_pPythonParser)
	{
		if(g_application.m_pPythonParser->scriptsSize() != scriptSize)
		{
			int selectedItem = GetSelectedItem();
			Update(m_strDirectory);
			CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,selectedItem);
			CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,selectedItem);
			scriptSize = g_application.m_pPythonParser->scriptsSize();
		}
	}
	else if (scriptSize != 0)
	{
		int selectedItem = GetSelectedItem();
		Update(m_strDirectory);
		CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,selectedItem);
		CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,selectedItem);
		scriptSize = 0;
	}

	CGUIWindow::Render();
}
void CGUIWindowScripts::GoParentFolder()
{
 	if (!m_vecItems.size()) return;
  CFileItem* pItem=m_vecItems[0];
  CStdString strPath=pItem->m_strPath;
  if (pItem->m_bIsFolder && pItem->GetLabel()=="..")
  {
    Update(strPath);
  }
  UpdateButtons();
}


