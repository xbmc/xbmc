//

#include "stdafx.h"
#include "guiwindowVideoFiles.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "localizestrings.h"
#include "texturemanager.h"
#include "util.h"
#include "picture.h"
#include "url.h"
#include "utils/imdb.h"
#include "utils/http.h"
#include "GUIDialogOK.h"
#include "GUIDialogprogress.h"
#include "GUIDialogSelect.h" 
#include "GUIWindowVideoInfo.h" 
#include "PlayListFactory.h"
#include "application.h" 
#include <algorithm>
#include "DetectDVDType.h"
#include "nfofile.h"
#include "utils/fstrcmp.h"
#include "utils/log.h"
#include "filesystem/file.h"
#include "playlistplayer.h"
#include "xbox/iosupport.h"
#include "GUIThumbnailPanel.h"
#include "GUIListControl.h"
#include "filesystem/directorycache.h"
#include "GUIDialogOK.h"

#define VIEW_AS_LIST           0
#define VIEW_AS_ICONS          1
#define VIEW_AS_LARGEICONS     2

#define CONTROL_BTNVIEWASICONS		2
#define CONTROL_BTNSORTBY					3
#define CONTROL_BTNSORTASC				4
#define CONTROL_BTNTYPE           5
#define CONTROL_PLAY_DVD          6
#define CONTROL_STACK             7
#define CONTROL_BTNSCAN           8
#define CONTROL_IMDB              9
#define CONTROL_LIST							10
#define CONTROL_THUMBS						11
#define CONTROL_LABELFILES         12

struct SSortVideoListByName
{
	bool operator()(CStdString& strItem1, CStdString& strItem2)
	{
    return strcmp(strItem1.c_str(),strItem2.c_str())<0;
  }
};

struct SSortVideoByName
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


CGUIWindowVideoFiles::CGUIWindowVideoFiles()
:CGUIWindowVideoBase()
{
	m_strDirectory="?";
  m_iItemSelected=-1;
	m_iLastControl=-1;
	m_bDisplayEmptyDatabaseMessage = false;
}

CGUIWindowVideoFiles::~CGUIWindowVideoFiles()
{
}


void CGUIWindowVideoFiles::OnAction(const CAction &action)
{
	if (action.wID==ACTION_SHOW_PLAYLIST)
	{
    OutputDebugString("activate guiwindowvideoplaylist!\n");
		m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
		return;
	}

	CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoFiles::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_DVDDRIVE_EJECTED_CD:
		case GUI_MSG_DVDDRIVE_CHANGED_CD:
		case GUI_MSG_WINDOW_DEINIT:
			CGUIWindowVideoBase::OnMessage(message);
		break;

    case GUI_MSG_WINDOW_INIT:
		{
			//	This window is started by the home window.
			//	Now we decide which my music window has to be shown and
			//	switch to the my music window the user last activated.
			if (g_stSettings.m_iVideoStartWindow >0 && g_stSettings.m_iVideoStartWindow !=GetID() )
			{
				m_gWindowManager.ActivateWindow(g_stSettings.m_iVideoStartWindow);
				return false;
			}

			if (m_strDirectory=="?") 
			{
				m_rootDir.SetMask(g_stSettings.m_szMyVideoExtensions);
				m_rootDir.SetShares(g_settings.m_vecMyVideoShares);
				m_strDirectory=g_stSettings.m_szDefaultVideos;
				SetHistoryForPath(m_strDirectory);
			}
			return CGUIWindowVideoBase::OnMessage(message);
		}
		break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
      if (iControl==CONTROL_BTNSORTBY) // sort by
      {
				if (m_strDirectory.IsEmpty())
				{
					if (g_stSettings.m_iMyVideoRootSortMethod==0)
						g_stSettings.m_iMyVideoRootSortMethod=3;
					else
						g_stSettings.m_iMyVideoRootSortMethod=0;
				}
				else
				{
					g_stSettings.m_iMyVideoSortMethod++;
					if (g_stSettings.m_iMyVideoSortMethod >=3) g_stSettings.m_iMyVideoSortMethod=0;
				}
				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl==CONTROL_BTNSORTASC) // sort asc
      {
				if (m_strDirectory.IsEmpty())
					g_stSettings.m_bMyVideoRootSortAscending=!g_stSettings.m_bMyVideoRootSortAscending;
				else
					g_stSettings.m_bMyVideoSortAscending=!g_stSettings.m_bMyVideoSortAscending;

				g_settings.Save();
        UpdateButtons();
        OnSort();
      }
 			else if (iControl==CONTROL_BTNSCAN)
			{
				OnScan(m_vecItems);
			}

      else if (iControl==CONTROL_STACK)
      {
        // toggle between the following states:
        //   0 : no stacking
        //   1 : simple stacking
        //   2 : fuzzy stacking
				g_stSettings.m_iMyVideoVideoStack++;
				if (g_stSettings.m_iMyVideoVideoStack > STACK_FUZZY) g_stSettings.m_iMyVideoVideoStack = STACK_NONE;
        if (g_stSettings.m_iMyVideoVideoStack != STACK_NONE)
          g_stSettings.m_bMyVideoCleanTitles = true;
				else
					g_stSettings.m_bMyVideoCleanTitles = false;
        g_settings.Save();
        UpdateButtons();
        Update( m_strDirectory );
      }
      else
 				return CGUIWindowVideoBase::OnMessage(message);
    }
	}
  return CGUIWindow::OnMessage(message);
}


