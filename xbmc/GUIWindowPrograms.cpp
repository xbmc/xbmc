#include "stdafx.h"
#include "GUIWindowPrograms.h"
#include "util.h"
#include "Xbox\IoSupport.h"
#include "Xbox\Undocumented.h"
#include "Shortcut.h"
#include "filesystem/HDDirectory.h"
#include "filesystem/directorycache.h"
#include "GUIThumbnailPanel.h"
#include "GUIPassword.h"
#include "GUIDialogContextMenu.h"
#include "xbox/xbeheader.h"
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#endif

using namespace DIRECTORY;

#define CONTROL_BTNVIEWASICONS 2
#define CONTROL_BTNSORTBY      3
#define CONTROL_BTNSORTASC     4
#define CONTROL_LIST          50
#define CONTROL_THUMBS        51
#define CONTROL_LABELFILES    12

#define CONTROL_BTNSCAN       6

CGUIWindowPrograms::CGUIWindowPrograms(void)
    : CGUIMediaWindow(WINDOW_PROGRAMS, "MyPrograms.xml")
{
}


CGUIWindowPrograms::~CGUIWindowPrograms(void)
{
}

bool CGUIWindowPrograms::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_database.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      int iLastControl = m_iLastControl;
      m_iRegionSet = 0;
      CGUIWindow::OnMessage(message);
      m_database.Open();
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // remove shortcuts
      if (g_guiSettings.GetBool("MyPrograms.NoShortcuts") && g_stSettings.m_szShortcutDirectory[0] && g_settings.m_vecMyProgramsBookmarks[0].strName.Equals("shortcuts"))
        g_settings.m_vecMyProgramsBookmarks.erase(g_settings.m_vecMyProgramsBookmarks.begin());

      // check for a passed destination path
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
      }
      // otherwise, is this the first time accessing this window?
      else if (m_vecItems.m_strPath == "?")
      {
        m_vecItems.m_strPath = strDestination = g_stSettings.m_szDefaultPrograms;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }
      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // default parameters if the jump fails
        m_vecItems.m_strPath = "";
        m_shareDirectory = "";
        m_iDepth = 1;
        m_strBookmarkName = "default";
        m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);

        bool bIsBookmarkName = false;
        int iIndex = CUtil::GetMatchingShare(strDestination, g_settings.m_vecMyProgramsBookmarks, bIsBookmarkName);
        if (iIndex > -1)
        {
          // set current directory to matching share
          if (bIsBookmarkName)
            m_vecItems.m_strPath = g_settings.m_vecMyProgramsBookmarks[iIndex].strPath;
          else
            m_vecItems.m_strPath = strDestination;
          m_shareDirectory = g_settings.m_vecMyProgramsBookmarks[iIndex].strPath;
          m_iDepth = g_settings.m_vecMyProgramsBookmarks[iIndex].m_iDepthSize;
          m_strBookmarkName = g_settings.m_vecMyProgramsBookmarks[iIndex].strName;
          m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);
          CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
        }
        else
        {
          CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
        }
      }

      // make controls 100-110 invisible...
      for (int i = 100; i < 110; i++)
      {
        SET_CONTROL_HIDDEN(i);
      }

      if (g_guiSettings.GetBool("MyPrograms.NoShortcuts"))    // let's hide Scan button
      {
        SET_CONTROL_HIDDEN(CONTROL_BTNSCAN);
      }
      else
      {
        SET_CONTROL_VISIBLE(CONTROL_BTNSCAN);
      }


      int iStartID = 100;

      // create bookmark buttons

      for (int i = 0; i < (int)g_settings.m_vecMyProgramsBookmarks.size(); ++i)
      {
        CShare& share = g_settings.m_vecMyProgramsBookmarks[i];

        SET_CONTROL_VISIBLE(i + iStartID);
        SET_CONTROL_LABEL(i + iStartID, share.strName);
      }


      
      m_vecPaths.clear();
      m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);
      Update(m_vecItems.m_strPath);

      if (iLastControl > -1)
      {
        SET_CONTROL_FOCUS(iLastControl, 0);
      }
      else
      {
        SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      }

      if (m_iSelectedItem > -1)
        m_viewControl.SetSelectedItem(m_iSelectedItem);

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSCAN) // button
      {
        int iTotalApps = 0;
        CStdString strDir = m_vecItems.m_strPath;
        m_vecItems.m_strPath = g_stSettings.m_szShortcutDirectory;

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
        rootDir.GetDirectory(m_vecItems.m_strPath, m_vecItems);

        for (int i = 0; i < (int)m_vecItems.Size(); ++i)
        {
          CFileItem* pItem = m_vecItems[i];
          if (pItem->IsShortCut())
          {
            DeleteFile(pItem->m_strPath.c_str());
          }
        }

        // create new ones.

        CFileItemList items;
        {
          m_vecItems.m_strPath = "C:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("C:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }

        {
          m_vecItems.m_strPath = "E:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("E:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }

        if (g_stSettings.m_bUseFDrive)
        {
          m_vecItems.m_strPath = "F:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("F:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }
        if (g_stSettings.m_bUseGDrive)
        {
          m_vecItems.m_strPath = "G:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("G:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }


        m_vecItems.m_strPath = strDir;
        CUtil::ClearCache();
        Update(m_vecItems.m_strPath);

        if (m_dlgProgress)
        {
          m_dlgProgress->Close();
        }
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        // get selected item
        int iAction = message.GetParam1();
        if (ACTION_CONTEXT_MENU == iAction)
        {
          int iItem = m_viewControl.GetSelectedItem();
          // iItem is checked inside OnPopupMenu
          if (OnPopupMenu(iItem))
          { // update bookmark buttons
            int iStartID = 100;
            for (int i = 0; i < (int)g_settings.m_vecMyProgramsBookmarks.size(); ++i)
            {
              CShare& share = g_settings.m_vecMyProgramsBookmarks[i];
              SET_CONTROL_VISIBLE(i + iStartID);
              SET_CONTROL_LABEL(i + iStartID, share.strName);
            }
            // erase the button following the last updated button
            // incase the user just deleted a bookmark
            SET_CONTROL_HIDDEN((int)g_settings.m_vecMyProgramsBookmarks.size() + iStartID);
            SET_CONTROL_LABEL((int)g_settings.m_vecMyProgramsBookmarks.size() + iStartID, "");
          }
        }
      }
      else if (iControl >= 100 && iControl <= 110)
      {
        // bookmark button
        int iShare = iControl - 100;
        if (iShare < (int)g_settings.m_vecMyProgramsBookmarks.size())
        {
          CShare share = g_settings.m_vecMyProgramsBookmarks[iControl - 100];
          // do nothing if the bookmark is locked, and update the panel with new bookmark settings
          if ( !g_passwordManager.IsItemUnlocked( &share, "myprograms" ) )
          {
            UpdateDir("");
            return false;
          }
          m_shareDirectory = share.strPath;    // since m_strDirectory can change, we always want something that won't.
          m_vecItems.m_strPath = share.strPath;
          m_strBookmarkName = share.strName;
          m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);  // fetch all paths in this bookmark
          m_iDepth = share.m_iDepthSize;
          Update(m_vecItems.m_strPath);
        }
      }
    }
    break;
  }

  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowPrograms::OnPopupMenu(int iItem)
{
  // calculate our position
  int iPosX = 200;
  int iPosY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    iPosX = pList->GetXPosition() + pList->GetWidth() / 2;
    iPosY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  if ( m_vecItems.IsVirtualDirectoryRoot() )
  {
    if (iItem < 0)
    { // TODO: Add option to add shares in this case
      return false;
    }
    // mark the item
    m_vecItems[iItem]->Select(true);

    bool bMaxRetryExceeded = false;
    if (g_stSettings.m_iMasterLockMaxRetry != 0)
      bMaxRetryExceeded = !(m_vecItems[iItem]->m_iBadPwdCount < g_stSettings.m_iMasterLockMaxRetry);

    // and do the popup menu
    if (CGUIDialogContextMenu::BookmarksMenu("myprograms", m_vecItems[iItem]->GetLabel(), m_vecItems[iItem]->m_strPath, m_vecItems[iItem]->m_iLockMode, bMaxRetryExceeded, iPosX, iPosY))
    {
      Update("");
      return true;
    }
    m_vecItems[iItem]->Select(false);
    return false;
  }
  else if (iItem >= 0)// assume a program
  {
    // mark the item
    m_vecItems[iItem]->Select(true);
    // popup the context menu
    CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
    if (!pMenu) return false;
    // load our menu
    pMenu->Initialize();
    // add the needed buttons
    
    CStdStringW strLaunch = g_localizeStrings.Get(518); // Launch
    if (g_guiSettings.GetBool("MyPrograms.GameAutoRegion"))
    {
      int iRegion = GetRegion(iItem);
      if (iRegion == VIDEO_NTSCM)
        strLaunch += " (NTSC-M)";
      if (iRegion == VIDEO_NTSCJ)
        strLaunch += " (NTSC-J)";
      if (iRegion == VIDEO_PAL50)
        strLaunch += " (PAL)";
      if (iRegion == VIDEO_PAL60)
        strLaunch += " (PAL-60)";
    }
    
    int btn_Launch = pMenu->AddButton(strLaunch); // launch
    int btn_Rename = -2;
    int btn_LaunchIn = -2;
    if (g_guiSettings.GetBool("MyPrograms.GameAutoRegion"))
      btn_LaunchIn = pMenu->AddButton(519); // launch in video mode
    if (m_vecItems[iItem]->IsType(".xbe"))
      btn_Rename = pMenu->AddButton(520); // edit xbe title
    int btn_Settings = pMenu->AddButton(5); // Settings

    // position it correctly
    pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
    pMenu->DoModal(GetID());

    int btnid = pMenu->GetButton();
    if (!btnid)
    {
      m_vecItems[iItem]->Select(false);
      return false;
    }

    if (btnid == btn_Rename)
    {
      CStdString strDescription = m_vecItems[iItem]->GetLabel();
      if (CGUIDialogKeyboard::ShowAndGetInput(strDescription, (CStdStringW)g_localizeStrings.Get(16013), false))
      {
        // truncate to 39 characters before updating xbe header
        if (strDescription.size() > 39)
          strDescription = strDescription.Left(39);
        CUtil::SetXBEDescription(m_vecItems[iItem]->m_strPath,strDescription);
        m_database.SetDescription(m_vecItems[iItem]->m_strPath,strDescription);
        Update(m_vecItems.m_strPath);
      }
    }
    if (btnid == btn_Settings)
    {
      //MasterPassword
      int iLockSettings = g_guiSettings.GetInt("Masterlock.LockSettingsFilemanager");
      if (iLockSettings == 1 || iLockSettings == 3) 
      {
        if (g_passwordManager.IsMasterLockLocked(true))
          m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPROGRAMS);
      }
      else m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPROGRAMS); 
    }
    else if (btnid == btn_Launch)
    {
      OnClick(iItem);
      return true;
    }
    else if (btnid == btn_LaunchIn)
    {
      pMenu->Initialize();
      int btn_PAL;
      int btn_NTSCM;
      int btn_NTSCJ;
      int btn_PAL60;
      CStdStringW strPAL, strNTSCJ, strNTSCM, strPAL60;
      strPAL = "PAL";
      strNTSCM = "NTSC-M";
      strNTSCJ = "NTSC-J";
      strPAL60 = "PAL-60";
      int iRegion = GetRegion(iItem,true);

      if (iRegion == VIDEO_NTSCM)
        strNTSCM += " (default)";
      if (iRegion == VIDEO_NTSCJ)
        strNTSCJ += " (default)";
      if (iRegion == VIDEO_PAL50)
        strPAL += " (default)";

      btn_PAL = pMenu->AddButton(strPAL);
      btn_NTSCM = pMenu->AddButton(strNTSCM);
      btn_NTSCJ = pMenu->AddButton(strNTSCJ);
      btn_PAL60 = pMenu->AddButton(strPAL60);
      
      pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
      pMenu->DoModal(GetID());
      int btnid = pMenu->GetButton();
      
      if (btnid == btn_NTSCM)
      {
        m_iRegionSet = VIDEO_NTSCM;
        m_database.SetRegion(m_vecItems[iItem]->m_strPath,1);
      }
      if (btnid == btn_NTSCJ)
      {
        m_iRegionSet = VIDEO_NTSCJ;
        m_database.SetRegion(m_vecItems[iItem]->m_strPath,2);
      }
      if (btnid == btn_PAL)
      {
        m_iRegionSet = VIDEO_PAL50;
        m_database.SetRegion(m_vecItems[iItem]->m_strPath,4);
      }
      if (btnid == btn_PAL60)
      {
        m_iRegionSet = VIDEO_PAL60;
        m_database.SetRegion(m_vecItems[iItem]->m_strPath,8);
      }

      if (btnid > -1)
        OnClick(iItem);
    }
  }
  else
    return false;

  m_vecItems[iItem]->Select(false);
  return true;
}

