#include "guiwindowprograms.h"
#include "localizestrings.h"
#include "GUIWindowManager.h" 
#include "util.h"
#include "Xbox\IoSupport.h"
#include "Xbox\Undocumented.h"
#include "crc32.h"
#include "settings.h"
#include "lib/cximage/ximage.h"
#include "Shortcut.h"
#include "guidialog.h"
#include "sectionLoader.h"
#include "application.h"

#include <algorithm>

#define CONTROL_BTNVIEWAS     2
#define CONTROL_BTNFLATTEN    3
#define CONTROL_BTNSORTMETHOD 4
#define CONTROL_BTNSORTASC    5
#define CONTROL_LIST	        7
#define CONTROL_THUMBS        8
#define CONTROL_LABELFILES    9

CGUIWindowPrograms::CGUIWindowPrograms(void)
:CGUIWindow(0)
{
  m_strDirectory="";
  
}

CGUIWindowPrograms::~CGUIWindowPrograms(void)
{
}

bool CGUIWindowPrograms::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_DEINIT:
      Clear();
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      if (m_strDirectory=="")
      {
        if (g_stSettings.m_szShortcutDirectory[0] != 0)
        {
          m_strDirectory=g_stSettings.m_szShortcutDirectory;
        }
        else
        {
          m_strDirectory="E:\\";
        }
      }
      // make controls 100-110 invisible...
			for (int i=100; i < 110; i++)
      {
        CGUIMessage msg(GUI_MSG_HIDDEN,GetID(), i);
        g_graphicsContext.SendMessage(msg);
      }
			// create bookmark buttons
      int iStartID=100;
      
			for (i=0; i < (int)g_settings.m_vecMyProgramsBookmarks.size(); ++i)
			{
				CShare& share = g_settings.m_vecMyProgramsBookmarks[i];
				
        CGUIMessage msg(GUI_MSG_VISIBLE,GetID(), i+iStartID);
        g_graphicsContext.SendMessage(msg);

				WCHAR wszText[1024];
				swprintf(wszText,L"%S", share.strName.c_str());
				CGUIMessage msg2(GUI_MSG_LABEL_SET,GetID(), i+iStartID,0,0,(void*)wszText);
        g_graphicsContext.SendMessage(msg2);
        
			}

      
      Update(m_strDirectory);
    }
    break;

    case GUI_MSG_SETFOCUS:
    {
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if (iControl==CONTROL_BTNVIEWAS)
      {
				//CGUIDialog* pDialog=(CGUIDialog*)m_gWindowManager.GetWindow(100);
				//pDialog->DoModal(GetID());

        g_stSettings.m_bMyProgramsViewAsIcons=!g_stSettings.m_bMyProgramsViewAsIcons;
				g_settings.Save();
				
        UpdateButtons();
      }
      else if (iControl==CONTROL_BTNFLATTEN) //flatten button
      {
        g_stSettings.m_bMyProgramsFlatten = !g_stSettings.m_bMyProgramsFlatten;
        Update(m_strDirectory);
      }
      else if (iControl==CONTROL_BTNSORTMETHOD) // sort by
      {
        g_stSettings.m_bMyProgramsSortMethod++;
        if (g_stSettings.m_bMyProgramsSortMethod >=3) g_stSettings.m_bMyProgramsSortMethod=0;
				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
        g_stSettings.m_bMyProgramsSortAscending=!g_stSettings.m_bMyProgramsSortAscending;
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
        OnClick(iItem);
        
      }
			else if (iControl >= 100 && iControl <= 110)
			{
				// bookmark button
				int iShare=iControl-100;
				if (iShare < (int)g_settings.m_vecMyProgramsBookmarks.size())
				{
					CShare share = g_settings.m_vecMyProgramsBookmarks[iControl-100];
					m_strDirectory=share.strPath;
					Update(m_strDirectory);
				}
			}
    }
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowPrograms::Render()
{
  CGUIWindow::Render();
}


