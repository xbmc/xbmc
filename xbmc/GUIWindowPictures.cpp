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
#include "Application.h"
#include "GUIPassword.h"
#include "GUIDialogMediaSource.h"
#include "GUIDialogPictureInfo.h"
#include "PlayListFactory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "PictureInfoLoader.h"

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
        m_vecItems.m_strPath = strDestination = g_settings.m_defaultPictureSource;
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

          bool bIsSourceName = false;

          SetupShares();
          VECSHARES shares;
          m_rootDir.GetShares(shares);
          int iIndex = CUtil::GetMatchingShare(strDestination, shares, bIsSourceName);
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
              if (bIsSourceName)
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
        if (iAction == ACTION_DELETE_ITEM)
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
        else if (iAction == ACTION_SHOW_INFO)
        {
          OnInfo(iItem);
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

  if (items.GetFolderCount()==items.Size() || !g_guiSettings.GetBool("pictures.usetags"))
    return;

  // Start the music info loader thread
  CPictureInfoLoader loader;
  loader.SetProgressCallback(m_dlgProgress);
  loader.Load(items);

  bool bShowProgress=!m_gWindowManager.HasModalDialog();
  bool bProgressVisible=false;

  DWORD dwTick=timeGetTime();

  while (loader.IsLoading())
  {
    if (bShowProgress)
    { // Do we have to init a progress dialog?
      DWORD dwElapsed=timeGetTime()-dwTick;

      if (!bProgressVisible && dwElapsed>1500 && m_dlgProgress)
      { // tag loading takes more then 1.5 secs, show a progress dialog
        CURL url(items.m_strPath);
        CStdString strStrippedPath;
        url.GetURLWithoutUserDetails(strStrippedPath);
        m_dlgProgress->SetHeading(189);
        m_dlgProgress->SetLine(0, 505);
        m_dlgProgress->SetLine(1, "");
        m_dlgProgress->SetLine(2, strStrippedPath );
        m_dlgProgress->StartModal();
        m_dlgProgress->ShowProgressBar(true);
        bProgressVisible = true;
      }

      if (bProgressVisible && m_dlgProgress)
      { // keep GUI alive
        m_dlgProgress->Progress();
      }
    } // if (bShowProgress)
    Sleep(1);
  } // while (loader.IsLoading())

  if (bProgressVisible && m_dlgProgress)
    m_dlgProgress->Close();
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

  if (pItem->IsCBZ() || pItem->IsCBR())
  {
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
      pSlideShow->Add(pItem);
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
      pSlideShow->Add(pItem);
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
      pSlideShow->Add(pItem);
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
      pSlideShow->Add(pItem);
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

void CGUIWindowPictures::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItem *item = NULL;
  if (itemNumber >= 0 && itemNumber < m_vecItems.Size())
    item = m_vecItems[itemNumber];

  if ( m_vecItems.IsVirtualDirectoryRoot() && item)
  {
    // get the usual shares
    CShare *share = CGUIDialogContextMenu::GetShare("pictures", item);
    CGUIDialogContextMenu::GetContextButtons("pictures", share, buttons);
  }
  else
  {
    if (item)
    {
      if (m_vecItems.GetFileCount() != 0)
        buttons.Add(CONTEXT_BUTTON_VIEW_SLIDESHOW, 13317);      // View Slideshow

      buttons.Add(CONTEXT_BUTTON_RECURSIVE_SLIDESHOW, 13318);     // Recursive Slideshow
      if (!(item->m_bIsFolder || item->IsZIP() || item->IsRAR() || item->IsCBZ() || item->IsCBR()))
        buttons.Add(CONTEXT_BUTTON_INFO, 13406); // picture info

      if (!m_thumbLoader.IsLoading())
        buttons.Add(CONTEXT_BUTTON_REFRESH_THUMBS, 13315);         // Create Thumbnails
      if (g_guiSettings.GetBool("filelists.allowfiledeletion") && !item->IsReadOnly())
      {
        buttons.Add(CONTEXT_BUTTON_DELETE, 117);
        buttons.Add(CONTEXT_BUTTON_RENAME, 118);
      }
    }
    buttons.Add(CONTEXT_BUTTON_GOTO_ROOT, 20128);
    buttons.Add(CONTEXT_BUTTON_SWITCH_MEDIA, 523);
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
  buttons.Add(CONTEXT_BUTTON_SETTINGS, 5);                  // Settings
}

bool CGUIWindowPictures::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItem *item = (itemNumber >= 0 && itemNumber < m_vecItems.Size()) ? m_vecItems[itemNumber] : NULL;
  if (m_vecItems.IsVirtualDirectoryRoot() && item)
  {
    CShare *share = CGUIDialogContextMenu::GetShare("pictures", item);
    if (CGUIDialogContextMenu::OnContextButton("pictures", share, button))
    {
      Update("");
      return true;
    }
  }
  switch (button)
  {
  case CONTEXT_BUTTON_VIEW_SLIDESHOW:
    OnSlideShow(item->m_strPath);
    return true;
  case CONTEXT_BUTTON_RECURSIVE_SLIDESHOW:
    OnSlideShowRecursive(item->m_strPath);
    return true;
  case CONTEXT_BUTTON_INFO:
    OnInfo(itemNumber);
    return true;
  case CONTEXT_BUTTON_REFRESH_THUMBS:
    OnRegenerateThumbs();
    return true;
  case CONTEXT_BUTTON_DELETE:
    OnDeleteItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_RENAME:
    OnRenameItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_SETTINGS:
    m_gWindowManager.ActivateWindow(WINDOW_SETTINGS_MYPICTURES);
    return true;
  case CONTEXT_BUTTON_GOTO_ROOT:
    Update("");
    return true;
  case CONTEXT_BUTTON_SWITCH_MEDIA:
    CGUIDialogContextMenu::SwitchMedia("pictures", m_vecItems.m_strPath);
    return true;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
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
        pSlideShow->Add(pItem);
    }

    // start slideshow if there are items
    pSlideShow->StartSlideShow();
    if (pSlideShow->NumSlides())
      m_gWindowManager.ActivateWindow(WINDOW_SLIDESHOW);
  }
}

void CGUIWindowPictures::OnInfo(int itemNumber)
{
  CFileItem *item = (itemNumber >= 0 && itemNumber < m_vecItems.Size()) ? m_vecItems[itemNumber] : NULL;
  if (!item || item->m_bIsFolder || item->IsZIP() || item->IsRAR() || item->IsCBZ() || item->IsCBR())
    return;
  CGUIDialogPictureInfo *pictureInfo = (CGUIDialogPictureInfo *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PICTURE_INFO);
  if (pictureInfo)
  {
    pictureInfo->SetPicture(item);
    pictureInfo->DoModal();
  }
}