void CGUIWindowPrograms::LoadDirectory(const CStdString& strDirectory, int idepth)
{
  WIN32_FIND_DATA wfd;
  bool bOnlyDefaultXBE = g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
  bool bFlattenDir = g_guiSettings.GetBool("MyPrograms.Flatten");
  bool bUseDirectoryName = g_guiSettings.GetBool("MyPrograms.UseDirectoryName");

  memset(&wfd, 0, sizeof(wfd));
  CStdString strRootDir = strDirectory;
  if (!CUtil::HasSlashAtEnd(strRootDir) )
    strRootDir += "\\";

  CFileItem rootDir(strRootDir, true);
  if ( rootDir.IsDVD() )
  {
    CIoSupport helper;
    helper.Remount("D:", "Cdrom0");
  }
  CStdString strSearchMask = strRootDir;
  strSearchMask += "*.*";

  FILETIME localTime;
  CAutoPtrFind hFind ( FindFirstFile(strSearchMask.c_str(), &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if (wfd.cFileName[0] != 0)
    {
      CFileItem fileName(wfd.cFileName, false);
      CFileItem file(strRootDir + CStdString(wfd.cFileName), false);

      if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      {
        if ( fileName.m_strPath != "." && fileName.m_strPath != ".." && !bFlattenDir)
        {
          CFileItem *pItem = new CFileItem(fileName.m_strPath);
          pItem->m_strPath = file.m_strPath;
          pItem->m_bIsFolder = true;
          FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
          FileTimeToSystemTime(&localTime, &pItem->m_stTime);
          m_vecItems.Add(pItem);
        }
        else
        {
          if (fileName.m_strPath != "." && fileName.m_strPath != ".." && bFlattenDir && idepth > 0 )
          {
            m_vecPaths1.push_back(file.m_strPath);
            bool foundPath = false;                             
            for (int i = 0; i < (int)m_vecPaths.size(); i++)    
            
            {
              if (file.m_strPath == m_vecPaths[i])
              {
                foundPath = true;              
                break;
              }                                                 
            }                                                   
            if (!foundPath)                                     
              LoadDirectory(file.m_strPath, idepth - 1);
            
          }
        }

      }
      else
      {
        if (bOnlyDefaultXBE ? fileName.IsDefaultXBE() : fileName.IsXBE())
        {
          CStdString strDescription;

          if (!CUtil::GetXBEDescription(file.m_strPath, strDescription) || (bUseDirectoryName && fileName.IsDefaultXBE()) )
          {
            CUtil::GetDirectoryName(file.m_strPath, strDescription);
            CUtil::ShortenFileName(strDescription);
            CUtil::RemoveIllegalChars(strDescription);
          }

          if (!bFlattenDir || file.IsOnDVD())
          {
            CFileItem *pItem = new CFileItem(strDescription);
            pItem->m_strPath = file.m_strPath;
            pItem->m_bIsFolder = false;
            pItem->m_strTitle=strDescription;
            //pItem->m_dwSize = wfd.nFileSizeLow;

            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            FileTimeToSystemTime(&localTime, &pItem->m_stTime);
            m_vecItems.Add(pItem);
          }
          else
          {
            DWORD titleId = CUtil::GetXbeID(file.m_strPath);
            m_database.AddProgram(file.m_strPath, titleId, strDescription, m_strBookmarkName);  
          }
        }

        if (fileName.IsShortCut())
        {
          if (!bFlattenDir)
          {
            CFileItem *pItem = new CFileItem(wfd.cFileName);
            pItem->m_strPath = file.m_strPath;
            pItem->m_bIsFolder = false;
            //pItem->m_dwSize = wfd.nFileSizeLow;

            FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &localTime);
            FileTimeToSystemTime(&localTime, &pItem->m_stTime);
            m_vecItems.Add(pItem);
          }

          else
          {
            DWORD titleId = CUtil::GetXbeID(file.m_strPath);
            m_database.AddProgram(file.m_strPath, titleId, wfd.cFileName, m_strBookmarkName);
          }
           
        }
      }
    }
  }
  while (FindNextFile(hFind, &wfd));
}

