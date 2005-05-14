#include "stdafx.h"
#include "GUIWindowPrograms.h"
#include "util.h"
#include "Xbox\IoSupport.h"
#include "Xbox\Undocumented.h"
#include "lib/cximage/ximage.h"
#include "Shortcut.h"
#include "application.h"
#include "filesystem/HDDirectory.h"
#include "filesystem/directorycache.h"
#include "GUIThumbnailPanel.h"
#include "AutoSwitch.h"
#include "GUIPassword.h"
#include "GUIDialogContextMenu.h"

using namespace DIRECTORY;

#define CONTROL_BTNVIEWAS     2
#define CONTROL_BTNSCAN       3
#define CONTROL_BTNSORTMETHOD 4
#define CONTROL_BTNSORTASC    5
#define CONTROL_LIST          7
#define CONTROL_THUMBS        8
#define CONTROL_LABELFILES    9

CGUIWindowPrograms::CGUIWindowPrograms(void)
    : CGUIWindow(0)
{
  m_Directory.m_strPath = "?";
  m_Directory.m_bIsFolder = true;
  m_iLastControl = -1;
  m_iViewAsIcons = -1;
}


CGUIWindowPrograms::~CGUIWindowPrograms(void)
{}

bool CGUIWindowPrograms::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_iLastControl = GetFocusedControl();
      m_iSelectedItem = m_viewControl.GetSelectedItem();
      ClearFileItems();
      m_database.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      int iLastControl = m_iLastControl;
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
      else if (m_Directory.m_strPath == "?")
      {
        m_Directory.m_strPath = strDestination = g_stSettings.m_szDefaultPrograms;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }
      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // default parameters if the jump fails
        m_Directory.m_strPath = "";
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
            m_Directory.m_strPath = g_settings.m_vecMyProgramsBookmarks[iIndex].strPath;
          else
            m_Directory.m_strPath = strDestination;
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

      if (m_iViewAsIcons == -1)
      {
        m_iViewAsIcons = g_stSettings.m_iMyProgramsViewAsIcons;
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


      UpdateDir(m_Directory.m_strPath);

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
      if (iControl == CONTROL_BTNVIEWAS)
      {
        bool bLargeIcons(false);
        m_iViewAsIcons++;
        if (m_iViewAsIcons > VIEW_AS_LARGE_ICONS) m_iViewAsIcons = VIEW_AS_LIST;
        g_stSettings.m_iMyProgramsViewAsIcons = m_iViewAsIcons;
        g_settings.Save();

        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNSCAN) // button
      {
        int iTotalApps = 0;
        CStdString strDir = m_Directory.m_strPath;
        m_Directory.m_strPath = g_stSettings.m_szShortcutDirectory;

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
        rootDir.GetDirectory(m_Directory.m_strPath, m_vecItems);

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
          m_Directory.m_strPath = "C:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("C:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }

        {
          m_Directory.m_strPath = "E:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("E:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }

        if (g_stSettings.m_bUseFDrive)
        {
          m_Directory.m_strPath = "F:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("F:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }
        if (g_stSettings.m_bUseGDrive)
        {
          m_Directory.m_strPath = "G:";
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory("G:\\", items);
          OnScan(items, iTotalApps);
          items.Clear();
        }


        m_Directory.m_strPath = strDir;
        CUtil::ClearCache();
        Update(m_Directory.m_strPath);

        if (m_dlgProgress)
        {
          m_dlgProgress->Close();
        }
      }
      else if (iControl == CONTROL_BTNSORTMETHOD) // sort by
      {
        g_stSettings.m_iMyProgramsSortMethod++;
        if (g_stSettings.m_iMyProgramsSortMethod >= 3)
          g_stSettings.m_iMyProgramsSortMethod = 0;
        g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        g_stSettings.m_bMyProgramsSortAscending = !g_stSettings.m_bMyProgramsSortAscending;
        g_settings.Save();

        UpdateButtons();
        OnSort();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        // get selected item
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
        {
          int iItem = m_viewControl.GetSelectedItem();
          // iItem is checked within the OnClick routine
          OnClick(iItem);
        }
        else if (ACTION_CONTEXT_MENU == iAction)
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
          m_Directory.m_strPath = share.strPath;
          m_strBookmarkName = share.strName;
          m_database.GetPathsByBookmark(m_strBookmarkName, m_vecPaths);  // fetch all paths in this bookmark
          m_iDepth = share.m_iDepthSize;
          Update(m_Directory.m_strPath);
        }
      }
    }
    break;
  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
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
  if ( m_Directory.IsVirtualDirectoryRoot() )
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
  }
  return false;
}