void CGUIWindowVideoFiles::UpdateButtons()
{
	CGUIWindowVideoBase::UpdateButtons();
	SET_CONTROL_LABEL(GetID(), CONTROL_STACK,g_stSettings.m_iMyVideoVideoStack + 14000);
}

void CGUIWindowVideoFiles::FormatItemLabels()
{
  for (int i=0; i < (int)m_vecItems.size(); i++)
  {
    CFileItem* pItem=m_vecItems[i];
    if (g_stSettings.m_iMyVideoSortMethod==0||g_stSettings.m_iMyVideoSortMethod==2)
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
}

void CGUIWindowVideoFiles::SortItems()
{
  SSortVideoByName sortmethod;
	if (m_strDirectory.IsEmpty())
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyVideoRootSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyVideoRootSortAscending;
	}
	else
	{
		sortmethod.m_iSortMethod=g_stSettings.m_iMyVideoSortMethod;
		sortmethod.m_bSortAscending=g_stSettings.m_bMyVideoSortAscending;
	}
  sort(m_vecItems.begin(), m_vecItems.end(), sortmethod);
}

void CGUIWindowVideoFiles::Update(const CStdString &strDirectory)
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
				m_vecItems.push_back(pItem);
			}
			m_strParentPath = strParentPath;
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
			pItem->m_bIsShareOrDrive=false;
			pItem->m_bIsFolder=true;
			m_vecItems.push_back(pItem);
		}
		m_strParentPath.Empty();
  }

  m_strDirectory=strDirectory;

  if (g_stSettings.m_iMyVideoVideoStack != STACK_NONE)
  {
    VECFILEITEMS items;
	  m_rootDir.GetDirectory(strDirectory,items);
    bool bDVDFolder(false);
		//Figure out first if we are in a folder that contains a dvd
    for (int i=0; i < (int)items.size(); ++i) //Do it this way to avoid an extra roundtrip to files
    {
			CFileItem* pItem1= items[i];
			if (CStdString(CUtil::GetFileName(pItem1->m_strPath)).Equals("VIDEO_TS.IFO"))
			{
				bDVDFolder = true;
				m_vecItems.push_back(pItem1);
				items.erase(items.begin() + i); //Make sure this is not included in the comeing search as it would have been deleted.
				break;
			}
		}
	  
		for (int i=0; i < (int)items.size(); ++i)
		{
			bool bAdd(true);
			CFileItem* pItem1= items[i];
			if (CUtil::IsNFO(pItem1->m_strPath))
			{
				bAdd=false;
			}
			else if(bDVDFolder && CUtil::IsDVDFile(pItem1->m_strPath, true, true)) //Hide all dvdfiles
			{
				bAdd=false;
			}
			else
			{
				//don't stack folders and playlists
				if ((!pItem1->m_bIsFolder) && !CUtil::IsPlayList(pItem1->m_strPath))
				{
					CStdString fileName1 = CUtil::GetFileName(pItem1->m_strPath);

					CStdString fileTitle;
					CStdString volumePrefix;
					int volumeNumber;

					bool searchForStackedFiles = false;
					if (g_stSettings.m_iMyVideoVideoStack == STACK_FUZZY)
					{
						searchForStackedFiles = true;
					}
					else
					{
						if (CUtil::GetVolumeFromFileName(fileName1, fileTitle, volumePrefix, volumeNumber))
						{
							if (volumeNumber > 1)
							{
								searchForStackedFiles = true;
							}
						}
					}
	          
					if (searchForStackedFiles)
					{
						for (int x=0; x < (int)items.size(); ++x)
						{
							if (i != x)
							{
								CFileItem* pItem2= items[x];  
								if ((!pItem2->m_bIsFolder) && !CUtil::IsPlayList(pItem2->m_strPath))
								{
									CStdString fileName2 = CUtil::GetFileName(pItem2->m_strPath);

									if (g_stSettings.m_iMyVideoVideoStack == STACK_FUZZY)
									{
										// use "fuzzy" stacking

										double fPercentage=fstrcmp(fileName1, fileName2, COMPARE_PERCENTAGE_MIN);
										if (fPercentage>=COMPARE_PERCENTAGE)
										{
											int iGreater = strcmp(fileName1, fileName2);
											if (iGreater >0)
											{
												bAdd=false;
												break;
											}
										}
									}
									else
									{
										// use traditional "simple" stacking (like XBMP)
										// file name must end in -CD[n], where only the first 
										// one (-CD1) will be added to the display list

										CStdString fileTitle2;
										CStdString volumePrefix2;
										int volumeNumber2;
										if (CUtil::GetVolumeFromFileName(fileName2, fileTitle2, volumePrefix2, volumeNumber2))
										{
											// TODO: check volumePrefix - they should be in the 
											// same category, but not necessarily equal!

											if ((volumeNumber2 == 1) 
													&& volumePrefix.Equals(volumePrefix2)
													&& fileTitle.Equals(fileTitle2))
											{
												bAdd = false;
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}
			if (bAdd)
			{
				m_vecItems.push_back(pItem1);
			}
			else
			{
				delete pItem1;
			}
		}
  }
  else
  {
    VECFILEITEMS items;
	  m_rootDir.GetDirectory(strDirectory,m_vecItems);
  }

	m_iLastControl=GetFocusedControl();

	CUtil::SetThumbs(m_vecItems);
	if ((g_stSettings.m_bHideExtensions) || (g_stSettings.m_bMyVideoCleanTitles))
		CUtil::RemoveExtensions(m_vecItems);
  if (g_stSettings.m_bMyVideoCleanTitles)
    CUtil::CleanFileNames(m_vecItems);

	SetIMDBThumbs(m_vecItems);
	CUtil::FillInDefaultIcons(m_vecItems);
  OnSort();
  UpdateButtons();

	strSelectedItem=m_history.Get(m_strDirectory);	

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

void CGUIWindowVideoFiles::OnClick(int iItem)
{
  CFileItem* pItem=m_vecItems[iItem];
  CStdString strPath=pItem->m_strPath;
  CStdString strExtension;
	CUtil::GetExtension(pItem->m_strPath,strExtension);
	if ( CUtil::cmpnocase(strExtension.c_str(),".nfo") ==0) 
	{
		OnInfo(iItem);
		return;
	}

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
    // Set selected item
	  m_iItemSelected=GetSelectedItem();
    CStdString strFileName=pItem->m_strPath;
    if (CUtil::IsPlayList(strFileName))
    {
      LoadPlayList(strFileName);
      return;
    }
    if (!CheckMovie(strFileName)) return;
    if (g_stSettings.m_iMyVideoVideoStack != STACK_NONE)
    {
      CStdString fileName = CUtil::GetFileName(strFileName);

      CStdString fileTitle;
      CStdString volumePrefix;
      int volumeNumber;
      bool fileStackable = true;
      if (g_stSettings.m_iMyVideoVideoStack == STACK_SIMPLE)
      {
        if (!CUtil::GetVolumeFromFileName(fileName, fileTitle, volumePrefix, volumeNumber))
          fileStackable = false;
      }

      vector<CStdString> movies;
      {
        if (fileStackable)
        {
        VECFILEITEMS items;
	      m_rootDir.GetDirectory(m_strDirectory,items);
        for (int i=0; i < (int)items.size(); ++i)
        {
          CFileItem *pItemTmp=items[i];
            CStdString fileNameTemp = pItemTmp->m_strPath;
            if (!CUtil::IsNFO(fileNameTemp) && !CUtil::IsPlayList(fileNameTemp))
            {
              if (CUtil::IsVideo(fileNameTemp))
              {
                fileNameTemp = CUtil::GetFileName(fileNameTemp);
                bool stackFile = false;

                if (fileName.Equals(fileNameTemp))
                {
                  stackFile = true;
                }
                else if (g_stSettings.m_iMyVideoVideoStack == STACK_FUZZY)
								{
                  // fuzzy stacking
                  double fPercentage=fstrcmp(fileNameTemp, fileName, COMPARE_PERCENTAGE_MIN);
									if (fPercentage >=COMPARE_PERCENTAGE)
									{
                    stackFile = true;
                  }
                }
                else
                {
                  // simple stacking
                  CStdString fileTitle2;
                  CStdString volumePrefix2;
                  int volumeNumber2;
                  if (CUtil::GetVolumeFromFileName(fileNameTemp, fileTitle2, volumePrefix2, volumeNumber2))
                  {
                    if (fileTitle.Equals(fileTitle2) && volumePrefix.Equals(volumePrefix2))
                    {
                      stackFile = true;
                    }
                  }
                }

                if (stackFile)
              {
                movies.push_back(pItemTmp->m_strPath);
              }
            }
          }
        }
        CFileItemList itemlist(items); // will clean up everything
      }
        else
        {
          // file is not stackable - simply add it as the only item in the list
          movies.push_back(strFileName);
        }
      }
		  if (movies.size() <=0) return;
      int iSelectedFile=1;
      if (movies.size() >1)
      {
        sort(movies.begin(), movies.end(), SSortVideoListByName());
        for (int i=0; i < (int)movies.size(); ++i)
        {
          AddFileToDatabase(movies[i]);
        }

        CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
		    if (dlg)
        {
          dlg->SetNumberOfFiles(movies.size());
		      dlg->DoModal(GetID());
		      iSelectedFile = dlg->GetSelectedFile();
          if (iSelectedFile < 1) return;
        }
				g_playlistPlayer.Reset();
				g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO_TEMP);
				CPlayList& playlist=g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO_TEMP);
				playlist.Clear();
				for (int i=iSelectedFile-1; i < (int)movies.size(); ++i)
				{
					strFileName=movies[i];
					CPlayList::CPlayListItem item;
					item.SetFileName(strFileName);
					playlist.Add(item);
				}

				// play movie...
				g_playlistPlayer.PlayNext();
				return;
      }
    }

		// play movie...
    AddFileToDatabase(strFileName);
		g_application.PlayFile(*pItem);
	}
}

void CGUIWindowVideoFiles::OnInfo(int iItem)
{
 bool                 bFolder(false);
 CStdString           strFolder="";
 int iSelectedItem=GetSelectedItem();
 CFileItem*						pItem=m_vecItems[iItem];
 CStdString           strFile=pItem->m_strPath;
 CStdString           strMovie=pItem->GetLabel();
 if (pItem->m_bIsFolder)
 {
    // IMDB is done on a folder, find first file in folder
   strFolder=pItem->m_strPath;
    bFolder=true;
    VECFILEITEMS vecitems;
    m_rootDir.GetDirectory(pItem->m_strPath, vecitems);
    bool bFoundFile(false);
    for (int i=0; i < (int)vecitems.size(); ++i)
    {
      CFileItem *pItem=vecitems[i];
      if (!pItem->m_bIsFolder)
      {
        if (CUtil::IsVideo(pItem->m_strPath) && !CUtil::IsNFO(pItem->m_strPath) && !CUtil::IsPlayList(pItem->m_strPath) )
        {
          strFile=pItem->m_strPath;
          bFoundFile=true;
          break;
        }
      }
    }
    CFileItemList itemlist(vecitems); // will clean up everything
    if (!bFoundFile) 
    {
      // no video file in this folder?
      // then just lookup IMDB info and show it
      ShowIMDB(strMovie,strFolder, strFolder, false);
      CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iSelectedItem);
      CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iSelectedItem);
      return;
    }
 }

 AddFileToDatabase(strFile);

 
 ShowIMDB(strMovie,strFile, strFolder, bFolder);
CONTROL_SELECT_ITEM(GetID(), CONTROL_LIST,iSelectedItem);
CONTROL_SELECT_ITEM(GetID(), CONTROL_THUMBS,iSelectedItem);
}

void CGUIWindowVideoFiles::Render()
{
	CGUIWindow::Render();
	if (m_bDisplayEmptyDatabaseMessage)
	{
		CGUIListControl *pControl = (CGUIListControl *)GetControl(CONTROL_LIST);
		int iX = pControl->GetXPosition()+pControl->GetWidth()/2;
		int iY = pControl->GetYPosition()+pControl->GetHeight()/2;
		CGUIFont *pFont = g_fontManager.GetFont(pControl->GetFontName());
		if (pFont)
		{
			float fWidth, fHeight;
			CStdStringW wszText = g_localizeStrings.Get(745);	// "No scanned information for this view"
			CStdStringW wszText2 = g_localizeStrings.Get(746); // "Switch back to Files view"
			pFont->GetTextExtent(wszText, &fWidth, &fHeight);
			pFont->DrawText((float)iX, (float)iY-fHeight, 0xffffffff, wszText.c_str(), XBFONT_CENTER_X|XBFONT_CENTER_Y);
			pFont->DrawText((float)iX, (float)iY+fHeight, 0xffffffff, wszText2.c_str(), XBFONT_CENTER_X|XBFONT_CENTER_Y);
		}
	}
}

void CGUIWindowVideoFiles::AddFileToDatabase(const CStdString& strFile)
{
  CStdString strCDLabel="";
  bool bHassubtitles=false;
  if (!CUtil::IsVideo(strFile)) return;
  if ( CUtil::IsNFO(strFile)) return;
  if ( CUtil::IsPlayList(strFile)) return;

    // get disc label for dvd's / iso9660
  if (CUtil::IsDVD(strFile) || CUtil::IsISO9660(strFile)) 
  {
    CCdInfo* pinfo=CDetectDVDMedia::GetCdInfo();
    if (pinfo)
    {
      strCDLabel=pinfo->GetDiscLabel();
    }
  }

  char * sub_exts[] = {  ".utf", ".utf8", ".utf-8", ".sub", ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".ass", ".idx",".ifo", NULL};
  // check if movie has subtitles
  int ipos=0;
  while (sub_exts[ipos])
  {
    CStdString strSubTitleFile = strFile;     
    CUtil::ReplaceExtension(strFile,sub_exts[ipos],strSubTitleFile);
    CFile file;
    if (file.Open(strSubTitleFile,false) )
    {
        bHassubtitles=true;
        break;
    }
    ipos++;
  }  
  m_database.AddMovie(strFile, strCDLabel, bHassubtitles);
}

void CGUIWindowVideoFiles::OnRetrieveVideoInfo(VECFILEITEMS& items)
{

  // for every file found
  for (int i=0; i < (int)items.size(); ++i)
	{
    g_application.ResetScreenSaver();
    CFileItem* pItem=items[i];
    if (!pItem->m_bIsFolder)
    {
      if (CUtil::IsVideo(pItem->m_strPath) && !CUtil::IsNFO(pItem->m_strPath) && !CUtil::IsPlayList(pItem->m_strPath) )
      {
        CStdString strItem;
        strItem.Format("%i/%i", i+1, items.size());
        if (m_dlgProgress)
        {
          m_dlgProgress->SetLine(0,strItem);
          m_dlgProgress->SetLine(1,CUtil::GetFileName(pItem->m_strPath) );
          m_dlgProgress->Progress();
          if (m_dlgProgress->IsCanceled()) return;
        }
        AddFileToDatabase(pItem->m_strPath);
        if (!m_database.HasMovieInfo(pItem->m_strPath))
        {
          // do IMDB lookup...
          CStdString strMovieName=CUtil::GetFileName(pItem->m_strPath);
					CUtil::RemoveExtension(strMovieName);

					if (m_dlgProgress)
          {
		        m_dlgProgress->SetHeading(197);
		        m_dlgProgress->SetLine(0,strMovieName);
		        m_dlgProgress->SetLine(1,"");
		        m_dlgProgress->SetLine(2,"");
		        m_dlgProgress->Progress();
          }


          CIMDB								IMDB;
          IMDB_MOVIELIST	movielist;
		      if (IMDB.FindMovie(strMovieName,	movielist) ) 
		      {
            int iMoviesFound=movielist.size();
			      if (iMoviesFound > 0)
			      {
				      CIMDBMovie movieDetails;
              movieDetails.m_strSearchString=pItem->m_strPath;
				      CIMDBUrl& url=movielist[0];

				      // show dialog that we're downloading the movie info
              if (m_dlgProgress)
              {
				        m_dlgProgress->SetHeading(198);
				        m_dlgProgress->SetLine(0,strMovieName);
				        m_dlgProgress->SetLine(1,url.m_strTitle);
				        m_dlgProgress->SetLine(2,"");
                m_dlgProgress->Progress();
              }

              CUtil::ClearCache();
              if ( IMDB.GetDetails(url, movieDetails) )
				      {
                // get & save thumbnail
                m_database.SetMovieInfo(pItem->m_strPath,movieDetails);
                CStdString strThumb="";
                CStdString strImage=movieDetails.m_strPictureURL;
                if (strImage.size() >0 && movieDetails.m_strSearchString.size() > 0)
                {
                  
                  
                  CUtil::GetVideoThumbnail(movieDetails.m_strIMDBNumber,strThumb);
	                ::DeleteFile(strThumb.c_str());

		              CHTTP http;
		              CStdString strExtension;
		              CUtil::GetExtension(strImage,strExtension);
		              CStdString strTemp="Z:\\temp";
		              strTemp+=strExtension;
                  ::DeleteFile(strTemp.c_str());
                  if (m_dlgProgress)
                  {
				            m_dlgProgress->SetLine(2,415);
                    m_dlgProgress->Progress();
                  }
		              http.Download(strImage, strTemp);

                  try
                  {
		                CPicture picture;
		                picture.Convert(strTemp,strThumb);
                  }
                  catch(...)
                  {
                    ::DeleteFile(strThumb.c_str());
                  }
                  ::DeleteFile(strTemp.c_str());
                }
              }
            }
          }
        }
      }
    }
  }
}

bool CGUIWindowVideoFiles::OnScan(VECFILEITEMS& items)
{
	// remove username + password from m_strDirectory for display in Dialog
	CURL url(m_strDirectory);
	CStdString strStrippedPath;
	url.GetURLWithoutUserDetails(strStrippedPath);

	if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(189);
	  m_dlgProgress->SetLine(0,"");
	  m_dlgProgress->SetLine(1,"");
	  m_dlgProgress->SetLine(2,strStrippedPath );
	  m_dlgProgress->StartModal(GetID());
  }

  OnRetrieveVideoInfo(items);

	if (m_dlgProgress)
  {
    m_dlgProgress->SetLine(2,strStrippedPath );
	  if (m_dlgProgress->IsCanceled()) return false;
  }
	
	bool bCancel=false;
	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem *pItem= items[i];
    if (m_dlgProgress)
    {
		  if (m_dlgProgress->IsCanceled()) 
		  {
			  bCancel=true;
			  break;
		  }
    }
		if ( pItem->m_bIsFolder)
		{
			if (pItem->GetLabel() != "..")
			{
				// load subfolder
				CStdString strDir=m_strDirectory;
				m_strDirectory=pItem->m_strPath;
				VECFILEITEMS subDirItems;
				CFileItemList itemlist(subDirItems);
				m_rootDir.GetDirectory(pItem->m_strPath,subDirItems);
				if (m_dlgProgress)
					m_dlgProgress->Close();
				if (!OnScan(subDirItems))
				{
					bCancel=true;
				}
				
				m_strDirectory=strDir;
				if (bCancel) break;
			}
		}
	}
	
	if (m_dlgProgress) m_dlgProgress->Close();
	return !bCancel;
}

void CGUIWindowVideoFiles::SetIMDBThumbs(VECFILEITEMS& items)
{
  VECMOVIES movies;
  m_database.GetMoviesByPath(m_strDirectory,movies);
  for (int x=0; x < (int)items.size(); ++x)
  {
    CFileItem* pItem=items[x];
    if (!pItem->m_bIsFolder && pItem->GetThumbnailImage() == "")
    {
      for (int i=0; i < (int)movies.size(); ++i)
      {
        CIMDBMovie& info=movies[i];
        CStdString strFile = CUtil::GetFileName(pItem->m_strPath);
        if (info.m_strFile[0]=='\\' || info.m_strFile[0]=='/')
          info.m_strFile.Delete(0,1);

        if (strFile.size() > 0)
        {
          if (info.m_strFile == strFile /*|| pItem->GetLabel() == info.m_strTitle*/)
          {
            CStdString strThumb;
            CUtil::GetVideoThumbnail(info.m_strIMDBNumber,strThumb);
						if (CUtil::FileExists(strThumb))
							pItem->SetThumbnailImage(strThumb);
            
            break;
          }
        }
      }
    }
  }
}

bool CGUIWindowVideoFiles::ViewByIcon()
{
  if ( m_strDirectory.IsEmpty() )
  {
    if (g_stSettings.m_iMyVideoRootViewAsIcons != VIEW_AS_LIST) return true;
  }
  else
  {
    if (g_stSettings.m_iMyVideoViewAsIcons != VIEW_AS_LIST) return true;
  }
  return false;
}

bool CGUIWindowVideoFiles::ViewByLargeIcon()
{
  if ( m_strDirectory.IsEmpty() )
  {
    if (g_stSettings.m_iMyVideoRootViewAsIcons == VIEW_AS_LARGEICONS) return true;
  }
  else
  {
    if (g_stSettings.m_iMyVideoViewAsIcons== VIEW_AS_LARGEICONS) return true;
  }
  return false;
}

bool CGUIWindowVideoFiles::SortAscending()
{
	if (m_strDirectory.IsEmpty())
		return g_stSettings.m_bMyVideoRootSortAscending;
	else
		return g_stSettings.m_bMyVideoSortAscending;
}

int CGUIWindowVideoFiles::SortMethod()
{
	if (m_strDirectory.IsEmpty())
	{
		if (g_stSettings.m_iMyVideoRootSortMethod==0)
			return 103;
		else
			return 498;	//	Sort by: Type
	}
	else
		return g_stSettings.m_iMyVideoSortMethod+103;
}

void CGUIWindowVideoFiles::LoadPlayList(const CStdString& strPlayList)
{
  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
	CPlayListFactory factory;
	auto_ptr<CPlayList> pPlayList (factory.Create(strPlayList));
	if ( NULL != pPlayList.get())
	{
    // load it
		if (!pPlayList->Load(strPlayList))
		{
			CGUIDialogOK* pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
			if (pDlgOK)
			{
				pDlgOK->SetHeading(6);
				pDlgOK->SetLine(0,L"");
				pDlgOK->SetLine(1,477);
				pDlgOK->SetLine(2,L"");
				pDlgOK->DoModal(GetID());
			}
			return; //hmmm unable to load playlist?
		}

    // how many songs are in the new playlist
    if (pPlayList->size() == 1)
    {
      // just 1 song? then play it (no need to have a playlist of 1 song)
      CPlayList::CPlayListItem item=(*pPlayList)[0];
      g_application.PlayFile(CFileItem(item));
      return;
    }

    // clear current playlist
		g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).Clear();

    // add each item of the playlist to the playlistplayer
		for (int i=0; i < (int)pPlayList->size(); ++i)
		{
			const CPlayList::CPlayListItem& playListItem =(*pPlayList)[i];
			CStdString strLabel=playListItem.GetDescription();
			if (strLabel.size()==0) 
				strLabel=CUtil::GetTitleFromPath(playListItem.GetFileName());

			CPlayList::CPlayListItem playlistItem;
			playlistItem.SetFileName(playListItem.GetFileName());
			playlistItem.SetDescription(strLabel);
			playlistItem.SetDuration(playListItem.GetDuration());
			g_playlistPlayer.GetPlaylist( PLAYLIST_VIDEO ).Add(playlistItem);
		}
	} 

  // if we got a playlist
	if (g_playlistPlayer.GetPlaylist( PLAYLIST_VIDEO ).size() )
	{
    // then get 1st song
		CPlayList& playlist=g_playlistPlayer.GetPlaylist( PLAYLIST_VIDEO );
		const CPlayList::CPlayListItem& item=playlist[0];

    // and start playing it
		g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
		g_playlistPlayer.Reset();
		g_playlistPlayer.Play(0);

    // and activate the playlist window if its not activated yet
    if (GetID() == m_gWindowManager.GetActiveWindow())
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    }
	}
}