void CGUIWindowPrograms::OnKey(const CKey& key)
{
  if (key.IsButton())
  {
    if ( key.GetButtonCode() == KEY_BUTTON_BACK  || key.GetButtonCode() == KEY_REMOTE_BACK)
    {
      m_gWindowManager.ActivateWindow(0); // back 2 home
      return;
    }
  }
  CGUIWindow::OnKey(key);
}


void CGUIWindowPrograms::LoadDirectory(const CStdString& strDirectory)
{
	WIN32_FIND_DATA wfd;
	HANDLE hFind;
  bool   bRecurseSubDirs(true);
	memset(&wfd,0,sizeof(wfd));
	CStdString strRootDir=strDirectory;
	if (!CUtil::HasSlashAtEnd(strRootDir) )
		strRootDir+="\\";

	if ( CUtil::IsDVD(strRootDir) )
  {
    CIoSupport helper;
    helper.Remount("D:","Cdrom0");
  }
  CStdString strSearchMask=strRootDir;
  strSearchMask+="*.*";

  FILETIME localTime;
  hFind = FindFirstFile(strSearchMask.c_str(),&wfd);
	if (hFind==NULL)
		return ;
	do
	{
		if (wfd.cFileName[0]!=0)
		{
      CStdString strFileName=wfd.cFileName;
      CStdString strFile=strRootDir;
      strFile+=wfd.cFileName;

			if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      {
        if (strFileName != "." && strFileName != "..")
        {
          if (g_stSettings.m_bMyProgramsFlatten)
          {
            if (bRecurseSubDirs) 
            {
              LoadDirectory(strFile);
            }
          }
          else
          {          
            CFileItem *pItem = new CFileItem(strFileName);
            pItem->m_strPath=strFile;
            pItem->m_bIsFolder=true;
						CStdString strThumb=pItem->m_strPath;
						strThumb+=".tbn";
						pItem->SetThumbnailImage(strThumb);
            FileTimeToLocalFileTime(&wfd.ftLastWriteTime,&localTime);
            FileTimeToSystemTime(&localTime, &pItem->m_stTime);
  		
            m_vecItems.push_back(pItem);      
          }
        }
      }
      else
      {
        if (CUtil::IsXBE(strFileName))
        {
          CStdString strDescription;
					if ( CUtil::GetXBEDescription(strFile,strDescription))
          {
            if ( !CUtil::IsDVD(strFile) )
            {
              CShortcut cut;
              cut.m_strPath=strFile;
              cut.Save(strDescription);
            }
          }
          CFileItem *pItem = new CFileItem(strDescription);
          pItem->m_strPath=strFile;
          pItem->m_bIsFolder=false;
          pItem->m_dwSize=wfd.nFileSizeLow;
					CStdString strThumb;
					if (!CUtil::GetXBEIcon(pItem->m_strPath.c_str(), strThumb))
          {
						pItem->SetThumbnailImage("defaultProgramIcon.png");
          }
					else
					{
						pItem->SetThumbnailImage(strThumb);
					}
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime,&localTime);
          FileTimeToSystemTime(&localTime, &pItem->m_stTime);
          m_vecItems.push_back(pItem);      
          bRecurseSubDirs=false;          
        }

        if ( CUtil::IsShortCut(strFileName)  )
        {
          CFileItem *pItem = new CFileItem(wfd.cFileName);
          pItem->m_strPath=strFile;
          pItem->m_bIsFolder=false;
          pItem->m_dwSize=wfd.nFileSizeLow;
					CStdString strThumb=pItem->m_strPath;
					CUtil::GetThumbnail(pItem->m_strPath,strThumb);
					pItem->SetThumbnailImage(strThumb);

          if (!CUtil::FileExists(strThumb))
          {
						pItem->SetThumbnailImage("defaultShortCutIcon.png");
          }

          FileTimeToLocalFileTime(&wfd.ftLastWriteTime,&localTime);
          FileTimeToSystemTime(&localTime, &pItem->m_stTime);
          m_vecItems.push_back(pItem);      
        }
      }
    }
  } while (FindNextFile(hFind, &wfd));

	FindClose( hFind );	  
}

