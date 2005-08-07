
#include "stdafx.h"
#include "GUIWindowPictures.h"
#include "Util.h"
#include "Picture.h"
#include "application.h"
#include "GUIThumbnailPanel.h"
#include "AutoSwitch.h"
#include "GUIPassword.h"
#include "FileSystem/ZipManager.h"
#include "GUIDialogContextMenu.h"
#include "GUIWindowFileManager.h"

#define CONTROL_BTNVIEWASICONS  2
#define CONTROL_BTNSORTBY     3
#define CONTROL_BTNSORTASC    4

#define CONTROL_BTNSLIDESHOW   6
#define CONTROL_BTNSLIDESHOW_RECURSIVE   7

#define CONTROL_SHUFFLE      9
#define CONTROL_LIST       10
#define CONTROL_THUMBS      11
#define CONTROL_LABELFILES         12

struct SSortPicturesByName
{
  static bool Sort(CFileItem* pStart, CFileItem* pEnd)
  {
    CFileItem& rpStart = *pStart;
    CFileItem& rpEnd = *pEnd;
    if (rpStart.GetLabel() == "..") return true;
    if (rpEnd.GetLabel() == "..") return false;
    bool bGreater = true;
    if (m_bSortAscending) bGreater = false;
    if ( rpStart.m_bIsFolder == rpEnd.m_bIsFolder)
    {
      char szfilename1[1024];
      char szfilename2[1024];

      switch ( m_iSortMethod )
      {
      case 0:  // Sort by Filename
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
      case 1:  // Sort by Date
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

      case 3:  // Sort by share type
        if ( rpStart.m_iDriveType > rpEnd.m_iDriveType) return bGreater;
        if ( rpStart.m_iDriveType < rpEnd.m_iDriveType) return !bGreater;
        strcpy(szfilename1, rpStart.GetLabel());
        strcpy(szfilename2, rpEnd.GetLabel());
        break;

      default:  // Sort by Filename by default
        strcpy(szfilename1, rpStart.GetLabel().c_str());
        strcpy(szfilename2, rpEnd.GetLabel().c_str());
        break;
      }


      for (int i = 0; i < (int)strlen(szfilename1); i++)
        szfilename1[i] = tolower((unsigned char)szfilename1[i]);

      for (i = 0; i < (int)strlen(szfilename2); i++)
        szfilename2[i] = tolower((unsigned char)szfilename2[i]);
      //return (rpStart.strPath.compare( rpEnd.strPath )<0);

      if (m_bSortAscending)
        return (strcmp(szfilename1, szfilename2) < 0);
      else
        return (strcmp(szfilename1, szfilename2) >= 0);
    }
    if (!rpStart.m_bIsFolder) return false;
    return true;
  }
  static bool m_bSortAscending;
  static int m_iSortMethod;
};
bool SSortPicturesByName::m_bSortAscending;
int SSortPicturesByName::m_iSortMethod;

CGUIWindowPictures::CGUIWindowPictures(void)
    : CGUIWindow(WINDOW_PICTURES, "MyPics.xml")
{
  m_Directory.m_strPath = "?";
  m_Directory.m_bIsFolder = true;
  m_iItemSelected = -1;
  m_iLastControl = -1;

  m_iViewAsIcons = -1;
  m_iViewAsIconsRoot = -1;
  m_thumbLoader.SetObserver(this);
}

CGUIWindowPictures::~CGUIWindowPictures(void)
{}

bool CGUIWindowPictures::OnAction(const CAction &action)
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

