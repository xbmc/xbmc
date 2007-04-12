/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowPictures.h"
#include "Util.h"
#include "Picture.h"
#include "application.h"
#include "GUIPassword.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogMediaSource.h"
#include "PlayListFactory.h"
#include "FileSystem/MultiPathDirectory.h"

#define CONTROL_BTNVIEWASICONS      2
#define CONTROL_BTNSORTBY           3
#define CONTROL_BTNSORTASC          4
#define CONTROL_LIST               50
#define CONTROL_THUMBS             51
#define CONTROL_LABELFILES         12

using namespace XFILE;
using namespace DIRECTORY;
using namespace PLAYLIST;

#define CONTROL_BTNSLIDESHOW   6
#define CONTROL_BTNSLIDESHOW_RECURSIVE   7
#define CONTROL_SHUFFLE      9

CGUIWindowPictures::CGUIWindowPictures(void)
    : CGUIMediaWindow(WINDOW_PICTURES, "MyPics.xml")
{
  m_thumbLoader.SetObserver(this);
}

CGUIWindowPictures::~CGUIWindowPictures(void)
{
}

bool CGUIWindowPictures::OnAction(const CAction &action)
{
  // the non-contextual menu can be called at any time
  if (action.wID == ACTION_CONTEXT_MENU && !m_viewControl.HasControl(GetFocusedControlID()))
  {
    OnPopupMenu(-1, false);
    return true;
  }

  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowPictures::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();

      if (message.GetParam1() != WINDOW_SLIDESHOW)
      {
        m_ImageLib.Unload();
      }
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      // check for a passed destination path
      CStdString strDestination = message.GetStringParam();
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to quickpath to: %s", strDestination.c_str());
        m_history.ClearPathHistory();
      }
      // otherwise, is this the first time accessing this window?
      else if (m_vecItems.m_strPath == "?")
      {
        m_vecItems.m_strPath = strDestination = g_stSettings.m_szDefaultPictures;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // open root
        if (strDestination.Equals("$ROOT"))
        {
          m_vecItems.m_strPath = "";
          CLog::Log(LOGINFO, "  Success! Opening root listing.");
        }
        else
        {
          // default parameters if the jump fails
          m_vecItems.m_strPath = "";

          bool bIsBookmarkName = false;

          SetupShares();
          VECSHARES shares;
          m_rootDir.GetShares(shares);
          int iIndex = CUtil::GetMatchingShare(strDestination, shares, bIsBookmarkName);
          if (iIndex > -1)
          {
            bool bDoStuff = true;
            if (shares[iIndex].m_iHasLock == 2)
            {
              CFileItem item(shares[iIndex]);
              if (!g_passwordManager.IsItemUnlocked(&item,"pictures"))
              {
                m_vecItems.m_strPath = ""; // no u don't
                bDoStuff = false;
                CLog::Log(LOGINFO, "  Failure! Failed to unlock destination path: %s", strDestination.c_str());
              }
            }
            // set current directory to matching share
            if (bDoStuff)
            {
              if (bIsBookmarkName)
                m_vecItems.m_strPath=shares[iIndex].strPath;
              else
                m_vecItems.m_strPath=strDestination;
              CUtil::RemoveSlashAtEnd(m_vecItems.m_strPath);
              CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
            }
          }
          else
          {
            CLog::Log(LOGERROR, "  Failed! Destination parameter (%s) does not match a valid share!", strDestination.c_str());
          }
        }

        SetHistoryForPath(m_vecItems.m_strPath);
      }

      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      if (message.GetParam1() != WINDOW_SLIDESHOW)
      {
        m_ImageLib.Load();
      }

      if (!CGUIMediaWindow::OnMessage(message))
        return false;

      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNSLIDESHOW) // Slide Show
      {
        OnSlideShow();
      }
      else if (iControl == CONTROL_BTNSLIDESHOW_RECURSIVE) // Recursive Slide Show
      {
        OnSlideShowRecursive();
      }
      else if (iControl == CONTROL_SHUFFLE)
      {
        g_guiSettings.ToggleBool("slideshow.shuffle");
        g_settings.Save();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnPopupMenu(iItem);
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          if (g_guiSettings.GetBool("filelists.allowfiledeletion"))
            OnDeleteItem(iItem);
          else
            return false;
        }
        else if (iAction == ACTION_PLAYER_PLAY)
        {
          OnPlayMedia(iItem);
          return true;
        }
      }
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowPictures::UpdateButtons()
{
  CGUIMediaWindow::UpdateButtons();

  // Update the shuffle button
  if (g_guiSettings.GetBool("slideshow.shuffle"))
  {
    CGUIMessage msg2(GUI_MSG_SELECTED, GetID(), CONTROL_SHUFFLE, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg2);
  }
  else
  {
    CGUIMessage msg2(GUI_MSG_DESELECTED, GetID(), CONTROL_SHUFFLE, 0, 0, NULL);
    g_graphicsContext.SendMessage(msg2);
  }

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
  if (m_guiState.get() && !m_guiState->HideParentDirItems())
    nFolders--;
  if (m_vecItems.Size() == 0 || nFolders == 0)
  {
    CONTROL_DISABLE(CONTROL_BTNSLIDESHOW_RECURSIVE);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNSLIDESHOW_RECURSIVE);
  }
}