bool CGUIWindowPrograms::Update(const CStdString &strDirectory)
{
  UpdateDir(strDirectory);
  if (g_guiSettings.GetBool("ProgramFiles.UseAutoSwitching"))
  {
    UpdateButtons();
  }
  return true;
}

void CGUIWindowPrograms::UpdateDir(const CStdString &strDirectory)
{
  bool bFlattenDir = g_guiSettings.GetBool("MyPrograms.Flatten");
  bool bOnlyDefaultXBE = g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
  bool bParentPath(false);
  bool bPastBookMark(true);
  CStdString strParentPath;
  CStdString strDir = strDirectory;
  CStdString strShortCutsDir = g_stSettings.m_szShortcutDirectory;
  int idepth = m_iDepth;
  vector<CStdString> vecPaths;
  m_isRoot = false;
  ClearFileItems();

  if (strDirectory == "")  // display only the root shares
  {
    m_isRoot = true;
    for (int i = 0; i < (int)g_settings.m_vecMyProgramsBookmarks.size(); ++i)
    {
      CShare& share = g_settings.m_vecMyProgramsBookmarks[i];
      vector<CStdString> vecShares;
      CFileItem *pItem = new CFileItem(share);
      pItem->m_bIsShareOrDrive = false;
      CUtil::Tokenize(pItem->m_strPath, vecShares, ",");
      CStdString strThumb;
      for (int j = 0; j < (int)vecShares.size(); j++)    // use the first folder image that we find in the vector of shares
      {
        CFileItem item(vecShares[j], false);
        if (!item.IsXBE())
        {
          if (CUtil::GetFolderThumb(item.m_strPath, strThumb))
          {
            pItem->SetThumbnailImage(strThumb);
            break;
          }
        }
      }
      m_vecItems.Add(pItem);
    }

    m_strParentPath = "";
  }

  CUtil::Tokenize(strDir, vecPaths, ",");

  if (!g_guiSettings.GetBool("MyPrograms.NoShortcuts"))
  {
    if (CUtil::HasSlashAtEnd(strShortCutsDir))
      strShortCutsDir.Delete(strShortCutsDir.size() - 1);

    if (vecPaths.size() > 0 && vecPaths[0] == strShortCutsDir)
      m_strBookmarkName = "shortcuts";
      m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);
  }

  for (int k = 0; k < (int)vecPaths.size(); k++)
  {
    int start = vecPaths[k].find_first_not_of(" \t");
    int end = vecPaths[k].find_last_not_of(" \t") + 1;
    vecPaths[k] = vecPaths[k].substr(start, end - start);
    if (CUtil::HasSlashAtEnd(vecPaths[k]))
    {
      vecPaths[k].Delete(vecPaths[k].size() - 1);
    }
  }

  if (!bFlattenDir)
  {
    vector<CStdString> vecOrigShare;
    CUtil::Tokenize(m_shareDirectory, vecOrigShare, ",");
    if (vecOrigShare.size() && vecPaths.size())
    {
      bParentPath = CUtil::GetParentPath(vecPaths[0], strParentPath);
      if (CUtil::HasSlashAtEnd(vecOrigShare[0]))
        vecOrigShare[0].Delete(vecOrigShare[0].size() - 1);
      if (strParentPath < vecOrigShare[0])
      {
        bPastBookMark = false;
        strParentPath = "";
      }
    }
  }
  else
  {
    strParentPath = "";
  }

  if (strDirectory != "")
  {
    auto_ptr<CGUIViewState> pState(CGUIViewState::GetViewState(GetID(), m_vecItems));
    if (pState.get() && !pState->HideParentDirItems())
    {
      CFileItem *pItem = new CFileItem("..");
      pItem->m_strPath = strParentPath;
      pItem->m_bIsShareOrDrive = false;
      pItem->m_bIsFolder = true;
      m_vecItems.Add(pItem);
    }
    m_strParentPath = strParentPath;
  }

  m_iLastControl = GetFocusedControl();

  if (!m_vecItems.IsVirtualDirectoryRoot())
  {
    for (int j = 0; j < (int)vecPaths.size(); j++)
    {
      CFileItem item(vecPaths[j], false);
      if (item.IsXBE())     // we've found a single XBE in the path vector
      {
        CStdString strDescription;
        if ( (m_database.GetFile(item.m_strPath, m_vecItems) < 0) && CFile::Exists(item.m_strPath))
        {
          if (!CUtil::GetXBEDescription(item.m_strPath, strDescription))
          {
            CUtil::GetDirectoryName(item.m_strPath, strDescription);
            CUtil::ShortenFileName(strDescription);
            CUtil::RemoveIllegalChars(strDescription);
          }

          if (item.IsOnDVD())
          {
            WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
            FILETIME localTime;
            CFileItem *pItem = new CFileItem(strDescription);
            pItem->m_strPath = item.m_strPath;
            pItem->m_bIsFolder = false;
            pItem->m_strTitle=strDescription;
            GetFileAttributesEx(pItem->m_strPath, GetFileExInfoStandard, &FileAttributeData);
            FileTimeToLocalFileTime(&FileAttributeData.ftLastWriteTime, &localTime);
            FileTimeToSystemTime(&localTime, &pItem->m_stTime);
            pItem->m_dwSize = FileAttributeData.nFileSizeLow;
            m_vecItems.Add(pItem);
          }
          else
          {
            DWORD titleId = CUtil::GetXbeID(item.m_strPath);
            m_database.AddProgram(item.m_strPath, titleId, strDescription, m_strBookmarkName);
            m_database.GetFile(item.m_strPath, m_vecItems);
          }
        }
        CStdString directoryName;
        CUtil::GetDirectoryName(item.m_strPath, directoryName);        
        bool found = false;
        for (int i = 0; i < (int)m_vecPaths1.size(); i++)
        {
          if (m_vecPaths1[i] == directoryName) 
          {
            found = true;
            break;
          }
        }
        if (!found)  m_vecPaths1.push_back(directoryName);
      }
      else
      {
        LoadDirectory(item.m_strPath, idepth);
//      m_database.GetProgramsByPath(item.m_strPath, m_vecItems, idepth, bOnlyDefaultXBE);
      }
    }


    if (m_strBookmarkName == "shortcuts")
      bOnlyDefaultXBE = false;   // let's do this so that we don't only grab default.xbe from database when getting shortcuts
    if (bFlattenDir)
    {
      // let's first delete the items in database that no longer exist       
      if ( (int)m_vecPaths.size() != (int)m_vecPaths1.size())
      {
        bool found = true;
        for (int j = 0; j < (int)m_vecPaths.size(); j++)
        {
          for (int i = 0; i < (int)m_vecPaths1.size(); i++)
          { 
            if (m_vecPaths1[i]==m_vecPaths[j])
            {
              found = true;
              break;
            }
            found = false;
          }
          if (!found)
          {
            CStdString strDeletePath = m_vecPaths[j];
            strDeletePath.Replace("\\", "/");
            m_database.DeleteProgram(strDeletePath);
          }
        }
      }
      m_database.GetProgramsByBookmark(m_strBookmarkName, m_vecItems, idepth, bOnlyDefaultXBE);
    }
  }  
  
  CUtil::ClearCache();
  m_vecItems.SetThumbs();
  auto_ptr<CGUIViewState> pState(CGUIViewState::GetViewState(GetID(), m_vecItems));
  if (pState.get() && pState->HideExtensions())
    m_vecItems.RemoveExtensions();
  m_vecItems.FillInDefaultIcons();
  OnSort();
  UpdateButtons();
}