bool CGUIWindowPictures::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_DVDDRIVE_EJECTED_CD:
    {
      if ( !m_Directory.IsVirtualDirectoryRoot() )
      {
        if ( m_Directory.IsCDDA() || m_Directory.IsDVD() || m_Directory.IsISO9660() )
        {
          // Disc has changed and we are inside a DVD Drive share, get out of here :)
          m_Directory.m_strPath.Empty();
          Update( m_Directory.m_strPath );
        }
      }
      else
      {
        int iItem = m_viewControl.GetSelectedItem();
        Update( m_Directory.m_strPath );
        m_viewControl.SetSelectedItem(iItem);
      }
    }
    break;
  case GUI_MSG_DVDDRIVE_CHANGED_CD:
    {
      if ( m_Directory.IsVirtualDirectoryRoot() )
      {
        int iItem = m_viewControl.GetSelectedItem();
        Update( m_Directory.m_strPath );
        m_viewControl.SetSelectedItem(iItem);
      }
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();

      m_iLastControl = GetFocusedControl();
      m_iItemSelected = m_viewControl.GetSelectedItem();

      ClearFileItems();
      if (message.GetParam1() != WINDOW_SLIDESHOW)
      {
        CSectionLoader::UnloadDLL(IMAGE_DLL);
      }
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      int iLastControl = m_iLastControl;
      CGUIWindow::OnMessage(message);
      if (message.GetParam1() != WINDOW_SLIDESHOW)
      {
        CSectionLoader::LoadDLL(IMAGE_DLL);
      }

      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
            
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
        m_Directory.m_strPath = strDestination = g_stSettings.m_szDefaultPictures;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }
      
      if (m_rootDir.GetNumberOfShares() == 0)
      {
        m_rootDir.SetMask(g_stSettings.m_szMyPicturesExtensions);
        m_rootDir.SetShares(g_settings.m_vecMyPictureShares);
      }

      for (unsigned int i=g_settings.m_vecMyPictureShares.size();i<m_rootDir.GetNumberOfShares();++i)
      {
        const CShare& share = m_rootDir[i];
        if (share.strEntryPoint.IsEmpty()) // do not unmount 'normal' rars/zips
          continue;
        
        CURL url(share.strPath);
        if (url.GetProtocol() == "zip://")
          g_ZipManager.release(share.strPath);
        
        strDestination = share.strEntryPoint;
        m_rootDir.RemoveShare(share.strPath);
      }
      
      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // default parameters if the jump fails
        m_Directory.m_strPath = "";

        bool bIsBookmarkName = false;
        int iIndex = CUtil::GetMatchingShare(strDestination, g_settings.m_vecMyPictureShares, bIsBookmarkName);
        if (iIndex > -1)
        {
          // set current directory to matching share
          if (bIsBookmarkName)
            m_Directory.m_strPath = g_settings.m_vecMyPictureShares[iIndex].strPath;
          else
            m_Directory.m_strPath = strDestination;
          CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
        }
        else
        {
          CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
        }
        SetHistoryForPath(m_Directory.m_strPath);
      }

      if (m_iViewAsIcons == -1 && m_iViewAsIconsRoot == -1)
      {
        m_iViewAsIcons = g_stSettings.m_iMyPicturesViewAsIcons;
        m_iViewAsIconsRoot = g_stSettings.m_iMyPicturesRootViewAsIcons;
      }

      Update(m_Directory.m_strPath);

      if (iLastControl > -1)
      {
        SET_CONTROL_FOCUS(iLastControl, 0);
      }
      else
      {
        SET_CONTROL_FOCUS(m_dwDefaultFocusControlID, 0);
      }

      if (m_iItemSelected >= 0)
      {
        m_viewControl.SetSelectedItem(m_iItemSelected);
      }

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNVIEWASICONS)
      {
        // cycle LIST->ICONS->LARGEICONS
        if (m_Directory.IsVirtualDirectoryRoot())
          m_iViewAsIconsRoot++;
        else
          m_iViewAsIcons++;
        if (m_iViewAsIconsRoot > VIEW_AS_LARGE_ICONS) m_iViewAsIconsRoot = VIEW_AS_LIST;
        if (m_iViewAsIcons > VIEW_AS_LARGE_ICONS) m_iViewAsIcons = VIEW_AS_LIST;
        g_stSettings.m_iMyPicturesRootViewAsIcons = m_iViewAsIconsRoot;
        g_stSettings.m_iMyPicturesViewAsIcons = m_iViewAsIcons;
        g_settings.Save();
        UpdateButtons();
      }
      else if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        if (m_Directory.IsVirtualDirectoryRoot())
        {
          if (g_stSettings.m_iMyPicturesRootSortMethod == 0)
            g_stSettings.m_iMyPicturesRootSortMethod = 3;
          else
            g_stSettings.m_iMyPicturesRootSortMethod = 0;
        }
        else
        {
          g_stSettings.m_iMyPicturesSortMethod++;
          if (g_stSettings.m_iMyPicturesSortMethod >= 3) g_stSettings.m_iMyPicturesSortMethod = 0;
        }

        g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        if (m_Directory.IsVirtualDirectoryRoot())
          g_stSettings.m_bMyPicturesRootSortAscending = !g_stSettings.m_bMyPicturesRootSortAscending;
        else
          g_stSettings.m_bMyPicturesSortAscending = !g_stSettings.m_bMyPicturesSortAscending;

        g_settings.Save();
        UpdateButtons();
        OnSort();
      }
      else if (iControl == CONTROL_BTNSLIDESHOW) // Slide Show
      {
        OnSlideShow();
      }
      else if (iControl == CONTROL_BTNSLIDESHOW_RECURSIVE) // Recursive Slide Show
      {
        OnSlideShowRecursive();
      }
      else if (iControl == CONTROL_SHUFFLE)
      {
        g_guiSettings.ToggleBool("Slideshow.Shuffle");
        g_settings.Save();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnClick(iItem);
        }
        else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnPopupMenu(iItem);
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          if (g_guiSettings.GetBool("Pictures.AllowFileDeletion"))
            OnDeleteItem(iItem);

          else
            return false;
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