void CGUIWindowPictures::OnPrepareFileItems(CFileItemList& items)
{
  for (int i=0;i<items.Size();++i )
    if (items[i]->GetLabel().Equals("folder.jpg"))
      items.Remove(i);
}

bool CGUIWindowPictures::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  m_vecItems.SetThumbnailImage("");
  m_thumbLoader.Load(m_vecItems);
  m_vecItems.SetCachedPictureThumb();

  return true;
}

bool CGUIWindowPictures::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return true;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPath = pItem->m_strPath;

  if (pItem->IsCBZ() || pItem->IsCBR())
  {
    CUtil::GetDirectory(pItem->m_strPath,strPath);
    CStdString strComicPath;
    if (pItem->IsCBZ())
      CUtil::CreateZipPath(strComicPath, pItem->m_strPath, "");
    else
      CUtil::CreateRarPath(strComicPath, pItem->m_strPath, "");
    CFileItemList vecItems;
    if (CGUIMediaWindow::GetDirectory(strComicPath, vecItems))
    {
      vecItems.Sort(SORT_METHOD_FILE,SORT_ORDER_ASC);
      if (vecItems.Size() > 0)
      {
        OnShowPictureRecursive("",&vecItems);
        return true;
      }
      else
      {
        CLog::Log(LOGERROR,"No pictures found in comic file!");
        return false;
      }
    }
  }
  else if (CGUIMediaWindow::OnClick(iItem))
    return true;

  return false;
}

bool CGUIWindowPictures::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems.Size() ) return false;
  CFileItem* pItem = m_vecItems[iItem];
  CStdString strPicture = pItem->m_strPath;

  if (pItem->m_strPath == "add") // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("pictures"))
    {
      Update("");
      return true;
    }
    return false;
  }

  if (pItem->IsDVD())
    return CAutorun::PlayDisc();

  if (pItem->m_bIsShareOrDrive)
    return false;

  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return false;
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  for (int i = 0; i < (int)m_vecItems.Size();++i)
  {
    CFileItem* pItem = m_vecItems[i];
    if (!pItem->m_bIsFolder && !(CUtil::IsRAR(pItem->m_strPath) || CUtil::IsZIP(pItem->m_strPath)))
    {
      pSlideShow->Add(pItem->m_strPath);
    }
  }
  pSlideShow->Select(strPicture);
  m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);

  return true;
}