void CGUIWindowPrograms::Clear()
{
  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    delete pItem;
  }
   m_vecItems.erase(m_vecItems.begin(),m_vecItems.end() );
}

void CGUIWindowPrograms::Update(const CStdString &strDirectory)
{
  Clear();
 
  if (!g_stSettings.m_bMyProgramsFlatten)
  {
    CStdString strParentPath;
    if (CUtil::GetParentPath(strDirectory,strParentPath))
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath=strParentPath;
      pItem->m_bIsShareOrDrive=false;
      pItem->m_bIsFolder=true;
      m_vecItems.push_back(pItem);
    }
  }
 
  LoadDirectory(strDirectory);
  OnSort();
  UpdateButtons();
}

void CGUIWindowPrograms::OnClick(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
  if (pItem->m_bIsFolder)
  {
    m_strDirectory=pItem->m_strPath;
		m_strDirectory+="\\";
    Update(m_strDirectory);
  }
  else
  {
    // launch xbe...
    char szPath[1024];
    char szDevicePath[1024];
    char szXbePath[1024];
	  char szParameters[1024];
    memset(szParameters,0,sizeof(szParameters));
    strcpy(szPath,pItem->m_strPath.c_str());

    if (CUtil::IsShortCut(pItem->m_strPath) )
    {
      CShortcut shortcut;
		  if ( shortcut.Create(pItem->m_strPath))
		  {
			  // if another shortcut is specified, load this up and use it
			  if ( CUtil::IsShortCut(shortcut.m_strPath.c_str() ) )
			  {
				  CHAR szNewPath[1024];
				  strcpy(szNewPath,szPath);
				  CHAR* szFile = strrchr(szNewPath,'\\');
				  strcpy(&szFile[1],shortcut.m_strPath.c_str());

				  CShortcut targetShortcut;
				  if (FAILED(targetShortcut.Create(szNewPath)))
					  return;

				  shortcut.m_strPath = targetShortcut.m_strPath;
			  }

			  strcpy( szPath, shortcut.m_strPath.c_str() );
			  strcpy( szDevicePath, shortcut.m_strPath.c_str() );

			  CHAR* szXbe = strrchr(szDevicePath,'\\');
			  *szXbe++=0x00;

			  wsprintf(szXbePath,"d:\\%s",szXbe);

			  CHAR szMode[16];
				strcpy( szMode, shortcut.m_strVideo.c_str() );
				strlwr( szMode );

				LPDWORD pdwVideo = (LPDWORD) 0x8005E760;
				BOOL	bRow = strstr(szMode,"pal")!=NULL;
				BOOL	bJap = strstr(szMode,"ntsc-j")!=NULL;
				BOOL	bUsa = strstr(szMode,"ntsc")!=NULL;

				if (bRow)
					*pdwVideo = 0x00800300;
				else if (bJap)
					*pdwVideo = 0x00400200;
				else if (bUsa)
					*pdwVideo = 0x00400100;
			  
			  strcat(szParameters, shortcut.m_strParameters.c_str());
		  }
	  }
    char* szBackslash = strrchr(szPath,'\\');
		*szBackslash=0x00;
		char* szXbe = &szBackslash[1];

		char* szColon = strrchr(szPath,':');
		*szColon=0x00;
		char* szDrive = szPath;
		char* szDirectory = &szColon[1];
    
    CIoSupport helper;
		helper.GetPartition( (LPCSTR) szDrive, szDevicePath);

		strcat(szDevicePath,szDirectory);
		wsprintf(szXbePath,"d:\\%s",szXbe);


		m_gWindowManager.DeInitialize();
		CSectionLoader::UnloadAll();

		g_application.Stop();
    if (strlen(szParameters))
      CUtil::LaunchXbe(szDevicePath,szXbePath,szParameters);
    else
      CUtil::LaunchXbe(szDevicePath,szXbePath,NULL);
  }
}