void CGUIWindowPictures::OnSort()
{
  for (int i = 0; i < (int)m_vecItems.Size(); i++)
  {
    CFileItem* pItem = m_vecItems[i];
    if (g_stSettings.m_iMyPicturesSortMethod == 0 || g_stSettings.m_iMyPicturesSortMethod == 2)
    {
      if (pItem->m_bIsFolder)
        pItem->SetLabel2("");
      else
        pItem->SetFileSizeLabel();
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

  m_viewControl.SetItems(m_vecItems);
}

void CGUIWindowPictures::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems.Clear(); // will clean up everything
}

void CGUIWindowPictures::UpdateButtons()
{
  if (m_Directory.IsVirtualDirectoryRoot())
    m_viewControl.SetCurrentView(m_iViewAsIconsRoot);
  else
    m_viewControl.SetCurrentView(m_iViewAsIcons);

  if (g_guiSettings.GetBool("Slideshow.Shuffle"))
  {
    CGUIMessage msg2(GUI_MSG_SELECTED, GetID(), CONTROL_SHUFFLE, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg2);
  }
  else
  {
    CGUIMessage msg2(GUI_MSG_DESELECTED, GetID(), CONTROL_SHUFFLE, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg2);
  }

  // Update sort by button
  SET_CONTROL_LABEL(CONTROL_BTNSORTBY, SortMethod());

  // Update sorting control
  if (SortAscending())
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
    if (pItem->GetLabel() == "..") iItems--;
  }
  WCHAR wszText[20];
  const WCHAR* szText = g_localizeStrings.Get(127).c_str();
  swprintf(wszText, L"%i %s", iItems, szText);


  SET_CONTROL_LABEL(CONTROL_LABELFILES, wszText);

  // check we can slideshow or recursive slideshow
  int nFolders = m_vecItems.GetFolderCount();
  if (nFolders == m_vecItems.Size())
  {
    CONTROL_DISABLE(CONTROL_BTNSLIDESHOW);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNSLIDESHOW);
  }
  if (m_vecItems.Size() == 0)
  {
    CONTROL_DISABLE(CONTROL_BTNSLIDESHOW_RECURSIVE);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNSLIDESHOW_RECURSIVE);
  }
}

