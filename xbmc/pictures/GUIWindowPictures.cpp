/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/SystemClock.h"
#include "system.h"
#include "GUIWindowPictures.h"
#include "URL.h"
#include "Util.h"
#include "Application.h"
#include "GUIPassword.h"
#include "GUIDialogPictureInfo.h"
#include "addons/GUIDialogAddonInfo.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "dialogs/GUIDialogProgress.h"
#include "playlists/PlayListFactory.h"
#include "PictureInfoLoader.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "dialogs/GUIDialogOK.h"
#include "view/GUIViewState.h"
#include "PlayListPlayer.h"
#include "playlists/PlayList.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "Autorun.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "GUIWindowSlideShow.h"

#ifdef TARGET_POSIX
#include "linux/XTimeUtils.h"
#endif

#define CONTROL_BTNVIEWASICONS      2
#define CONTROL_BTNSORTBY           3
#define CONTROL_BTNSORTASC          4
#define CONTROL_LABELFILES         12

using namespace XFILE;
using namespace PLAYLIST;

#define CONTROL_BTNSLIDESHOW   6
#define CONTROL_BTNSLIDESHOW_RECURSIVE   7
#define CONTROL_SHUFFLE      9

CGUIWindowPictures::CGUIWindowPictures(void)
    : CGUIMediaWindow(WINDOW_PICTURES, "MyPics.xml")
{
  m_thumbLoader.SetObserver(this);
  m_slideShowStarted = false;
  m_dlgProgress = NULL;
}

void CGUIWindowPictures::OnInitWindow()
{
  CGUIMediaWindow::OnInitWindow();
  if (m_slideShowStarted)
  {
    CGUIWindowSlideShow* wndw = (CGUIWindowSlideShow*)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    std::string path;
    if (wndw && wndw->GetCurrentSlide())
      path = URIUtils::GetDirectory(wndw->GetCurrentSlide()->GetPath());
    if (m_vecItems->IsPath(path))
    {
      if (wndw && wndw->GetCurrentSlide())
        m_viewControl.SetSelectedItem(wndw->GetCurrentSlide()->GetPath());
      SaveSelectedItemInHistory();
    }
    m_slideShowStarted = false;
  }
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

    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      // is this the first time accessing this window?
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().empty())
        message.SetStringParam(CMediaSourceSettings::GetInstance().GetDefaultSource("pictures"));

      m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

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
        CSettings::GetInstance().ToggleBool(CSettings::SETTING_SLIDESHOW_SHUFFLE);
        CSettings::GetInstance().Save();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          if (CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_ALLOWFILEDELETION))
            OnDeleteItem(iItem);
          else
            return false;
        }
        else if (iAction == ACTION_PLAYER_PLAY)
        {
          ShowPicture(iItem, true);
          return true;
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          OnItemInfo(iItem);
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
  SET_CONTROL_SELECTED(GetID(), CONTROL_SHUFFLE, CSettings::GetInstance().GetBool(CSettings::SETTING_SLIDESHOW_SHUFFLE));

  // check we can slideshow or recursive slideshow
  int nFolders = m_vecItems->GetFolderCount();
  if (nFolders == m_vecItems->Size() ||
      m_vecItems->GetPath() == "addons://sources/image/")
  {
    CONTROL_DISABLE(CONTROL_BTNSLIDESHOW);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNSLIDESHOW);
  }
  if (m_guiState.get() && !m_guiState->HideParentDirItems())
    nFolders--;
  if (m_vecItems->Size() == 0 || nFolders == 0 ||
      m_vecItems->GetPath() == "addons://sources/image/")
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
  CGUIMediaWindow::OnPrepareFileItems(items);

  for (int i=0;i<items.Size();++i )
    if (StringUtils::EqualsNoCase(items[i]->GetLabel(), "folder.jpg"))
      items.Remove(i);

  if (items.GetFolderCount() == items.Size())
    return;

  // Start the music info loader thread
  CPictureInfoLoader loader;
  loader.SetProgressCallback(m_dlgProgress);
  loader.Load(items);

  bool bShowProgress=!g_windowManager.HasModalDialog();
  bool bProgressVisible=false;

  unsigned int tick=XbmcThreads::SystemClockMillis();

  while (loader.IsLoading() && m_dlgProgress && !m_dlgProgress->IsCanceled())
  {
    if (bShowProgress)
    { // Do we have to init a progress dialog?
      unsigned int elapsed=XbmcThreads::SystemClockMillis()-tick;

      if (!bProgressVisible && elapsed>1500 && m_dlgProgress)
      { // tag loading takes more then 1.5 secs, show a progress dialog
        CURL url(items.GetPath());

        m_dlgProgress->SetHeading(CVariant{189});
        m_dlgProgress->SetLine(0, CVariant{505});
        m_dlgProgress->SetLine(1, CVariant{""});
        m_dlgProgress->SetLine(2, CVariant{url.GetWithoutUserDetails()});
        m_dlgProgress->Open();
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

bool CGUIWindowPictures::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory, updateFilterPath))
    return false;

  m_vecItems->SetArt("thumb", "");
  if (CSettings::GetInstance().GetBool(CSettings::SETTING_PICTURES_GENERATETHUMBS))
    m_thumbLoader.Load(*m_vecItems);

  CPictureThumbLoader thumbLoader;
  std::string thumb = thumbLoader.GetCachedImage(*m_vecItems, "thumb");
  m_vecItems->SetArt("thumb", thumb);

  return true;
}