void CGUIWindowVideoFiles::GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)
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
			if (!g_stSettings.m_bHideParentDirItems)
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
		if (!g_stSettings.m_bHideParentDirItems)
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

/// \brief Can be overwritten to build an own history string for \c m_history
/// \param pItem Item to build the history string from
/// \param strHistoryString History string build as return value
void CGUIWindowVideoFiles::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
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

void CGUIWindowVideoFiles::SetHistoryForPath(const CStdString& strDirectory)
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

void CGUIWindowVideoFiles::SetViewMode(int iViewMode)
{
  if ( m_strDirectory.IsEmpty() )
    g_stSettings.m_iMyVideoRootViewAsIcons = iViewMode;
  else
    g_stSettings.m_iMyVideoViewAsIcons = iViewMode;
}

void CGUIWindowVideoFiles::OnPopupMenu(int iItem)
{
	// calculate our position
	int iPosX=200;
	int iPosY=100;
	CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
	if (pList)
	{
		iPosX = pList->GetXPosition()+pList->GetWidth()/2;
		iPosY = pList->GetYPosition()+pList->GetHeight()/2;
	}	
	if ( m_strDirectory.IsEmpty() )
	{
		// mark the item
		m_vecItems[iItem]->Select(true);
		// and do the popup menu
		if (CGUIDialogContextMenu::BookmarksMenu("videos", m_vecItems[iItem]->GetLabel(), m_vecItems[iItem]->m_strPath, iPosX, iPosY))
		{
			m_rootDir.SetShares(g_settings.m_vecMyVideoShares);
			Update(m_strDirectory);
			return;
		}
		m_vecItems[iItem]->Select(false);
		return;
	}
	CGUIWindowVideoBase::OnPopupMenu(iItem);
}