void CGUIWindowPrograms::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  if (pItem->m_bIsFolder)
  {
    // do nothing if the bookmark is locked
    if ( !g_passwordManager.IsItemUnlocked( pItem, "myprograms" ) )
      return ;

    if (m_vecItems.IsVirtualDirectoryRoot())
      m_shareDirectory = pItem->m_strPath;
    m_vecItems.m_strPath = pItem->m_strPath;
    m_iDepth = pItem->m_idepth;
    bool bIsBookmarkName(false);
    if (m_isRoot) {
      int iIndex = CUtil::GetMatchingShare(m_shareDirectory, g_settings.m_vecMyProgramsBookmarks, bIsBookmarkName);
      if (iIndex > -1)
      {
        m_vecPaths.clear();
        m_vecPaths1.clear();
        m_strBookmarkName = g_settings.m_vecMyProgramsBookmarks[iIndex].strName;
        m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);
      }
    }
    Update(m_vecItems.m_strPath);
  }
  else
  {

    // launch xbe...
    char szPath[1024];
    char szParameters[1024];
    if (g_guiSettings.GetBool("MyPrograms.Flatten"))
      m_database.IncTimesPlayed(pItem->m_strPath);

    int iRegion = m_iRegionSet?m_iRegionSet:GetRegion(iItem);

    m_database.Close();
    memset(szParameters, 0, sizeof(szParameters));

    strcpy(szPath, pItem->m_strPath.c_str());

    if (pItem->IsShortCut())
    {
      CShortcut shortcut;
      if ( shortcut.Create(pItem->m_strPath))
      {
        CFileItem item(shortcut.m_strPath, false);
        // if another shortcut is specified, load this up and use it
        if (item.IsShortCut())
        {
          CHAR szNewPath[1024];
          strcpy(szNewPath, szPath);
          CHAR* szFile = strrchr(szNewPath, '\\');
          strcpy(&szFile[1], shortcut.m_strPath.c_str());

          CShortcut targetShortcut;
          if (FAILED(targetShortcut.Create(szNewPath)))
            return ;

          shortcut.m_strPath = targetShortcut.m_strPath;
        }

        strcpy( szPath, shortcut.m_strPath.c_str() );

        CHAR szMode[16];
        strcpy( szMode, shortcut.m_strVideo.c_str() );
        strlwr( szMode );

        LPDWORD pdwVideo = (LPDWORD) 0x8005E760;
        BOOL bRow = strstr(szMode, "pal") != NULL;
        BOOL bJap = strstr(szMode, "ntsc-j") != NULL;
        BOOL bUsa = strstr(szMode, "ntsc") != NULL;

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
      CUtil::RunXBE(szPath, szParameters,F_VIDEO(iRegion));
    else
      CUtil::RunXBE(szPath,NULL,F_VIDEO(iRegion));
  }
}

void CGUIWindowPrograms::OnScan(CFileItemList& items, int& iTotalAppsFound)
{
  // remove username + password from m_strDirectory for display in Dialog
  CURL url(m_vecItems.m_strPath);
  CStdString strStrippedPath;
  url.GetURLWithoutUserDetails(strStrippedPath);

  CStdString strText = g_localizeStrings.Get(212);
  CStdString strTotal;
  strTotal.Format("%i %s", iTotalAppsFound, strText.c_str());
  if (m_dlgProgress)
  {
    m_dlgProgress->SetLine(0, strTotal);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, strStrippedPath);
    m_dlgProgress->Progress();
  }
  //bool   bOnlyDefaultXBE=g_guiSettings.GetBool("MyPrograms.DefaultXBEOnly");
  bool bScanSubDirs = true;
  bool bFound = false;
  DeleteThumbs(items);
  //CUtil::SetThumbs(items);
  CUtil::CreateShortcuts(items);
  if ((int)m_vecItems.m_strPath.size() != 2) // true for C:, E:, F:, G:
  {
    // first check all files
    for (int i = 0; i < (int)items.Size(); ++i)
    {
      CFileItem *pItem = items[i];
      if (! pItem->m_bIsFolder)
      {
        if (pItem->IsXBE() )
        {
          bScanSubDirs = false;
          break;
        }
      }
    }
  }

  for (int i = 0; i < (int)items.Size(); ++i)
  {
    CFileItem *pItem = items[i];
    if (pItem->m_bIsFolder)
    {
      if (bScanSubDirs && !bFound && !pItem->IsParentFolder())
      {
        // load subfolder
        CStdString strDir = m_vecItems.m_strPath;
        if (pItem->m_strPath != "E:\\UDATA" && pItem->m_strPath != "E:\\TDATA")
        {
          m_vecItems.m_strPath = pItem->m_strPath;
          CFileItemList subDirItems;
          CHDDirectory rootDir;
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory(pItem->m_strPath, subDirItems);
          OnScan(subDirItems, iTotalAppsFound);
          subDirItems.Clear();
          m_vecItems.m_strPath = strDir;
        }
      }
    }
    else
    {
      if (pItem->IsXBE())
      {
        bFound = true;
        iTotalAppsFound++;

        CStdString strText = g_localizeStrings.Get(212);
        strTotal.Format("%i %s", iTotalAppsFound, strText.c_str());
        CStdString strDescription;
        CUtil::GetXBEDescription(pItem->m_strPath, strDescription);
        if (strDescription == "")
          strDescription = CUtil::GetFileName(pItem->m_strPath);
        if (m_dlgProgress)
        {
          m_dlgProgress->SetLine(0, strTotal);
          m_dlgProgress->SetLine(1, "");
          m_dlgProgress->SetLine(2, strStrippedPath);
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

void CGUIWindowPrograms::DeleteThumbs(CFileItemList& items)
{
  CUtil::ClearCache();
  for (int i = 0; i < (int)items.Size(); ++i)
  {
    CFileItem *pItem = items[i];
    if (! pItem->m_bIsFolder)
    {
      if (pItem->IsXBE() )
      {
        CStdString strThumb;
        CUtil::GetXBEIcon(pItem->m_strPath, strThumb);
        CStdString strName = pItem->m_strPath;
        CUtil::ReplaceExtension(pItem->m_strPath, ".tbn", strName);
        if (strName != strThumb)
        {
          ::DeleteFile(strThumb.c_str());
        }
      }
    }
  }
}

/// \brief Call to go to parent folder
void CGUIWindowPrograms::GoParentFolder()
{
  //CStdString strPath=m_strParentPath;
  m_vecItems.m_strPath = m_strParentPath;
  Update(m_vecItems.m_strPath);
}


void CGUIWindowPrograms::OnWindowLoaded()
{
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  if (g_SkinInfo.GetVersion() < 1.8)
  {
    ChangeControlID(7, CONTROL_LIST, CGUIControl::GUICONTROL_LIST);
    ChangeControlID(8, CONTROL_THUMBS, CGUIControl::GUICONTROL_THUMBNAIL);
    ChangeControlID(9, CONTROL_LABELFILES, CGUIControl::GUICONTROL_LABEL);
    ChangeControlID(3, CONTROL_BTNSCAN, CGUIControl::GUICONTROL_BUTTON);
    ChangeControlID(4, CONTROL_BTNSORTBY, CGUIControl::GUICONTROL_BUTTON);
    ChangeControlID(5, CONTROL_BTNSORTASC, CGUIControl::GUICONTROL_TOGGLEBUTTON);
  }
#endif
  CGUIMediaWindow::OnWindowLoaded();
}

int CGUIWindowPrograms::GetRegion(int iItem, bool bReload)
{
  if (!g_guiSettings.GetBool("MyPrograms.GameAutoRegion"))
    return 0;

  int iRegion;
  if (bReload || m_vecItems[iItem]->IsOnDVD())
  {
    CXBE xbe;
    iRegion = xbe.ExtractGameRegion(m_vecItems[iItem]->m_strPath); 
  }
  else
  {
    m_database.Open();
    iRegion = m_database.GetRegion(m_vecItems[iItem]->m_strPath);
    m_database.Close();
  }
  if (iRegion == -1)
  {
    if (g_guiSettings.GetBool("MyPrograms.GameAutoRegion"))
    {
      CXBE xbe;
      iRegion = xbe.ExtractGameRegion(m_vecItems[iItem]->m_strPath);
      if (iRegion < 1 || iRegion > 7)
        iRegion = 0;
    }
    else
      iRegion = 0;
    m_database.SetRegion(m_vecItems[iItem]->m_strPath,iRegion);
  }
  
  if (bReload)
    return CXBE::FilterRegion(iRegion,true);
  else
    return CXBE::FilterRegion(iRegion);
}