#include "guiwindowpictures.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "util.h"
#include "url.h"
#include "picture.h"
#include "application.h"
#include <algorithm>

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4

#define CONTROL_BTNSLIDESHOW			6
#define CONTROL_BTNCREATETHUMBS		7
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
		if (g_stSettings.m_bMyPicturesSortAscending) bGreater=false;
    if ( rpStart.m_bIsFolder   == rpEnd.m_bIsFolder)
		{
			char szfilename1[1024];
			char szfilename2[1024];

			switch ( g_stSettings.m_bMyPicturesSortMethod ) 
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

			if (g_stSettings.m_bMyPicturesSortAscending)
				return (strcmp(szfilename1,szfilename2)<0);
			else
				return (strcmp(szfilename1,szfilename2)>=0);
		}
    if (!rpStart.m_bIsFolder) return false;
		return true;
	}
};


CGUIWindowPictures::CGUIWindowPictures(void)
:CGUIWindow(0)
{
	m_strDirectory="";
}

CGUIWindowPictures::~CGUIWindowPictures(void)
{
}

void CGUIWindowPictures::OnKey(const CKey& key)
{
  if (m_slideShow.IsPlaying() )
  {
    m_slideShow.OnKey(key);
		if (!m_slideShow.IsPlaying())
		{
			g_application.EnableOverlay();
		}
    return;
  }
  else
  {
    if (key.IsButton())
    {
      if ( key.GetButtonCode() == KEY_BUTTON_BACK  || key.GetButtonCode() == KEY_REMOTE_BACK)
      {
        m_gWindowManager.ActivateWindow(0); // back 2 home
        return;
      }
    }
  }
  CGUIWindow::OnKey(key);
}

bool CGUIWindowPictures::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_DEINIT:
      Clear();
			m_slideShow.Reset();
    break;

    case GUI_MSG_WINDOW_INIT:
		{
			m_strDirectory=g_stSettings.m_szDefaultPictures;

			m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(101);
			m_rootDir.SetMask(g_stSettings.m_szMyPicturesExtensions);
			int iControl=CONTROL_LIST;
      if (!g_stSettings.m_bMyPicturesViewAsIcons) iControl=CONTROL_THUMBS;
			SET_CONTROL_HIDDEN(GetID(), iControl);

      if ( g_stSettings.m_bMyPicturesSortAscending)
      {
        CGUIMessage msg(GUI_MSG_DESELECTED,GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }
      else
      {
        CGUIMessage msg(GUI_MSG_SELECTED,GetID(), CONTROL_BTNSORTASC);
        g_graphicsContext.SendMessage(msg);
      }

			m_rootDir.SetShares(g_settings.m_vecMyPictureShares);
			Update(m_strDirectory);
		}
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if (iControl==CONTROL_BTNVIEWASICONS)
      {
				//CGUIDialog* pDialog=(CGUIDialog*)m_gWindowManager.GetWindow(100);
				//pDialog->DoModal(GetID());

        g_stSettings.m_bMyPicturesViewAsIcons=!g_stSettings.m_bMyPicturesViewAsIcons;
				g_settings.Save();
				
        UpdateButtons();
      }
      else if (iControl==CONTROL_BTNSORTBY) // sort by
      {
        g_stSettings.m_bMyPicturesSortMethod++;
        if (g_stSettings.m_bMyPicturesSortMethod >=3) g_stSettings.m_bMyPicturesSortMethod=0;
				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
        g_stSettings.m_bMyPicturesSortAscending=!g_stSettings.m_bMyPicturesSortAscending;
				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSLIDESHOW) // Slide Show
      {
				OnSlideShow();
      }
      else if (iControl==CONTROL_BTNCREATETHUMBS) // Create Thumbs
      {
				OnCreateThumbs();
      }
      else if (iControl==CONTROL_LIST||iControl==CONTROL_THUMBS)  // list/thumb control
      {
         // get selected item
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
        g_graphicsContext.SendMessage(msg);         
        int iItem=msg.GetParam1();
        OnClick(iItem);
        
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
    if (g_stSettings.m_bMyPicturesSortMethod==0||g_stSettings.m_bMyPicturesSortMethod==2)
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

  
  sort(m_vecItems.begin(), m_vecItems.end(), SSortPicturesByName());

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
  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    delete pItem;
  }
  m_vecItems.erase(m_vecItems.begin(),m_vecItems.end() );
}