void CGUIWindowPictures::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  UpdateDir(strDirectory);
  if (!m_Directory.IsVirtualDirectoryRoot() && g_guiSettings.GetBool("Pictures.UseAutoSwitching"))
  {
    m_iViewAsIcons = CAutoSwitch::GetView(m_vecItems);

    UpdateButtons();
  }

  m_thumbLoader.Load(m_vecItems);
}

void CGUIWindowPictures::UpdateDir(const CStdString &strDirectory)
{
  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < (int)m_vecItems.Size())
  {
      CFileItem* pItem = m_vecItems[iItem];
    if (pItem->GetLabel() != "..")
    {
      GetDirectoryHistoryString(pItem, strSelectedItem);
      m_history.Set(strSelectedItem, m_Directory.m_strPath);
    }
  }
  ClearFileItems();

  GetDirectory(strDirectory, m_vecItems);

  m_Directory.m_strPath = strDirectory;
//  m_vecItems.SetThumbs();
  if (g_guiSettings.GetBool("FileLists.HideExtensions"))
    m_vecItems.RemoveExtensions();
  m_vecItems.FillInDefaultIcons();
  OnSort();
  UpdateButtons();

  strSelectedItem = m_history.Get(m_Directory.m_strPath);

  for (int i = 0; i < (int)m_vecItems.Size(); ++i)
  {
    CFileItem* pItem = m_vecItems[i];
    CStdString strHistory;
    GetDirectoryHistoryString(pItem, strHistory);
    if (strHistory == strSelectedItem)
    {
      m_viewControl.SetSelectedItem(i);
      break;
    }
  }

}

void CGUIWindowPictures::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return ;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  if (pItem->m_bIsFolder)
  {
    if ( !g_passwordManager.IsItemUnlocked( pItem, "pictures" ) )
      return ;

    m_iItemSelected = -1;
    if ( pItem->m_bIsShareOrDrive )
    {
      if ( !HaveDiscOrConnection( pItem->m_strPath, pItem->m_iDriveType ) )
        return ;
    }
    Update(strPath);
  }
  else if (pItem->IsZIP() && g_guiSettings.GetBool("Pictures.HandleArchives")) // mount zip archive
  {
    CShare shareZip;
    shareZip.strPath.Format("zip://Z:\\,%i,,%s,\\",1, pItem->m_strPath.c_str() );
    shareZip.strEntryPoint.Empty();
    m_rootDir.AddShare(shareZip);
    m_iItemSelected = -1;
    Update(shareZip.strPath);
  }
  else if (pItem->IsRAR() && g_guiSettings.GetBool("Pictures.HandleArchives")) // mount rar archive
  {
    CShare shareRar;
    shareRar.strPath.Format("rar://Z:\\,%i,,%s,\\",EXFILE_AUTODELETE, pItem->m_strPath.c_str() );
    shareRar.strEntryPoint.Empty();
    m_rootDir.AddShare(shareRar);
    m_iItemSelected = -1;
    Update(shareRar.strPath);
  }
  else if (pItem->IsCBZ()) //mount'n'show'n'unmount
  {
    CUtil::GetDirectory(pItem->m_strPath,strPath);
    CShare shareZip;
    shareZip.strPath.Format("zip://Z:\\filesrar\\,%i,,%s,\\",1, pItem->m_strPath.c_str() );
    shareZip.strEntryPoint = strPath;
    m_rootDir.AddShare(shareZip);
    Update(shareZip.strPath);
    if (m_vecItems.Size() > 0)
    {
      CStdString strEmpty; strEmpty.Empty();
      OnShowPictureRecursive(strEmpty);
    }
    else
    {
      CLog::Log(LOGERROR,"No pictures found in cbz file!");
      m_rootDir.RemoveShare(shareZip.strPath);
      Update(strPath);
    }
    m_iItemSelected = iItem;
  }
  else if (pItem->IsCBR()) // mount'n'show'n'unmount
  {
    CUtil::GetDirectory(pItem->m_strPath,strPath);
    CShare shareRar;
    shareRar.strPath.Format("rar://Z:\\,%i,,%s,\\",EXFILE_AUTODELETE,pItem->m_strPath.c_str() );
    shareRar.strEntryPoint = strPath;
    m_rootDir.AddShare(shareRar);
    m_iItemSelected = -1;
    Update(shareRar.strPath);
    if (m_vecItems.Size() > 0)
    {
      CStdString strEmpty; strEmpty.Empty();
      OnShowPictureRecursive(strEmpty);
    }
    else
    {
      CLog::Log(LOGERROR,"No pictures found in cbr file!");
      m_rootDir.RemoveShare(shareRar.strPath);
      Update(strPath);
    }
    m_iItemSelected = iItem;
  }
  else
  {
    // show picture
    m_iItemSelected = m_viewControl.GetSelectedItem();
    OnShowPicture(strPath);
  }
}