bool CGUIWindowPictures::OnClick(int iItem, const std::string &player)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return true;
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  if (pItem->IsCBZ() || pItem->IsCBR())
  {
    CURL pathToUrl;
    if (pItem->IsCBZ())
      pathToUrl = URIUtils::CreateArchivePath("zip", pItem->GetURL(), "");
    else
      pathToUrl = URIUtils::CreateArchivePath("rar", pItem->GetURL(), "");

    OnShowPictureRecursive(pathToUrl.Get());
    return true;
  }
  else if (CGUIMediaWindow::OnClick(iItem, player))
    return true;

  return false;
}

bool CGUIWindowPictures::GetDirectory(const std::string &strDirectory, CFileItemList& items)
{
  if (!CGUIMediaWindow::GetDirectory(strDirectory, items))
    return false;

  std::string label;
  if (items.GetLabel().empty() && m_rootDir.IsSource(items.GetPath(), CMediaSourceSettings::GetInstance().GetSources("pictures"), &label))
    items.SetLabel(label);

  if (items.GetContent().empty() && !items.IsVirtualDirectoryRoot() && !items.IsPlugin())
    items.SetContent("images");
  return true;
}

bool CGUIWindowPictures::OnPlayMedia(int iItem, const std::string &player)
{
  if (m_vecItems->Get(iItem)->IsVideo())
    return CGUIMediaWindow::OnPlayMedia(iItem);

  return ShowPicture(iItem, false);
}

bool CGUIWindowPictures::ShowPicture(int iItem, bool startSlideShow)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return false;
  CFileItemPtr pItem = m_vecItems->Get(iItem);
  std::string strPicture = pItem->GetPath();

#ifdef HAS_DVD_DRIVE
  if (pItem->IsDVD())
    return MEDIA_DETECT::CAutorun::PlayDiscAskResume(m_vecItems->Get(iItem)->GetPath());
#endif

  if (pItem->m_bIsShareOrDrive)
    return false;

  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
  if (!pSlideShow)
    return false;
  if (g_application.m_pPlayer->IsPlayingVideo())
    g_application.StopPlaying();

  pSlideShow->Reset();
  for (int i = 0; i < (int)m_vecItems->Size();++i)
  {
    CFileItemPtr pItem = m_vecItems->Get(i);
    if (!pItem->m_bIsFolder && !(URIUtils::IsRAR(pItem->GetPath()) || 
          URIUtils::IsZIP(pItem->GetPath())) && (pItem->IsPicture() || (
                                CSettings::GetInstance().GetBool(CSettings::SETTING_PICTURES_SHOWVIDEOS) &&
                                pItem->IsVideo())))
    {
      pSlideShow->Add(pItem.get());
    }
  }

  if (pSlideShow->NumSlides() == 0)
    return false;

  pSlideShow->Select(strPicture);

  if (startSlideShow)
    pSlideShow->StartSlideShow();
  else 
  {
    CVariant param;
    param["player"]["speed"] = 1;
    param["player"]["playerid"] = PLAYLIST_PICTURE;
    ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::Player, "xbmc", "OnPlay", pSlideShow->GetCurrentSlide(), param);
  }

  m_slideShowStarted = true;
  g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);

  return true;
}

