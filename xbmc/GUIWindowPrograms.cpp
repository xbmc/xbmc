
#include "stdafx.h"
#include "guiwindowprograms.h"
#include "localizestrings.h"
#include "GUIWindowManager.h"
#include "util.h"
#include "url.h"
#include "Xbox\IoSupport.h"
#include "Xbox\Undocumented.h"
#include "crc32.h"
#include "settings.h"
#include "lib/cximage/ximage.h"
#include "Shortcut.h"
#include "guidialog.h"
#include "sectionLoader.h"
#include "application.h"
#include "filesystem/HDDirectory.h"
#include "filesystem/directorycache.h"
#include "autoptrhandle.h"
#include "GUIThumbnailPanel.h"
#include "utils/log.h"
#include <algorithm>
#include "AutoSwitch.h"
#include "GUIPassword.h"

using namespace AUTOPTR;
using namespace DIRECTORY;

#define VIEW_AS_LIST           0
#define VIEW_AS_ICONS          1
#define VIEW_AS_LARGEICONS     2


#define CONTROL_BTNVIEWAS     2
#define CONTROL_BTNSCAN       3
#define CONTROL_BTNSORTMETHOD 4
#define CONTROL_BTNSORTASC    5
#define CONTROL_LIST          7
#define CONTROL_THUMBS        8
#define CONTROL_LABELFILES    9

CGUIWindowPrograms::CGUIWindowPrograms(void)
:CGUIWindow(0)
{
	m_strDirectory="?";
	m_iLastControl=-1;
	m_iViewAsIcons=-1;
}


CGUIWindowPrograms::~CGUIWindowPrograms(void)
{}