bool CGUIWindowPictures::HaveDiscOrConnection( CStdString& strPath, int iDriveType )
{
  if ( iDriveType == SHARE_TYPE_DVD )
  {
    CDetectDVDMedia::WaitMediaReady();
    if ( !CDetectDVDMedia::IsDiscInDrive() )
    {
      CGUIDialogOK::ShowAndGetInput(218, 219, 0, 0);
      int iItem = m_viewControl.GetSelectedItem();
      Update( m_Directory.m_strPath );
      m_viewControl.SetSelectedItem(iItem);
      return false;
    }
  }
  else if ( iDriveType == SHARE_TYPE_REMOTE )
  {
    // TODO: Handle not connected to a remote share
    if ( !CUtil::IsEthernetConnected() )
    {
      CGUIDialogOK::ShowAndGetInput(220, 221, 0, 0);
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
    return ;
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  for (int i = 0; i < (int)m_vecItems.Size();++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (!pItem->m_bIsFolder)
    {
      pSlideShow->Add(pItem->m_strPath);
    }
  }
  pSlideShow->Select(strPicture);
  m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowPictures::OnShowPictureRecursive(const CStdString& strPicture)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return ;
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  for (int i = 0; i < (int)m_vecItems.Size();++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (!pItem->m_bIsFolder)
    {
      pSlideShow->Add(pItem->m_strPath);
    }
    else
    {
      AddDir(pSlideShow,pItem->m_strPath);
    }
  }
  if (!strPicture.IsEmpty())
    pSlideShow->Select(strPicture);
  m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowPictures::AddDir(CGUIWindowSlideShow *pSlideShow, const CStdString& strPath)
{
  if (!pSlideShow) return ;
  CFileItemList items;
  m_rootDir.GetDirectory(strPath, items);
  SortItems(items);

  for (int i = 0; i < (int)items.Size();++i)
  {
    CFileItem* pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      pSlideShow->Add(pItem->m_strPath);
    }
    else
    {
      AddDir(pSlideShow, pItem->m_strPath);
    }
  }
}

void CGUIWindowPictures::OnSlideShowRecursive(const CStdString &strPicture)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return ;

  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  AddDir(pSlideShow, m_Directory.m_strPath);
  pSlideShow->StartSlideShow();
  if (!strPicture.IsEmpty())
    pSlideShow->Select(strPicture);
  if (pSlideShow->NumSlides())
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
    return ;
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  for (int i = 0; i < (int)m_vecItems.Size();++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (!pItem->m_bIsFolder)
    {
      pSlideShow->Add(pItem->m_strPath);
    }
  }
  pSlideShow->StartSlideShow();
  if (!strPicture.IsEmpty())
    pSlideShow->Select(strPicture);
  if (pSlideShow->NumSlides())
    m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowPictures::OnRegenerateThumbs()
{
  if (m_thumbLoader.IsLoading()) return;
  m_thumbLoader.SetRegenerateThumbs(true);
  m_thumbLoader.Load(m_vecItems);
}

void CGUIWindowPictures::Render()
{
  CGUIWindow::Render();
}

void CGUIWindowPictures::GoParentFolder()
{
  CURL url(m_Directory.m_strPath);
  if ((url.GetProtocol() == "rar") || (url.GetProtocol() == "zip")) 
  {
    // check for step-below, if, unmount rar
    if (url.GetFileName().IsEmpty())
    {
      if (url.GetProtocol() == "zip")
        g_ZipManager.release(m_Directory.m_strPath); // release resources
      m_rootDir.RemoveShare(m_Directory.m_strPath);
      CStdString strPath;
      CUtil::GetDirectory(url.GetHostName(),strPath);
      Update(strPath);
      return;
    }
  }
  
  CStdString strPath(m_strParentPath), strOldPath(m_Directory.m_strPath);
  Update(strPath);

  if (!g_guiSettings.GetBool("LookAndFeel.FullDirectoryHistory"))
    m_history.Remove(strOldPath); //Delete current path
}

/// \brief Build a directory history string
/// \param pItem Item to build the history string from
/// \param strHistoryString History string build as return value
void CGUIWindowPictures::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
  if (pItem->m_bIsShareOrDrive)
  {
    // We are in the virual directory

    // History string of the DVD drive
    // must be handel separately
    if (pItem->m_iDriveType == SHARE_TYPE_DVD)
    {
      // Remove disc label from item label
      // and use as history string, m_strPath
      // can change for new discs
      CStdString strLabel = pItem->GetLabel();
      int nPosOpen = strLabel.Find('(');
      int nPosClose = strLabel.ReverseFind(')');
      if (nPosOpen > -1 && nPosClose > -1 && nPosClose > nPosOpen)
      {
        strLabel.Delete(nPosOpen + 1, (nPosClose) - (nPosOpen + 1));
        strHistoryString = strLabel;
      }
      else
        strHistoryString = strLabel;
    }
    else
    {
      // Other items in virual directory
      CStdString strPath = pItem->m_strPath;
      while (CUtil::HasSlashAtEnd(strPath))
        strPath.Delete(strPath.size() - 1);

      strHistoryString = pItem->GetLabel() + strPath;
    }
  }
  else
  {
    // Normal directory items
    strHistoryString = pItem->m_strPath;

    if (CUtil::HasSlashAtEnd(strHistoryString))
      strHistoryString.Delete(strHistoryString.size() - 1);
  }
}

void CGUIWindowPictures::SetHistoryForPath(const CStdString& strDirectory)
{
  if (!strDirectory.IsEmpty())
  {
    // Build the directory history for default path
    CStdString strPath, strParentPath;
    strPath = strDirectory;
    CFileItemList items;
    GetDirectory("", items);

    while (CUtil::GetParentPath(strPath, strParentPath))
    {
      bool bSet = false;
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItem* pItem = items[i];
        while (CUtil::HasSlashAtEnd(pItem->m_strPath))
          pItem->m_strPath.Delete(pItem->m_strPath.size() - 1);
        if (pItem->m_strPath == strPath)
        {
          CStdString strHistory;
          GetDirectoryHistoryString(pItem, strHistory);
          m_history.Set(strHistory, "");
          return ;
        }
      }

      m_history.Set(strPath, strParentPath);
      strPath = strParentPath;
      while (CUtil::HasSlashAtEnd(strPath))
        strPath.Delete(strPath.size() - 1);
    }
    items.Clear();
  }
}