void CGUIWindowPictures::OnShowPictureRecursive(const std::string& strPath)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
  if (pSlideShow)
  {
    // stop any video
    if (g_application.m_pPlayer->IsPlayingVideo())
      g_application.StopPlaying();

    SortDescription sorting = m_guiState->GetSortMethod();
    pSlideShow->AddFromPath(strPath, true,
                            sorting.sortBy, sorting.sortOrder, sorting.sortAttributes);
    if (pSlideShow->NumSlides())
    {
      m_slideShowStarted = true;
      g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
    }
  }
}

void CGUIWindowPictures::OnSlideShowRecursive(const std::string &strPicture)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
  if (pSlideShow)
  {   
    std::string strExtensions;
    CFileItemList items;
    CGUIViewState* viewState=CGUIViewState::GetViewState(GetID(), items);
    if (viewState)
    {
      strExtensions = viewState->GetExtensions();
      delete viewState;
    }
    m_slideShowStarted = true;

    SortDescription sorting = m_guiState->GetSortMethod();
    pSlideShow->RunSlideShow(strPicture, true,
                             CSettings::GetInstance().GetBool(CSettings::SETTING_SLIDESHOW_SHUFFLE),false,
                             "", true,
                             sorting.sortBy, sorting.sortOrder, sorting.sortAttributes,
                             strExtensions);
  }
}

void CGUIWindowPictures::OnSlideShowRecursive()
{
  std::string strEmpty = "";
  OnSlideShowRecursive(m_vecItems->GetPath());
}

void CGUIWindowPictures::OnSlideShow()
{
  OnSlideShow(m_vecItems->GetPath());
}

void CGUIWindowPictures::OnSlideShow(const std::string &strPicture)
{
  CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
  if (pSlideShow)
  {    
    std::string strExtensions;
    CFileItemList items;
    CGUIViewState* viewState=CGUIViewState::GetViewState(GetID(), items);
    if (viewState)
    {
      strExtensions = viewState->GetExtensions();
      delete viewState;
    }
    m_slideShowStarted = true;

    SortDescription sorting = m_guiState->GetSortMethod();
    pSlideShow->RunSlideShow(strPicture, false ,false, false,
                             "", true,
                             sorting.sortBy, sorting.sortOrder, sorting.sortAttributes,
                             strExtensions);
  }
}

void CGUIWindowPictures::OnRegenerateThumbs()
{
  if (m_thumbLoader.IsLoading()) return;
  m_thumbLoader.SetRegenerateThumbs(true);
  m_thumbLoader.Load(*m_vecItems);
}