struct SSortProgramsByName
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
    CFileItem& rpStart=*pStart;
    CFileItem& rpEnd=*pEnd;
		if (rpStart.GetLabel()=="..") return true;
		if (rpEnd.GetLabel()=="..") return false;
		bool bGreater=true;
		if (g_stSettings.m_bMyProgramsSortAscending) bGreater=false;
    if ( rpStart.m_bIsFolder   == rpEnd.m_bIsFolder)
		{
			char szfilename1[1024];
			char szfilename2[1024];

			switch ( g_stSettings.m_bMyProgramsSortMethod ) 
			{
				case 0:	//	Sort by Filename
          strcpy(szfilename1, rpStart.m_strPath.c_str());
					strcpy(szfilename2, rpEnd.m_strPath.c_str());
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

			if (g_stSettings.m_bMyProgramsSortAscending)
				return (strcmp(szfilename1,szfilename2)<0);
			else
				return (strcmp(szfilename1,szfilename2)>=0);
		}
    if (!rpStart.m_bIsFolder) return false;
		return true;
	}
};

void CGUIWindowPrograms::OnSort()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
  g_graphicsContext.SendMessage(msg);         

  
  CGUIMessage msg2(GUI_MSG_LABEL_RESET,GetID(),CONTROL_THUMBS,0,0,NULL);
  g_graphicsContext.SendMessage(msg2);         

  
  
  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    if (g_stSettings.m_bMyProgramsSortMethod==0||g_stSettings.m_bMyProgramsSortMethod==2)
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

  
  sort(m_vecItems.begin(), m_vecItems.end(), SSortProgramsByName());

  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
		pItem->SetIconImage("defaultFolder.png");
		if (!CUtil::FileExists(pItem->GetThumbnailImage()) )
		{
			pItem->SetThumbnailImage("defaultFolderBig.png");
		}

    CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg);    
    CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
    g_graphicsContext.SendMessage(msg2);         
  }
}





void CGUIWindowPrograms::UpdateButtons()
{
    if (g_stSettings.m_bMyProgramsViewAsIcons) 
    {
      CGUIMessage msg(GUI_MSG_HIDDEN,GetID(), CONTROL_LIST);
      g_graphicsContext.SendMessage(msg);
      CGUIMessage msg2(GUI_MSG_VISIBLE,GetID(), CONTROL_THUMBS);
      g_graphicsContext.SendMessage(msg2);
    }
    else
    {
      CGUIMessage msg(GUI_MSG_HIDDEN,GetID(), CONTROL_THUMBS);
      g_graphicsContext.SendMessage(msg);
      CGUIMessage msg2(GUI_MSG_VISIBLE,GetID(), CONTROL_LIST);
      g_graphicsContext.SendMessage(msg2);
    }

    const WCHAR *szText;
    if (!g_stSettings.m_bMyProgramsViewAsIcons) 
    {
      szText=g_localizeStrings.Get(100).c_str();
    }
    else
    {
      szText=g_localizeStrings.Get(101).c_str();
    }
    CGUIMessage msg2(GUI_MSG_LABEL_SET,GetID(),CONTROL_BTNVIEWAS,0,0,(void*)szText);
    g_graphicsContext.SendMessage(msg2);         


    szText=g_localizeStrings.Get(g_stSettings.m_bMyProgramsSortMethod+103).c_str();
    
    CGUIMessage msg3(GUI_MSG_LABEL_SET,GetID(),CONTROL_BTNSORTMETHOD,0,0,(void*)szText);
    g_graphicsContext.SendMessage(msg3); 

    if ( g_stSettings.m_bMyProgramsSortAscending)
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
      if (pItem->GetLabel() =="..") iItems--;
    }
    WCHAR wszText[20];
    szText=g_localizeStrings.Get(127).c_str();
    swprintf(wszText,L"%i %s", iItems,szText);
    CGUIMessage msg4(GUI_MSG_LABEL_SET,GetID(),CONTROL_LABELFILES,0,0,(void*)wszText);
    g_graphicsContext.SendMessage(msg4); 
    

}