void CGUIWindowPictures::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (items.Size())
  {
    // cleanup items
    items.Clear();
  }

  CStdString strParentPath;
  bool bParentExists = CUtil::GetParentPath(strDirectory, strParentPath);

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
        pItem->m_strPath = strParentPath;
        pItem->m_bIsFolder = true;
        pItem->m_bIsShareOrDrive = false;
        items.Add(pItem);
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
      pItem->m_strPath = "";
      pItem->m_bIsShareOrDrive = false;
      pItem->m_bIsFolder = true;
      items.Add(pItem);
    }
    m_strParentPath = "";
  }
  m_rootDir.GetDirectory(strDirectory, items);

}

void CGUIWindowPictures::OnPopupMenu(int iItem)
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
    { // we should add the option here of adding shares
      return ;
    }
    // mark the item
    m_vecItems[iItem]->Select(true);
    bool bMaxRetryExceeded = false;
    if (g_stSettings.m_iMasterLockMaxRetry != 0)
      bMaxRetryExceeded = !(m_vecItems[iItem]->m_iBadPwdCount < g_stSettings.m_iMasterLockMaxRetry);

    // and do the popup menu
    if (CGUIDialogContextMenu::BookmarksMenu("pictures", m_vecItems[iItem]->GetLabel(), m_vecItems[iItem]->m_strPath, m_vecItems[iItem]->m_iLockMode, bMaxRetryExceeded, iPosX, iPosY))
    {
      m_rootDir.SetShares(g_settings.m_vecMyPictureShares);
      Update(m_Directory.m_strPath);
      return ;
    }
  }
  else if (iItem >= 0)
  {
    // mark the item
    m_vecItems[iItem]->Select(true);
    // popup the context menu
    CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
    if (!pMenu) return ;
    // load our menu
    pMenu->Initialize();
    // add the needed buttons
    int btn_SlideShow = pMenu->AddButton(13317);    // View Slideshow
    int btn_RecSlideShow = pMenu->AddButton(13318); // Recursive Slideshow
    int btn_Thumbs = pMenu->AddButton(13315);       // Create Thumbnails

    // this could be done like the delete button too
    if (m_vecItems.GetFileCount() == 0)
      pMenu->EnableButton(btn_SlideShow, false);
    if (m_thumbLoader.IsLoading())
      pMenu->EnableButton(btn_Thumbs, false);

    int btn_Delete = 0;
    if (g_guiSettings.GetBool("Pictures.AllowFileDeletion"))
      btn_Delete = pMenu->AddButton(117);           // Delete
    
    int btn_Settings = pMenu->AddButton(5);         // Settings

    // position it correctly
    pMenu->SetPosition(iPosX - pMenu->GetWidth() / 2, iPosY - pMenu->GetHeight() / 2);
    pMenu->DoModal(GetID());

    int btnid = pMenu->GetButton();

    if (btnid>0)
    {
      if (btnid == btn_SlideShow)
      {
        OnSlideShow(m_vecItems[iItem]->m_strPath);
        return;
      }
      else if (btnid == btn_RecSlideShow)
      {
        OnSlideShowRecursive(m_vecItems[iItem]->m_strPath);
        return;
      }
      else if (btnid == btn_Thumbs)
      {
        OnRegenerateThumbs();
      }
      else if (btnid == btn_Delete)
      {
        OnDeleteItem(iItem);
      }
      else if (btnid == btn_Settings)
      {
        m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPICTURES);
        return;
      }
    }
    m_vecItems[iItem]->Select(false);
  }
}