void CGUIWindowPictures::UpdateButtons()
{

    if (g_stSettings.m_bMyPicturesViewAsIcons) 
    {
			SET_CONTROL_HIDDEN(GetID(), CONTROL_LIST);
			SET_CONTROL_VISIBLE(GetID(), CONTROL_THUMBS);
    }
    else
    {
			SET_CONTROL_HIDDEN(GetID(), CONTROL_THUMBS);
			SET_CONTROL_VISIBLE(GetID(), CONTROL_LIST);
    }

    int iString=101;
    if (!g_stSettings.m_bMyPicturesViewAsIcons) 
    {
      iString=100;
    }
		SET_CONTROL_LABEL(GetID(), CONTROL_BTNVIEWASICONS,iString);
		SET_CONTROL_LABEL(GetID(), CONTROL_BTNSORTBY,g_stSettings.m_bMyPicturesSortMethod+103);

    if ( g_stSettings.m_bMyPicturesSortAscending)
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
  OnSort();
  UpdateButtons();

	strSelectedItem=m_history.Get(m_strDirectory);	
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


void CGUIWindowPictures::OnClick(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
  CStdString strPath=pItem->m_strPath;
  if (pItem->m_bIsFolder)
  {
    Update(strPath);
  }
	else
	{
		// show picture
		OnShowPicture(strPath);
	}
}

void CGUIWindowPictures::OnShowPicture(const CStdString& strPicture)
{
	g_application.DisableOverlay();
  m_slideShow.Reset();
  for (int i=0; i < (int)m_vecItems.size();++i)
  {
    CFileItem* pItem=m_vecItems[i];
    if (!pItem->m_bIsFolder)
    {
      m_slideShow.Add(pItem->m_strPath);
    }
  }
  m_slideShow.Select(strPicture);
}

void CGUIWindowPictures::OnSlideShow()
{
	g_application.DisableOverlay();
	m_slideShow.Reset();
  for (int i=0; i < (int)m_vecItems.size();++i)
  {
    CFileItem* pItem=m_vecItems[i];
    if (!pItem->m_bIsFolder)
    {
      m_slideShow.Add(pItem->m_strPath);
    }
  }
  m_slideShow.StartSlideShow();
}

void CGUIWindowPictures::OnCreateThumbs()
{
  CPicture picture;
	m_dlgProgress->SetHeading(110);
	m_dlgProgress->StartModal(GetID());
  for (int i=0; i < (int)m_vecItems.size();++i)
  {
    CFileItem* pItem=m_vecItems[i];
    if (!pItem->m_bIsFolder)
    {
			WCHAR wstrProgress[128];
			WCHAR wstrFile[128];
      swprintf(wstrProgress,L"   progress:%i/%i", i, m_vecItems.size() );
      swprintf(wstrFile,L"   picture:%S", pItem->GetLabel().c_str() );

			m_dlgProgress->SetLine(0, wstrFile);
			m_dlgProgress->SetLine(1, wstrProgress);
			m_dlgProgress->SetLine(2, L"");
			m_dlgProgress->Progress();
      picture.CreateThumnail(pItem->m_strPath);
    }
  }
	m_dlgProgress->Close();
  Update(m_strDirectory);
}

void CGUIWindowPictures::Render()
{
	CGUIWindow::Render();
	m_slideShow.Render();
}


int CGUIWindowPictures::GetSelectedItem()
{
	int iControl;
	if ( g_stSettings.m_bMyPicturesViewAsIcons) 
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