void CGUIWindowPrograms::Render()
{
  CGUIWindow::Render();
}


bool CGUIWindowPrograms::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PARENT_DIR)
  {
    GoParentFolder();
    return true;
  }

  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }

  return CGUIWindow::OnAction(action);
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

          if (!bFlattenDir || file.IsDVD())
          {
            CFileItem *pItem = new CFileItem(strDescription);
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


void CGUIWindowPrograms::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.Clear();
}

void CGUIWindowPrograms::Update(const CStdString &strDirectory)
{
  UpdateDir(strDirectory);
  if (g_guiSettings.GetBool("ProgramsLists.UseAutoSwitching"))
  {
    m_iViewAsIcons = CAutoSwitch::GetView(m_vecItems);
    UpdateButtons();
  }
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
    if (!g_guiSettings.GetBool("ProgramsLists.HideParentDirItems"))
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

  if (!m_Directory.IsVirtualDirectoryRoot())
  {
    for (int j = 0; j < (int)vecPaths.size(); j++)
    {
      CFileItem item(vecPaths[j], false);
      if (item.IsXBE())     // we've found a single XBE in the path vector
      {
        CStdString strDescription;
        if ( (m_database.GetFile(item.m_strPath, m_vecItems) < 0) && CUtil::FileExists(item.m_strPath))
        {
          if (!CUtil::GetXBEDescription(item.m_strPath, strDescription))
          {
            CUtil::GetDirectoryName(item.m_strPath, strDescription);
            CUtil::ShortenFileName(strDescription);
            CUtil::RemoveIllegalChars(strDescription);
          }

          if (item.IsDVD())
          {
            WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
            FILETIME localTime;
            CFileItem *pItem = new CFileItem(strDescription);
            pItem->m_strPath = item.m_strPath;
            pItem->m_bIsFolder = false;
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
  if (g_guiSettings.GetBool("FileLists.HideExtensions"))
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

    if (m_Directory.IsVirtualDirectoryRoot())
      m_shareDirectory = pItem->m_strPath;
    m_Directory.m_strPath = pItem->m_strPath;
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
    Update(m_Directory.m_strPath);
  }
  else
  {

    // launch xbe...
    char szPath[1024];
    char szParameters[1024];
    if (g_guiSettings.GetBool("MyPrograms.Flatten"))
      m_database.IncTimesPlayed(pItem->m_strPath);
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
      CUtil::RunXBE(szPath, szParameters);
    else
      CUtil::RunXBE(szPath);
  }
}

struct SSortProgramsByName
{
  static bool Sort(CFileItem* pStart, CFileItem* pEnd)
  {
    CFileItem& rpStart = *pStart;
    CFileItem& rpEnd = *pEnd;
    if (rpStart.GetLabel() == "..")
      return true;
    if (rpEnd.GetLabel() == "..")
      return false;
    bool bGreater = true;
    if (g_stSettings.m_bMyProgramsSortAscending)
      bGreater = false;
    if ( rpStart.m_bIsFolder == rpEnd.m_bIsFolder)
    {
      char szfilename1[1024];
      char szfilename2[1024];

      switch ( g_stSettings.m_iMyProgramsSortMethod )
      {
      case 0:  //  Sort by Filename
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
      case 1:  // Sort by Date
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

/*      case 2:
        if ( rpStart.m_dwSize > rpEnd.m_dwSize)
          return bGreater;
        if ( rpStart.m_dwSize < rpEnd.m_dwSize)
          return !bGreater;
        return true;
        break;
*/
      case 2:
        if (rpStart.m_iprogramCount > rpEnd.m_iprogramCount)
          return bGreater;
        if (rpStart.m_iprogramCount < rpEnd.m_iprogramCount)
          return !bGreater;
        return true;
        break;

      default:   //  Sort by Filename by default
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
      }


      for (int i = 0; i < (int)strlen(szfilename1); i++)
        szfilename1[i] = tolower((unsigned char)szfilename1[i]);

      for (i = 0; i < (int)strlen(szfilename2); i++)
        szfilename2[i] = tolower((unsigned char)szfilename2[i]);
      //return (rpStart.strPath.compare( rpEnd.strPath )<0);

      if (g_stSettings.m_bMyProgramsSortAscending)
        return (strcmp(szfilename1, szfilename2) < 0);
      else
        return (strcmp(szfilename1, szfilename2) >= 0);
    }
    if (!rpStart.m_bIsFolder)
      return false;
    return true;
  }
};

void CGUIWindowPrograms::OnSort()
{
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    if (g_stSettings.m_iMyProgramsSortMethod == 0)
    {
      if (pItem->m_bIsFolder)
        pItem->SetLabel2("");
      else
      {
        CStdString strDateTime;
        CUtil::GetDate(pItem->m_stTime, strDateTime);
        pItem->SetLabel2(strDateTime);
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
    if (g_stSettings.m_iMyProgramsSortMethod == 2)
    {
      if (pItem->m_bIsFolder)
        pItem->SetLabel2("");
      else
      {
        CStdString strTimesPlayed;
        strTimesPlayed.Format("%i", pItem->m_iprogramCount);
        pItem->SetLabel2(strTimesPlayed);
      }
    }
  }


  m_vecItems.Sort(SSortProgramsByName::Sort);

  m_viewControl.SetItems(m_vecItems);
}

void CGUIWindowPrograms::UpdateButtons()
{
  m_viewControl.SetCurrentView(m_iViewAsIcons);

  if (g_stSettings.m_iMyProgramsSortMethod == 2)
  {
    SET_CONTROL_LABEL(CONTROL_BTNSORTMETHOD, 507);  //Times Played
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTNSORTMETHOD, g_stSettings.m_iMyProgramsSortMethod + 103);
  }

  if ( g_stSettings.m_bMyProgramsSortAscending)
  {
    CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CONTROL_BTNSORTASC);
    g_graphicsContext.SendMessage(msg);
  }

  int iItems = m_vecItems.Size();
  if (iItems)
  {
    CFileItem* pItem = m_vecItems[0];
    if (pItem->GetLabel() == "..")
      iItems--;
  }
  WCHAR wszText[20];
  const WCHAR* szText = g_localizeStrings.Get(127).c_str();
  swprintf(wszText, L"%i %s", iItems, szText);

  SET_CONTROL_LABEL(CONTROL_LABELFILES, wszText);


}

void CGUIWindowPrograms::OnScan(CFileItemList& items, int& iTotalAppsFound)
{
  // remove username + password from m_strDirectory for display in Dialog
  CURL url(m_Directory.m_strPath);
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
  if ((int)m_Directory.m_strPath.size() != 2) // true for C:, E:, F:, G:
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
      if (bScanSubDirs && !bFound && pItem->GetLabel() != "..")
      {
        // load subfolder
        CStdString strDir = m_Directory.m_strPath;
        if (pItem->m_strPath != "E:\\UDATA" && pItem->m_strPath != "E:\\TDATA")
        {
          m_Directory.m_strPath = pItem->m_strPath;
          CFileItemList subDirItems;
          CHDDirectory rootDir;
          rootDir.SetMask(".xbe");
          rootDir.GetDirectory(pItem->m_strPath, subDirItems);
          OnScan(subDirItems, iTotalAppsFound);
          subDirItems.Clear();
          m_Directory.m_strPath = strDir;
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
  m_Directory.m_strPath = m_strParentPath;
  Update(m_Directory.m_strPath);
}


void CGUIWindowPrograms::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(VIEW_AS_LIST, GetControl(CONTROL_LIST));
  m_viewControl.AddView(VIEW_AS_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.AddView(VIEW_AS_LARGE_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.SetViewControlID(CONTROL_BTNVIEWAS);
}