int CGUIWindowPictures::SortMethod()
{
  if (m_Directory.IsVirtualDirectoryRoot())
  {
    if (g_stSettings.m_iMyPicturesRootSortMethod == 0)
      return g_stSettings.m_iMyPicturesRootSortMethod + 103;
    else
      return 498; // Sort by: Type
  }
  else
    return g_stSettings.m_iMyPicturesSortMethod + 103;
}

bool CGUIWindowPictures::SortAscending()
{
  if (m_Directory.IsVirtualDirectoryRoot())
    return g_stSettings.m_bMyPicturesRootSortAscending;
  else
    return g_stSettings.m_bMyPicturesSortAscending;
}

void CGUIWindowPictures::SortItems(CFileItemList& items)
{
  if (m_Directory.IsVirtualDirectoryRoot())
  {
    SSortPicturesByName::m_iSortMethod = g_stSettings.m_iMyPicturesRootSortMethod;
    SSortPicturesByName::m_bSortAscending = g_stSettings.m_bMyPicturesRootSortAscending;
  }
  else
  {
    SSortPicturesByName::m_iSortMethod = g_stSettings.m_iMyPicturesSortMethod;
    if (g_stSettings.m_iMyPicturesSortMethod == 1 || g_stSettings.m_iMyPicturesSortMethod == 2)
      SSortPicturesByName::m_bSortAscending = !g_stSettings.m_bMyPicturesSortAscending;
    else
      SSortPicturesByName::m_bSortAscending = g_stSettings.m_bMyPicturesSortAscending;
  }
  items.Sort(SSortPicturesByName::Sort);
}