void CGUIWindowPictures::OnShowPictureRecursive(const CStdString& strPicture, CFileItemList* pVecItems)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return ;
  if (g_application.IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  if (!pVecItems)
    pVecItems = &m_vecItems;
  CFileItemList& vecItems = *pVecItems;
  for (int i = 0; i < (int)vecItems.Size();++i)
  {
    CFileItem* pItem = vecItems[i];
    if (pItem->IsParentFolder())
      continue;
    if (pItem->m_bIsFolder)
      AddDir(pSlideShow, pItem->m_strPath);
    else if (!(CUtil::IsRAR(pItem->m_strPath) || CUtil::IsZIP(pItem->m_strPath)) && !CUtil::GetFileName(pItem->m_strPath).Equals("folder.jpg"))
      pSlideShow->Add(pItem->m_strPath);
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
  items.Sort(SORT_METHOD_FILE,SORT_ORDER_ASC); // WARNING: might make sense with the media window sort order but currently used only with comics and there it doesn't.

  for (int i = 0; i < (int)items.Size();++i)
  {
    CFileItem* pItem = items[i];
    if (pItem->m_bIsFolder)
      AddDir(pSlideShow, pItem->m_strPath);
    else if (!(CUtil::IsRAR(pItem->m_strPath) || CUtil::IsZIP(pItem->m_strPath)))
      pSlideShow->Add(pItem->m_strPath);
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
  AddDir(pSlideShow, m_vecItems.m_strPath);
  if (g_guiSettings.GetBool("slideshow.shuffle"))
    pSlideShow->Shuffle();
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
    if (!pItem->m_bIsFolder && !(CUtil::IsRAR(pItem->m_strPath) || CUtil::IsZIP(pItem->m_strPath)))
    {
      pSlideShow->Add(pItem->m_strPath);
    }
  }
  if (g_guiSettings.GetBool("slideshow.shuffle"))
    pSlideShow->Shuffle();
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

void CGUIWindowPictures::OnPopupMenu(int iItem, bool bContextDriven /* = true */)
{
  // calculate our position
  float posX = 200, posY = 100;
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList)
  {
    posX = pList->GetXPosition() + pList->GetWidth() / 2;
    posY = pList->GetYPosition() + pList->GetHeight() / 2;
  }
  if ( m_vecItems.IsVirtualDirectoryRoot() )
  {
    if (iItem < 0)
    { // we should add the option here of adding shares
      return ;
    }
    // mark the item
    m_vecItems[iItem]->Select(true);
    // and do the popup menu
    if (CGUIDialogContextMenu::BookmarksMenu("pictures", m_vecItems[iItem], posX, posY))
    {
      Update(m_vecItems.m_strPath);
      return ;
    }
    m_vecItems[iItem]->Select(false);
  }
  else
  {
    if (m_vecItems.Size() == 0)
      bContextDriven = false;
    if (bContextDriven && (iItem < 0 || iItem >= m_vecItems.Size())) return;

    // popup the context menu
    CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)m_gWindowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
    if (!pMenu) return ;

    // load our menu
    pMenu->Initialize();

    // contextual buttons
    int btn_Thumbs = 0;				// Create Thumbnails
    int btn_SlideShow = 0;		// View Slideshow
    int btn_RecSlideShow = 0; // Recursive Slideshow
    int btn_Delete = 0;				// Delete
    int btn_Rename = 0;				// Rename

    // check item boumds
    if (bContextDriven)
    {
      // mark the item
      m_vecItems[iItem]->Select(true);

      // this could be done like the delete button too
      if (m_vecItems.GetFileCount() != 0)
        btn_SlideShow = pMenu->AddButton(13317);      // View Slideshow

      btn_RecSlideShow = pMenu->AddButton(13318);     // Recursive Slideshow

      if (!m_thumbLoader.IsLoading())
        btn_Thumbs = pMenu->AddButton(13315);         // Create Thumbnails

      // add delete/rename functions if supported by the protocol
      if (g_guiSettings.GetBool("filelists.allowfiledeletion") && !m_vecItems[iItem]->IsReadOnly())
      {
        btn_Delete = pMenu->AddButton(117);           // Delete
        btn_Rename = pMenu->AddButton(118);           // Rename
      }
    } // if (iItem >= 0 && iItem < m_vecItems.Size())

    // non-contextual buttons
    int btn_Settings = pMenu->AddButton(5);			// Settings
    int btn_GoToRoot = pMenu->AddButton(20128);	// Go To Root
    int btn_Switch = pMenu->AddButton(523);     // switch media

    // position it correctly
    pMenu->SetPosition(posX - pMenu->GetWidth() / 2, posY - pMenu->GetHeight() / 2);
    pMenu->DoModal();

    int btnid = pMenu->GetButton();
    if (btnid>0)
    {
      // slideshow
      if (btnid == btn_SlideShow)
      {
        OnSlideShow(m_vecItems[iItem]->m_strPath);
        return;
      }
      // recursive slideshow
      else if (btnid == btn_RecSlideShow)
      {
        OnSlideShowRecursive(m_vecItems[iItem]->m_strPath);
        return;
      }
      // create thumbs
      else if (btnid == btn_Thumbs)
      {
        OnRegenerateThumbs();
      }
      // delete
      else if (btnid == btn_Delete)
      {
        OnDeleteItem(iItem);
      }
      //Rename
      else if (btnid == btn_Rename)
      {
        OnRenameItem(iItem);
      }
      else if (btnid == btn_Settings)
      {
        m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPICTURES);
        return;
      }
      // go to root
      else if (btnid == btn_GoToRoot)
      {
        Update("");
        return;
      }
      // switch media
      else if (btnid == btn_Switch)
      {
        CGUIDialogContextMenu::SwitchMedia("pictures", m_vecItems.m_strPath, posX, posY);
        return;
      }
    }
    if (iItem < m_vecItems.Size() && iItem > -1)
      m_vecItems[iItem]->Select(false);
  }
}