bool CGUIWindowPrograms::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
	{
	case GUI_MSG_WINDOW_DEINIT:
		{
			m_iLastControl=GetFocusedControl();
			m_iSelectedItem=GetSelectedItem();
			Clear();
			m_database.Close();
		}
		break;

	case GUI_MSG_WINDOW_INIT:
		{
			CGUIWindow::OnMessage(message);
			m_database.Open();
			m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
			if (m_strDirectory=="?")
			{
				m_strDirectory=g_stSettings.m_szDefaultPrograms;
				m_shareDirectory=g_stSettings.m_szDefaultPrograms;
				m_iDepth=1;
				m_strBookmarkName="default";
				if (g_guiSettings.GetBool("MyPrograms.NoShortcuts") && g_stSettings.m_szShortcutDirectory[0])	// let's remove shortcuts from vector
					g_settings.m_vecMyProgramsBookmarks.erase(g_settings.m_vecMyProgramsBookmarks.begin());
			}

			// make controls 100-110 invisible...
			for (int i=100; i < 110; i++)
			{		
				SET_CONTROL_HIDDEN(i);
			}

			if (m_iViewAsIcons==-1)
			{
				m_iViewAsIcons=g_stSettings.m_iMyProgramsViewAsIcons;
			}

			if (g_guiSettings.GetBool("MyPrograms.NoShortcuts"))				// let's hide Scan button
			{
				SET_CONTROL_HIDDEN(CONTROL_BTNSCAN);
			}
			else
			{
				SET_CONTROL_VISIBLE(CONTROL_BTNSCAN);
			}


			int iStartID=100;

			// create bookmark buttons

			for (int i=0; i < (int)g_settings.m_vecMyProgramsBookmarks.size(); ++i)
			{
				CShare& share = g_settings.m_vecMyProgramsBookmarks[i];

				SET_CONTROL_VISIBLE(i+iStartID);
				SET_CONTROL_LABEL(i+iStartID,share.strName);				
			}


			UpdateDir(m_strDirectory);

			if (m_iLastControl>-1)
				SET_CONTROL_FOCUS(m_iLastControl, 0);

			if (m_iSelectedItem>-1)
			{
				CONTROL_SELECT_ITEM(CONTROL_LIST,m_iSelectedItem);
				CONTROL_SELECT_ITEM(CONTROL_THUMBS,m_iSelectedItem);
			}
			ShowThumbPanel();
			return true;
		}
		break;

	case GUI_MSG_SETFOCUS:
		{}
		break;

	case GUI_MSG_CLICKED:
		{
			int iControl=message.GetSenderId();
			if (iControl==CONTROL_BTNVIEWAS)
			{
				bool bLargeIcons(false);
				m_iViewAsIcons++;
				if (m_iViewAsIcons > VIEW_AS_LARGEICONS) m_iViewAsIcons=VIEW_AS_LIST;
				g_stSettings.m_iMyProgramsViewAsIcons = m_iViewAsIcons;
				g_settings.Save();

				ShowThumbPanel();
				UpdateButtons();
			}
			else if (iControl==CONTROL_BTNSCAN) // button
			{
				int iTotalApps=0;
				CStdString strDir=m_strDirectory;
				m_strDirectory = g_stSettings.m_szShortcutDirectory;

				if (m_dlgProgress)
				{
					m_dlgProgress->SetHeading(211);
					m_dlgProgress->SetLine(0, "");
					m_dlgProgress->SetLine(1, "");
					m_dlgProgress->SetLine(2, "");
					m_dlgProgress->StartModal(GetID());
					m_dlgProgress->Progress();
				}

				CHDDirectory rootDir;

				// remove shortcuts...
				rootDir.SetMask(".cut");
				rootDir.GetDirectory(m_strDirectory,m_vecItems);

				for (int i=0; i < (int)m_vecItems.size(); ++i)
				{
					CFileItem* pItem=m_vecItems[i];
					if (CUtil::IsShortCut(pItem->m_strPath))
					{
						DeleteFile(pItem->m_strPath.c_str());
					}
				}

				// create new ones.

				VECFILEITEMS items;
				{
					m_strDirectory="C:";
					rootDir.SetMask(".xbe");
					rootDir.GetDirectory("C:\\",items);
					OnScan(items,iTotalApps);
					CFileItemList itemlist(items);
				}

				{
					m_strDirectory="E:";
					rootDir.SetMask(".xbe");
					rootDir.GetDirectory("E:\\",items);
					OnScan(items,iTotalApps);
					CFileItemList itemlist(items);
				}

				if (g_stSettings.m_bUseFDrive)
				{
					m_strDirectory="F:";
					rootDir.SetMask(".xbe");
					rootDir.GetDirectory("F:\\",items);
					OnScan(items,iTotalApps);
					CFileItemList itemlist(items);
				}
				if (g_stSettings.m_bUseGDrive)
				{
					m_strDirectory="G:";
					rootDir.SetMask(".xbe");
					rootDir.GetDirectory("G:\\",items);
					OnScan(items,iTotalApps);
					CFileItemList itemlist(items);
				}


				m_strDirectory=strDir;
				CUtil::ClearCache();
				Update(m_strDirectory);

				if (m_dlgProgress)
				{
					m_dlgProgress->Close();
				}
			}
			else if (iControl==CONTROL_BTNSORTMETHOD) // sort by
			{
				g_stSettings.m_iMyProgramsSortMethod++;
				if (g_stSettings.m_iMyProgramsSortMethod >=4)
					g_stSettings.m_iMyProgramsSortMethod=0;
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
				int iAction=message.GetParam1();
				if (ACTION_SELECT_ITEM==iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
				{
					CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
					g_graphicsContext.SendMessage(msg);
					int iItem=msg.GetParam1();
					OnClick(iItem);
				}
			}
			else if (iControl >= 100 && iControl <= 110)
			{
				// bookmark button
				int iShare=iControl-100;
				if (iShare < (int)g_settings.m_vecMyProgramsBookmarks.size())
				{
					CShare share = g_settings.m_vecMyProgramsBookmarks[iControl-100];
					m_shareDirectory=share.strPath;    // since m_strDirectory can change, we always want something that won't.
					m_strDirectory=share.strPath;
					m_strBookmarkName=share.strName;
					m_iDepth=share.m_iDepthSize;
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


void CGUIWindowPrograms::OnAction(const CAction &action)
{
	if (action.wID==ACTION_PARENT_DIR)
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



void CGUIWindowPrograms::LoadDirectory(const CStdString& strDirectory, int idepth)
{
	WIN32_FIND_DATA wfd;
	bool bOnlyDefaultXBE=g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
	bool bFlattenDir=g_guiSettings.GetBool("MyPrograms.Flatten");
	bool bUseDirectoryName=g_guiSettings.GetBool("MyPrograms.UseDirectoryName");

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
	CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(),&wfd));
	if (!hFind.isValid())
		return ;
	do {
		if (wfd.cFileName[0]!=0)
		{
			CStdString strFileName=wfd.cFileName;
			CStdString strFile=strRootDir;
			strFile+=wfd.cFileName;

			if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{
				if ( strFileName != "." && strFileName != ".." && !bFlattenDir)
				{
					CFileItem *pItem = new CFileItem(strFileName);
					pItem->m_strPath=strFile;
					pItem->m_bIsFolder=true;
					FileTimeToLocalFileTime(&wfd.ftLastWriteTime,&localTime);
					FileTimeToSystemTime(&localTime, &pItem->m_stTime);
					m_vecItems.push_back(pItem);
				}
				else
				{
					if (strFileName != "." && strFileName != ".." && bFlattenDir && idepth > 0 )
					{
						if (!m_database.EntryExists(strFile,m_strBookmarkName))
						{
							LoadDirectory(strFile, idepth-1);
						}
					}
				}


			}
			else
			{
				if (bOnlyDefaultXBE ? CUtil::IsDefaultXBE(strFileName) : CUtil::IsXBE(strFileName))
				{
					CStdString strDescription;

					if (!CUtil::GetXBEDescription(strFile, strDescription) || (bUseDirectoryName && CUtil::IsDefaultXBE(strFileName)) )
					{
						CUtil::GetDirectoryName(strFile, strDescription);
						CUtil::ShortenFileName(strDescription);
						CUtil::RemoveIllegalChars(strDescription);
					}

					if (!bFlattenDir || CUtil::IsDVD(strFile))
					{
						CFileItem *pItem = new CFileItem(strDescription);
						pItem->m_strPath=strFile;
						pItem->m_bIsFolder=false;
						pItem->m_dwSize=wfd.nFileSizeLow;

						FileTimeToLocalFileTime(&wfd.ftLastWriteTime,&localTime);
						FileTimeToSystemTime(&localTime, &pItem->m_stTime);
						m_vecItems.push_back(pItem);
					}
					else
					{
						m_database.AddProgram(strFile,strDescription,m_strBookmarkName);
					}
				}

				if (CUtil::IsShortCut(strFileName))
				{
					if (!bFlattenDir)
					{
						CFileItem *pItem = new CFileItem(wfd.cFileName);
						pItem->m_strPath=strFile;
						pItem->m_bIsFolder=false;
						pItem->m_dwSize=wfd.nFileSizeLow;

						FileTimeToLocalFileTime(&wfd.ftLastWriteTime,&localTime);
						FileTimeToSystemTime(&localTime, &pItem->m_stTime);
						m_vecItems.push_back(pItem);
					}

					else 
					{
						m_database.AddProgram(strFile,wfd.cFileName,m_strBookmarkName);
					}
				}
			}
		}
	} while(FindNextFile(hFind, &wfd));
}


void CGUIWindowPrograms::Clear()
{
	CFileItemList itemlist(m_vecItems); // will clean up everything
}

void CGUIWindowPrograms::Update(const CStdString &strDirectory)
{
	UpdateDir(strDirectory);
	if (g_guiSettings.GetBool("ProgramsLists.UseAutoSwitching"))
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


void CGUIWindowPrograms::UpdateDir(const CStdString &strDirectory)
{
	bool bFlattenDir=g_guiSettings.GetBool("MyPrograms.Flatten");
	bool bOnlyDefaultXBE=g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
	bool bParentPath(false);
	bool bPastBookMark(true);
	CStdString strParentPath;
	CStdString strDir = strDirectory;
	CStdString strShortCutsDir = g_stSettings.m_szShortcutDirectory;
	int idepth = m_iDepth;
	vector<CStdString> vecPaths;

	Clear();

	if (strDirectory=="")  // display only the root shares 
	{
		for (int i=0; i < (int)g_settings.m_vecMyProgramsBookmarks.size(); ++i)
		{
			CShare& share = g_settings.m_vecMyProgramsBookmarks[i];
			CStdString sharePath = share.strPath;
			vector<CStdString> vecShares;
			CFileItem *pItem = new CFileItem(share.strName);
			pItem->m_strPath=sharePath;
			pItem->m_bIsShareOrDrive=false;
			pItem->m_bIsFolder=true;
			pItem->m_idepth=share.m_iDepthSize;
			CUtil::Tokenize(sharePath, vecShares, ",");
			CStdString strThumb;
			for (int j=0; j < (int)vecShares.size(); j++)    // use the first folder image that we find
			{																								 // in the vector of shares
				if (!CUtil::IsXBE(vecShares[j]))
				{
					if (CUtil::GetFolderThumb(vecShares[j], strThumb)) 
					{
						pItem->SetThumbnailImage(strThumb);
						break;
					}
				}
			}
			m_vecItems.push_back(pItem);
		}

		m_strParentPath = "";
	}

	CUtil::Tokenize(strDir, vecPaths, ",");			

	if (!g_guiSettings.GetBool("MyPrograms.NoShortcuts"))
	{
		if (CUtil::HasSlashAtEnd(strShortCutsDir))
			strShortCutsDir.Delete(strShortCutsDir.size()-1);

		if (vecPaths.size()>0 && vecPaths[0]==strShortCutsDir)
			m_strBookmarkName="shortcuts";
	}

	for (int k=0; k < (int)vecPaths.size(); k++)
	{
		int start = vecPaths[k].find_first_not_of(" \t");
		int end = vecPaths[k].find_last_not_of(" \t") + 1;
		vecPaths[k]=vecPaths[k].substr(start, end - start);
		if (CUtil::HasSlashAtEnd(vecPaths[k]))
		{
			vecPaths[k].Delete(vecPaths[k].size()-1);
		}
	}

  if (!bFlattenDir)
	{
		vector<CStdString> vecOrigShare;
		CUtil::Tokenize(m_shareDirectory, vecOrigShare, ",");
		if (vecOrigShare.size() && vecPaths.size())
		{
			bParentPath=CUtil::GetParentPath(vecPaths[0],strParentPath);
			if (CUtil::HasSlashAtEnd(vecOrigShare[0]))
				vecOrigShare[0].Delete(vecOrigShare[0].size()-1);
			if (strParentPath<vecOrigShare[0])
			{
 				bPastBookMark=false;
				strParentPath="";
			}				
		}
	}
	else
	{
		strParentPath="";
	}

	if (strDirectory!="")
	{
			if (!g_guiSettings.GetBool("ProgramsLists.HideParentDirItems"))
			{
				CFileItem *pItem = new CFileItem("..");
				pItem->m_strPath=strParentPath;
				pItem->m_bIsShareOrDrive=false;
				pItem->m_bIsFolder=true;
				m_vecItems.push_back(pItem);
			}
			m_strParentPath = strParentPath;
	}

	m_iLastControl=GetFocusedControl();

	for (int j=0; j < (int)vecPaths.size(); j++)
	{
		if (CUtil::IsXBE(vecPaths[j]))					// we've found a single XBE in the path vector
		{
			CStdString strDescription;
			if ( (m_database.GetFile(vecPaths[j],m_vecItems) < 0) && CUtil::FileExists(vecPaths[j]))
			{
				if (!CUtil::GetXBEDescription(vecPaths[j], strDescription))
				{
					CUtil::GetDirectoryName(vecPaths[j], strDescription);
					CUtil::ShortenFileName(strDescription);
					CUtil::RemoveIllegalChars(strDescription);
				}

				if (CUtil::IsDVD(vecPaths[j]))
				{
					WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
					FILETIME localTime;
					CFileItem *pItem = new CFileItem(strDescription);
					pItem->m_strPath=vecPaths[j];
					pItem->m_bIsFolder=false;
					GetFileAttributesEx(pItem->m_strPath, GetFileExInfoStandard, &FileAttributeData);
					FileTimeToLocalFileTime(&FileAttributeData.ftLastWriteTime,&localTime);
					FileTimeToSystemTime(&localTime, &pItem->m_stTime);
					pItem->m_dwSize=FileAttributeData.nFileSizeLow;
					m_vecItems.push_back(pItem);
				}
				else
				{                                                                                                                                                                                                                                                                                                                                                                                    
					m_database.AddProgram(vecPaths[j], strDescription, m_strBookmarkName);
					m_database.GetFile(vecPaths[j],m_vecItems);
				}
			}
		}
		else
		{
			LoadDirectory(vecPaths[j], idepth);
			if (m_strBookmarkName=="shortcuts")
				bOnlyDefaultXBE=false;			// let's do this so that we don't only grab default.xbe from database when getting shortcuts
			if (bFlattenDir)
				m_database.GetProgramsByPath(vecPaths[j], m_vecItems, idepth, bOnlyDefaultXBE);
		}
	}

	//	if (bFlattenDir) 
	//		m_database.GetProgramsByBookmark(m_strBookmarkName, m_vecItems, bOnlyDefaultXBE); 

	CUtil::ClearCache();
	CUtil::SetThumbs(m_vecItems);
	if (g_guiSettings.GetBool("FileLists.HideExtensions"))
		CUtil::RemoveExtensions(m_vecItems);
	CUtil::FillInDefaultIcons(m_vecItems);

	OnSort();
	UpdateButtons();

	if (m_iLastControl==CONTROL_THUMBS || m_iLastControl==CONTROL_LIST)
	{
		if ( ViewByIcon() ) {	
			SET_CONTROL_FOCUS(CONTROL_THUMBS, 0);
		}
		else {
			SET_CONTROL_FOCUS(CONTROL_LIST, 0);
		}
	}
	ShowThumbPanel();
}

void CGUIWindowPrograms::OnClick(int iItem)
{
	if (iItem < 0 || iItem >(int)m_vecItems.size())
		return;
	CFileItem* pItem=m_vecItems[iItem];
	if (pItem->m_bIsFolder)
	{
        
/* LOCKSBUG: I get link errors when enabling this
    if ( !HaveBookmarkPermissions( pItem, "programs" ) )
      return;
*/

		if (m_strDirectory=="")
			m_shareDirectory=pItem->m_strPath;
		m_strDirectory=pItem->m_strPath;
		m_iDepth=pItem->m_idepth;
		Update(m_strDirectory);
	}
	else
	{

		// launch xbe...
		char szPath[1024];
		char szParameters[1024];
		if (g_guiSettings.GetBool("MyPrograms.Flatten"))
			m_database.IncTimesPlayed(pItem->m_strPath);
		m_database.Close();
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

				CHAR szMode[16];
				strcpy( szMode, shortcut.m_strVideo.c_str() );
				strlwr( szMode );

				LPDWORD pdwVideo = (LPDWORD) 0x8005E760;
				BOOL  bRow = strstr(szMode,"pal")!=NULL;
				BOOL  bJap = strstr(szMode,"ntsc-j")!=NULL;
				BOOL  bUsa = strstr(szMode,"ntsc")!=NULL;

				if (bRow)
					*pdwVideo = 0x00800300;
				else if (bJap)
					*pdwVideo = 0x00400200;
				else if (bUsa)
					*pdwVideo = 0x00400100;

				strcat(szParameters, shortcut.m_strParameters.c_str());
			}
		}
		if (strlen(szParameters))
			CUtil::RunXBE(szPath, szParameters);
		else
			CUtil::RunXBE(szPath);
	}
}

struct SSortProgramsByName
{
	bool operator()(CFileItem* pStart, CFileItem* pEnd)
	{
		CFileItem& rpStart=*pStart;
		CFileItem& rpEnd=*pEnd;
		if (rpStart.GetLabel()=="..")
			return true;
		if (rpEnd.GetLabel()=="..")
			return false;
		bool bGreater=true;
		if (g_stSettings.m_bMyProgramsSortAscending)
			bGreater=false;
		if ( rpStart.m_bIsFolder   == rpEnd.m_bIsFolder)
		{
			char szfilename1[1024];
			char szfilename2[1024];

			switch ( g_stSettings.m_iMyProgramsSortMethod )
			{
			case 0: //  Sort by Filename
				strcpy(szfilename1, rpStart.GetLabel().c_str());
				strcpy(szfilename2, rpEnd.GetLabel().c_str());
				break;
			case 1: // Sort by Date
				if ( rpStart.m_stTime.wYear > rpEnd.m_stTime.wYear )
					return bGreater;
				if ( rpStart.m_stTime.wYear < rpEnd.m_stTime.wYear )
					return !bGreater;

				if ( rpStart.m_stTime.wMonth > rpEnd.m_stTime.wMonth )
					return bGreater;
				if ( rpStart.m_stTime.wMonth < rpEnd.m_stTime.wMonth )
					return !bGreater;

				if ( rpStart.m_stTime.wDay > rpEnd.m_stTime.wDay )
					return bGreater;
				if ( rpStart.m_stTime.wDay < rpEnd.m_stTime.wDay )
					return !bGreater;

				if ( rpStart.m_stTime.wHour > rpEnd.m_stTime.wHour )
					return bGreater;
				if ( rpStart.m_stTime.wHour < rpEnd.m_stTime.wHour )
					return !bGreater;

				if ( rpStart.m_stTime.wMinute > rpEnd.m_stTime.wMinute )
					return bGreater;
				if ( rpStart.m_stTime.wMinute < rpEnd.m_stTime.wMinute )
					return !bGreater;

				if ( rpStart.m_stTime.wSecond > rpEnd.m_stTime.wSecond )
					return bGreater;
				if ( rpStart.m_stTime.wSecond < rpEnd.m_stTime.wSecond )
					return !bGreater;
				return true;
				break;

			case 2:
				if ( rpStart.m_dwSize > rpEnd.m_dwSize)
					return bGreater;
				if ( rpStart.m_dwSize < rpEnd.m_dwSize)
					return !bGreater;
				return true;
				break;

			case 3:
				if (rpStart.m_iprogramCount > rpEnd.m_iprogramCount)
					return bGreater;
				if (rpStart.m_iprogramCount < rpEnd.m_iprogramCount)
					return !bGreater;
				return true;
				break;

			default:  //  Sort by Filename by default
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
		if (!rpStart.m_bIsFolder)
			return false;
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
		if (g_stSettings.m_iMyProgramsSortMethod==0||g_stSettings.m_iMyProgramsSortMethod==2)
		{
			if (pItem->m_bIsFolder)
				pItem->SetLabel2("");
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
		if (g_stSettings.m_iMyProgramsSortMethod==3)
		{
			if (pItem->m_bIsFolder)
				pItem->SetLabel2("");
			else
			{
				CStdString strTimesPlayed;
				strTimesPlayed.Format("%i",pItem->m_iprogramCount);
				pItem->SetLabel2(strTimesPlayed);
			}
		}
	}


	sort(m_vecItems.begin(), m_vecItems.end(), SSortProgramsByName());

	for (int i=0; i < (int)m_vecItems.size(); i++)
	{
		CFileItem* pItem=m_vecItems[i];
		CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
		g_graphicsContext.SendMessage(msg);
		CGUIMessage msg2(GUI_MSG_LABEL_ADD,GetID(),CONTROL_THUMBS,0,0,(void*)pItem);
		g_graphicsContext.SendMessage(msg2);
	}
}

/// \brief Tests if the user is allowed to access the share folder
/// \param pItem The share folder item to access
/// \param strType The type of share being accessed, e.g. "music", "videos", etc. See CSettings::UpdateBookmark
/// \return If access is granted, returns \e true

void CGUIWindowPrograms::UpdateButtons()
{
	SET_CONTROL_HIDDEN(CONTROL_LIST);
	SET_CONTROL_HIDDEN(CONTROL_THUMBS);
	bool bViewIcon = false;
	int iString;

	switch (m_iViewAsIcons)
	{
	case VIEW_AS_LIST:
		iString=101; // view as list
		break;

	case VIEW_AS_ICONS:
		iString=100;  // view as icons
		bViewIcon=true;
		break;
	case VIEW_AS_LARGEICONS:
		iString=417; // view as list
		bViewIcon=true;
		break;
	}		

	if (bViewIcon) 
	{
		SET_CONTROL_VISIBLE(CONTROL_THUMBS);
	}
	else
	{
		SET_CONTROL_VISIBLE(CONTROL_LIST);
	}

	SET_CONTROL_LABEL(CONTROL_BTNVIEWAS,iString);

	if (g_stSettings.m_iMyProgramsSortMethod==3)
	{
		SET_CONTROL_LABEL(CONTROL_BTNSORTMETHOD, 507);  //Times Played
	}
	else
	{
		SET_CONTROL_LABEL(CONTROL_BTNSORTMETHOD,g_stSettings.m_iMyProgramsSortMethod+103);
	}

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
		if (pItem->GetLabel() =="..")
			iItems--;
	}
	WCHAR wszText[20];
	const WCHAR* szText=g_localizeStrings.Get(127).c_str();
	swprintf(wszText,L"%i %s", iItems,szText);

	SET_CONTROL_LABEL(CONTROL_LABELFILES,wszText);


}

void CGUIWindowPrograms::OnScan(VECFILEITEMS& items, int& iTotalAppsFound)
{
	// remove username + password from m_strDirectory for display in Dialog
	CURL url(m_strDirectory);
	CStdString strStrippedPath;
	url.GetURLWithoutUserDetails(strStrippedPath);

	CStdString strText=g_localizeStrings.Get(212);
	CStdString strTotal;
	strTotal.Format("%i %s", iTotalAppsFound, strText.c_str());
	if (m_dlgProgress)
	{
		m_dlgProgress->SetLine(0,strTotal);
		m_dlgProgress->SetLine(1,"");
		m_dlgProgress->SetLine(2,strStrippedPath);
		m_dlgProgress->Progress();
	}
	//bool   bOnlyDefaultXBE=g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
	bool bScanSubDirs=true;
	bool bFound=false;
	DeleteThumbs(items);
	//CUtil::SetThumbs(items);
	CUtil::CreateShortcuts(items);
	if ((int)m_strDirectory.size() != 2) // true for C:, E:, F:, G:
	{
		// first check all files
		for (int i=0; i < (int)items.size(); ++i)
		{
			CFileItem *pItem= items[i];
			if (! pItem->m_bIsFolder)
			{
				if (CUtil::IsXBE(pItem->m_strPath) )
				{
					bScanSubDirs=false;
					break;
				}
			}
		}
	}

	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem *pItem= items[i];
		if (pItem->m_bIsFolder)
		{
			if (bScanSubDirs && !bFound && pItem->GetLabel() != "..")
			{
				// load subfolder
				CStdString strDir=m_strDirectory;
				if (pItem->m_strPath != "E:\\UDATA" && pItem->m_strPath !="E:\\TDATA")
				{
					m_strDirectory=pItem->m_strPath;
					VECFILEITEMS subDirItems;
					CFileItemList itemlist(subDirItems); // will clean up everything
					CHDDirectory rootDir;
					rootDir.SetMask(".xbe");
					rootDir.GetDirectory(pItem->m_strPath,subDirItems);
					OnScan(subDirItems,iTotalAppsFound);
					m_strDirectory=strDir;
				}
			}
		}
		else
		{
			if ( CUtil::IsXBE(pItem->m_strPath) )
			{
				bFound=true;
				iTotalAppsFound++;

				CStdString strText=g_localizeStrings.Get(212);
				strTotal.Format("%i %s", iTotalAppsFound, strText.c_str());
				CStdString strDescription;
				CUtil::GetXBEDescription(pItem->m_strPath,strDescription);
				if (strDescription=="")
					strDescription=CUtil::GetFileName(pItem->m_strPath);
				if (m_dlgProgress)
				{
					m_dlgProgress->SetLine(0, strTotal);
					m_dlgProgress->SetLine(1, "");
					m_dlgProgress->SetLine(2,strStrippedPath);
					m_dlgProgress->Progress();
				}
				// CStdString strIcon;
				// CUtil::GetXBEIcon(pItem->m_strPath, strIcon);
				// ::DeleteFile(strIcon.c_str());

			}
		}
	}
	g_directoryCache.Clear();
}

void CGUIWindowPrograms::DeleteThumbs(VECFILEITEMS& items)
{
	CUtil::ClearCache();
	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem *pItem= items[i];
		if (! pItem->m_bIsFolder)
		{
			if (CUtil::IsXBE(pItem->m_strPath) )
			{
				CStdString strThumb;
				CUtil::GetXBEIcon(pItem->m_strPath,strThumb);
				CStdString strName=pItem->m_strPath;
				CUtil::ReplaceExtension(pItem->m_strPath,".tbn", strName);
				if (strName!=strThumb)
				{
					::DeleteFile(strThumb.c_str());
				}
			}
		}
	}
}

int CGUIWindowPrograms::GetSelectedItem()
{
	int iControl;
	if ( ViewByIcon())
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


bool CGUIWindowPrograms::ViewByIcon()
{
	if (m_iViewAsIcons != VIEW_AS_LIST) return true;
	return false;
}

bool CGUIWindowPrograms::ViewByLargeIcon()
{
	if (m_iViewAsIcons== VIEW_AS_LARGEICONS) return true;
	return false;
}

void CGUIWindowPrograms::ShowThumbPanel()
{
	int iItem=GetSelectedItem(); 
	if ( ViewByLargeIcon() )
	{
		CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
		pControl->ShowBigIcons(true);
	}
	else
	{
		CGUIThumbnailPanel* pControl=(CGUIThumbnailPanel*)GetControl(CONTROL_THUMBS);
		pControl->ShowBigIcons(false);
	}
	if (iItem>-1)
	{
		CONTROL_SELECT_ITEM(CONTROL_LIST,iItem);
		CONTROL_SELECT_ITEM(CONTROL_THUMBS,iItem);
	}
}

/// \brief Call to go to parent folder
void CGUIWindowPrograms::GoParentFolder()
{
	CStdString strPath=m_strParentPath;
	Update(strPath);
}