void CGUIWindowPictures::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  if (item)
  {
    if ( m_vecItems->IsVirtualDirectoryRoot() || m_vecItems->GetPath() == "sources://pictures/" )
    {
      CGUIDialogContextMenu::GetContextButtons("pictures", item, buttons);
    }
    else
    {
      if (item)
      {
        if (!(item->m_bIsFolder || item->IsZIP() || item->IsRAR() || item->IsCBZ() || item->IsCBR() || item->IsScript()))
        {
          buttons.Add(CONTEXT_BUTTON_INFO, 13406); // picture info
          buttons.Add(CONTEXT_BUTTON_VIEW_SLIDESHOW, item->m_bIsFolder ? 13317 : 13422);      // View Slideshow
        }
        if (item->m_bIsFolder)
          buttons.Add(CONTEXT_BUTTON_RECURSIVE_SLIDESHOW, 13318);     // Recursive Slideshow

        if (!m_thumbLoader.IsLoading())
          buttons.Add(CONTEXT_BUTTON_REFRESH_THUMBS, 13315);         // Create Thumbnails
        if (CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_ALLOWFILEDELETION) && !item->IsReadOnly())
        {
          buttons.Add(CONTEXT_BUTTON_DELETE, 117);
          buttons.Add(CONTEXT_BUTTON_RENAME, 118);
        }
      }

      if (!item->IsPlugin() && !item->IsScript() && !m_vecItems->IsPlugin())
        buttons.Add(CONTEXT_BUTTON_SWITCH_MEDIA, 523);
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowPictures::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = (itemNumber >= 0 && itemNumber < m_vecItems->Size()) ? m_vecItems->Get(itemNumber) : CFileItemPtr();
  if (CGUIDialogContextMenu::OnContextButton("pictures", item, button))
  {
    Update("");
    return true;
  }
  switch (button)
  {
  case CONTEXT_BUTTON_VIEW_SLIDESHOW:
    if (item && item->m_bIsFolder)
      OnSlideShow(item->GetPath());
    else
      ShowPicture(itemNumber, true);
    return true;
  case CONTEXT_BUTTON_RECURSIVE_SLIDESHOW:
    if (item)
      OnSlideShowRecursive(item->GetPath());
    return true;
  case CONTEXT_BUTTON_INFO:
    OnItemInfo(itemNumber);
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
  case CONTEXT_BUTTON_SWITCH_MEDIA:
    CGUIDialogContextMenu::SwitchMedia("pictures", m_vecItems->GetPath());
    return true;
  default:
    break;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowPictures::OnAddMediaSource()
{
  return CGUIDialogMediaSource::ShowAndAddMediaSource("pictures");
}

void CGUIWindowPictures::OnItemLoaded(CFileItem *pItem)
{
  CPictureThumbLoader::ProcessFoldersAndArchives(pItem);
}

void CGUIWindowPictures::LoadPlayList(const std::string& strPlayList)
{
  CLog::Log(LOGDEBUG,"CGUIWindowPictures::LoadPlayList()... converting playlist into slideshow: %s", strPlayList.c_str());
  std::unique_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if ( NULL != pPlayList.get())
  {
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(CVariant{6}, CVariant{477});
      return ; //hmmm unable to load playlist?
    }
  }

  CPlayList playlist = *pPlayList;
  if (playlist.size() > 0)
  {
    // set up slideshow
    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (!pSlideShow)
      return;
    if (g_application.m_pPlayer->IsPlayingVideo())
      g_application.StopPlaying();

    // convert playlist items into slideshow items
    pSlideShow->Reset();
    for (int i = 0; i < (int)playlist.size(); ++i)
    {
      CFileItemPtr pItem = playlist[i];
      //CLog::Log(LOGDEBUG,"-- playlist item: %s", pItem->GetPath().c_str());
      if (pItem->IsPicture() && !(pItem->IsZIP() || pItem->IsRAR() || pItem->IsCBZ() || pItem->IsCBR()))
        pSlideShow->Add(pItem.get());
    }

    // start slideshow if there are items
    pSlideShow->StartSlideShow();
    if (pSlideShow->NumSlides())
      g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
  }
}

void CGUIWindowPictures::OnItemInfo(int itemNumber)
{
  CFileItemPtr item = (itemNumber >= 0 && itemNumber < m_vecItems->Size()) ? m_vecItems->Get(itemNumber) : CFileItemPtr();
  if (!item)
    return;
  if (!m_vecItems->IsPlugin() && (item->IsPlugin() || item->IsScript()))
  {
    CGUIDialogAddonInfo::ShowForItem(item);
    return;
  }
  if (item->m_bIsFolder || item->IsZIP() || item->IsRAR() || item->IsCBZ() || item->IsCBR() || !item->IsPicture())
    return;
  CGUIDialogPictureInfo *pictureInfo = (CGUIDialogPictureInfo *)g_windowManager.GetWindow(WINDOW_DIALOG_PICTURE_INFO);
  if (pictureInfo)
  {
    pictureInfo->SetPicture(item.get());
    pictureInfo->Open();
  }
}

std::string CGUIWindowPictures::GetStartFolder(const std::string &dir)
{
  std::string lower(dir); StringUtils::ToLower(lower);
  if (lower == "plugins" || lower == "addons")
    return "addons://sources/image/";

  SetupShares();
  VECSOURCES shares;
  m_rootDir.GetSources(shares);
  bool bIsSourceName = false;
  int iIndex = CUtil::GetMatchingSource(dir, shares, bIsSourceName);
  if (iIndex > -1)
  {
    if (iIndex < (int)shares.size() && shares[iIndex].m_iHasLock == 2)
    {
      CFileItem item(shares[iIndex]);
      if (!g_passwordManager.IsItemUnlocked(&item,"pictures"))
        return "";
    }
    if (bIsSourceName)
      return shares[iIndex].strPath;
    return dir;
  }
  return CGUIMediaWindow::GetStartFolder(dir);
}