void CGUIWindowPictures::OnItemLoaded(CFileItem *pItem)
{
  if (pItem->IsCBR() || pItem->IsCBZ())
  {
    CStdString strTBN;
    CUtil::ReplaceExtension(pItem->m_strPath,".tbn",strTBN);
    if (CFile::Exists(strTBN))
    {
      CPicture pic;
      if (pic.DoCreateThumbnail(strTBN, pItem->GetCachedPictureThumb(),true))
      {
        pItem->SetCachedPictureThumb();
        pItem->FillInDefaultIcon();
        return;
      }
    }
  }
  if ((pItem->m_bIsFolder || pItem->IsCBR() || pItem->IsCBZ()) && !pItem->m_bIsShareOrDrive && !pItem->HasThumbnail() && !pItem->IsParentFolder())
  {
    // first check for a folder.jpg
    CStdString thumb;
    CStdString strPath = pItem->m_strPath;
    if (pItem->IsCBR())
      CUtil::CreateRarPath(strPath,pItem->m_strPath,"");
    if (pItem->IsCBZ())
      CUtil::CreateZipPath(strPath,pItem->m_strPath,"");
    if (pItem->IsMultiPath())
      strPath = CMultiPathDirectory::GetFirstPath(pItem->m_strPath);
    CUtil::AddFileToFolder(strPath, "folder.jpg", thumb);
    if (CFile::Exists(thumb))
    {
      CPicture pic;
      pic.DoCreateThumbnail(thumb, pItem->GetCachedPictureThumb(),true);
    }
    else
    {
      // we load the directory, grab 4 random thumb files (if available) and then generate
      // the thumb.

      CFileItemList items;

      CDirectory::GetDirectory(strPath, items, g_stSettings.m_pictureExtensions, false, false);

      // create the folder thumb by choosing 4 random thumbs within the folder and putting
      // them into one thumb.
      // count the number of images
      for (int i=0; i < items.Size();)
      {
        if (!items[i]->IsPicture() || items[i]->IsZIP() || items[i]->IsRAR() || items[i]->IsPlayList())
        {
          items.Remove(i);
        }
        else
          i++;
      }

      if (items.IsEmpty())
      {
        if (pItem->IsCBZ() || pItem->IsCBR())
        {
          CDirectory::GetDirectory(strPath, items, g_stSettings.m_pictureExtensions, false, false);
          for (int i=0;i<items.Size();++i)
          {
            if (items[i]->m_bIsFolder)
            {
              OnItemLoaded(items[i]);
              pItem->SetThumbnailImage(items[i]->GetThumbnailImage());
              pItem->SetIconImage(items[i]->GetIconImage());
              return;
            }
          }
        }
        return; // no images in this folder
      }

      // randomize them
      items.Randomize();

      if (items.Size() < 4)
      { // less than 4 items, so just grab a single random thumb
        CStdString folderThumb(pItem->GetCachedPictureThumb());
        CPicture pic;
        pic.DoCreateThumbnail(items[0]->m_strPath, folderThumb);
      }
      else
      {
        // ok, now we've got the files to get the thumbs from, lets create it...
        // we basically load the 4 thumbs, resample to 62x62 pixels, and add them
        CStdString strFiles[4];
        for (int thumb = 0; thumb < 4; thumb++)
          strFiles[thumb] = items[thumb]->m_strPath;
        CPicture pic;
        pic.CreateFolderThumb(strFiles, pItem->GetCachedPictureThumb());
      }
    }
    // refill in the icon to get it to update
    pItem->SetCachedPictureThumb();
    pItem->FillInDefaultIcon();
  }
}

void CGUIWindowPictures::LoadPlayList(const CStdString& strPlayList)
{
  CLog::Log(LOGDEBUG,"CGUIWindowPictures::LoadPlayList()... converting playlist into slideshow: %s", strPlayList.c_str());
  auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if ( NULL != pPlayList.get())
  {
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return ; //hmmm unable to load playlist?
    }
  }

  CPlayList playlist = *pPlayList;
  if (playlist.size() > 0)
  {
    // set up slideshow
    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!pSlideShow)
      return;
    if (g_application.IsPlayingVideo())
      g_application.StopPlaying();

    // convert playlist items into slideshow items
    pSlideShow->Reset();
    for (int i = 0; i < (int)playlist.size(); ++i)
    {
      CFileItem *pItem = new CFileItem(playlist[i].m_strPath, false);
      //CLog::Log(LOGDEBUG,"-- playlist item: %s", pItem->m_strPath.c_str());
      if (pItem->IsPicture() && !(pItem->IsZIP() || pItem->IsRAR() || pItem->IsCBZ() || pItem->IsCBR()))
        pSlideShow->Add(pItem->m_strPath);
    }

    // start slideshow if there are items
    pSlideShow->StartSlideShow();
    if (pSlideShow->NumSlides())
      m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
  }
}