void CGUIWindowPictures::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(VIEW_AS_LIST, GetControl(CONTROL_LIST));
  m_viewControl.AddView(VIEW_AS_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.AddView(VIEW_AS_LARGE_ICONS, GetControl(CONTROL_THUMBS));
  m_viewControl.SetViewControlID(CONTROL_BTNVIEWASICONS);
}

void CGUIWindowPictures::OnWindowUnload()
{
  CGUIWindow::OnWindowUnload();
  m_viewControl.Reset();
}

void CGUIWindowPictures::OnItemLoaded(CFileItem *pItem)
{
  if (pItem->m_bIsFolder && !pItem->m_bIsShareOrDrive && !pItem->HasThumbnail() && pItem->GetLabel() != "..")
  { // generate the thumb folder if necessary
    // we load the directory, grab 4 random thumb files (if available) and then generate
    // the thumb.
    if (pItem->IsRemote() && !pItem->IsDVD() && !g_guiSettings.GetBool("VideoFiles.FindRemoteThumbs")) return;
    CFileItemList items;
    m_rootDir.GetDirectory(pItem->m_strPath, items);
    // create the folder thumb by choosing 4 random thumbs within the folder and putting
    // them into one thumb.
    // count the number of images
    int numFiles = 0;
    for (int i=0; i < items.Size(); i++)
      if (items[i]->IsPicture() && !items[i]->IsZIP() && !items[i]->IsRAR())
        numFiles++;
    if (!numFiles) return;

    srand(timeGetTime());
    int thumbs[4];
    if (numFiles > 4)
    { // choose 4 random thumbs
      int i = 0;
      while (i < 4)
      {
        int thumbnum = rand() % numFiles;
        bool bFoundNew = true;
        for (int j = 0; j < i; j++)
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
      for (int i = 0; i < numFiles; i++)
        thumbs[i] = i;
      for (int i = numFiles; i < 4; i++)
        thumbs[i] = -1;
    }
    // ok, now we've got the files to get the thumbs from, lets create it...
    // we basically load the 4 thumbs, resample to 62x62 pixels, and add them
    CStdString strFiles[4];
    for (int thumb = 0; thumb < 4; thumb++)
    {
      if (thumbs[thumb] >= 0)
      {
        int files = 0;
        for (int i = 0; i < items.Size(); i++)
        {
          if (items[i]->IsPicture() && !items[i]->IsZIP() && !items[i]->IsRAR())
          {
            if (thumbs[thumb] == files)
              strFiles[thumb] = items[i]->m_strPath;
            files++;
          }
        }
      }
      else
        strFiles[thumb] = "";
    }
    CPicture pic;
    pic.CreateFolderThumb(pItem->m_strPath, strFiles);
    // refill in the icon to get it to update
    g_graphicsContext.Lock();
    pItem->FreeIcons();
    pItem->SetThumb();
    pItem->FillInDefaultIcon();
    g_graphicsContext.Unlock();
  }
}

void CGUIWindowPictures::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems.Size()) return;
  const CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;
  CStdString strFile = CUtil::GetFileName(strPath);
  if (pItem->m_bIsFolder)
    CUtil::GetDirectoryName(strPath, strFile);

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (pDialog)
  {
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 125);
    pDialog->SetLine(1, strFile.c_str());
    pDialog->SetLine(2, L"");
    pDialog->DoModal(GetID());
    if (!pDialog->IsConfirmed()) return ;
  }

  CGUIWindowFileManager* pWindow = (CGUIWindowFileManager*)m_gWindowManager.GetWindow(WINDOW_FILES);
  if (pWindow) pWindow->Delete(pItem);

  Update(m_Directory.m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}
