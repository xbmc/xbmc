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

#include "system.h"
#include "GUIWindowVideoBase.h"
#include "Util.h"
#include "video/VideoInfoDownloader.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoLibraryQueue.h"
#include "addons/GUIDialogAddonInfo.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "view/GUIViewState.h"
#include "playlists/PlayListFactory.h"
#include "Application.h"
#include "NfoFile.h"
#include "PlayListPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "GUIPassword.h"
#include "filesystem/StackDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "PartyModeManager.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIKeyboardFactory.h"
#include "filesystem/Directory.h"
#include "playlists/PlayList.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/dialogs/GUIDialogContentSettings.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "utils/FileUtils.h"
#include "utils/Variant.h"
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/recordings/PVRRecordingsPath.h"
#include "utils/URIUtils.h"
#include "GUIUserMessages.h"
#include "storage/MediaManager.h"
#include "Autorun.h"
#include "URL.h"
#include "utils/GroupUtils.h"
#include "TextureDatabase.h"

using namespace XFILE;
using namespace PLAYLIST;
using namespace VIDEODATABASEDIRECTORY;
using namespace VIDEO;
using namespace ADDON;
using namespace PVR;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_LABELFILES        12

#define CONTROL_PLAY_DVD           6

#define PROPERTY_GROUP_BY           "group.by"
#define PROPERTY_GROUP_MIXED        "group.mixed"

CGUIWindowVideoBase::CGUIWindowVideoBase(int id, const std::string &xmlFile)
    : CGUIMediaWindow(id, xmlFile.c_str())
{
  m_thumbLoader.SetObserver(this);
  m_stackingAvailable = true;
  m_dlgProgress = NULL;
}

CGUIWindowVideoBase::~CGUIWindowVideoBase()
{
}

bool CGUIWindowVideoBase::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SCAN_ITEM)
    return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_SCAN);
  else if (action.GetID() == ACTION_SHOW_PLAYLIST)
  {
    if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO ||
        g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).size() > 0)
    {
      g_windowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
      return true;
    }
  }

  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowVideoBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    m_database.Close();
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_database.Open();
      m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      return CGUIMediaWindow::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
#if defined(HAS_DVD_DRIVE)
      if (iControl == CONTROL_PLAY_DVD)
      {
        // play movie...
        MEDIA_DETECT::CAutorun::PlayDiscAskResume(g_mediaManager.TranslateDevicePath(""));
      }
      else
#endif
      if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        // get selected item
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          OnQueueItem(iItem);
          return true;
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          return OnItemInfo(iItem);
        }
        else if (iAction == ACTION_PLAYER_PLAY)
        {
          // if playback is paused or playback speed != 1, return
          if (g_application.m_pPlayer->IsPlayingVideo())
          {
            if (g_application.m_pPlayer->IsPausedPlayback())
              return false;
            if (g_application.m_pPlayer->GetPlaySpeed() != 1)
              return false;
          }

          // not playing video, or playback speed == 1
          return OnResumeItem(iItem);          
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          if (CProfilesManager::GetInstance().GetCurrentProfile().canWriteDatabases())
          {
            // must be at the title window
            if (GetID() == WINDOW_VIDEO_NAV)
              OnDeleteItem(iItem);

            // or be at the video playlists location
            else if (m_vecItems->IsPath("special://videoplaylists/"))
              OnDeleteItem(iItem);
            else
              return false;

            return true;
          }
        }
      }
    }
    break;
  case GUI_MSG_SEARCH:
    OnSearch();
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowVideoBase::OnItemInfo(const CFileItem& fileItem, ADDON::ScraperPtr& scraper)
{
  if (fileItem.IsParentFolder() || fileItem.m_bIsShareOrDrive || fileItem.IsPath("add") ||
     (fileItem.IsPlayList() && !URIUtils::HasExtension(fileItem.GetPath(), ".strm")))
    return;

  CFileItem item(fileItem);
  bool fromDB = false;
  if ((item.IsVideoDb() && item.HasVideoInfoTag()) ||
      (item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_iDbId != -1))
  {
    if (item.GetVideoInfoTag()->m_type == MediaTypeSeason)
    { // clear out the art - we're really grabbing the info on the show here
      item.ClearArt();
      item.GetVideoInfoTag()->m_iDbId = item.GetVideoInfoTag()->m_iIdShow;
    }
    item.SetPath(item.GetVideoInfoTag()->GetPath());
    fromDB = true;
  }
  else
  {
    if (item.m_bIsFolder && scraper && scraper->Content() != CONTENT_TVSHOWS)
    {
      CFileItemList items;
      CDirectory::GetDirectory(item.GetPath(), items, g_advancedSettings.m_videoExtensions);
      items.Stack();

      // check for media files
      bool bFoundFile(false);
      for (int i = 0; i < items.Size(); ++i)
      {
        CFileItemPtr item2 = items[i];

        if (item2->IsVideo() && !item2->IsPlayList() &&
            !CUtil::ExcludeFileOrFolder(item2->GetPath(), g_advancedSettings.m_moviesExcludeFromScanRegExps))
        {
          item.SetPath(item2->GetPath());
          item.m_bIsFolder = false;
          bFoundFile = true;
          break;
        }
      }

      // no video file in this folder
      if (!bFoundFile)
      {
        CGUIDialogOK::ShowAndGetInput(CVariant{13346}, CVariant{20349});
        return;
      }
    }
  }

  // we need to also request any thumbs be applied to the folder item
  if (fileItem.m_bIsFolder)
    item.SetProperty("set_folder_thumb", fileItem.GetPath());

  bool modified = ShowIMDB(CFileItemPtr(new CFileItem(item)), scraper, fromDB);
  if (modified &&
     (g_windowManager.GetActiveWindow() == WINDOW_VIDEO_NAV)) // since we can be called from the music library we need this check
  {
    int itemNumber = m_viewControl.GetSelectedItem();
    Refresh();
    m_viewControl.SetSelectedItem(itemNumber);
  }
}

// ShowIMDB is called as follows:
// 1.  To lookup info on a file.
// 2.  To lookup info on a folder (which may or may not contain a file)
// 3.  To lookup info just for fun (no file or folder related)

// We just need the item object for this.
// A "blank" item object is sent for 3.
// If a folder is sent, currently it sets strFolder and bFolder
// this is only used for setting the folder thumb, however.

// Steps should be:

// 1.  Check database to see if we have this information already
// 2.  Else, check for a nfoFile to get the URL
// 3.  Run a loop to check for refresh
// 4.  If no URL is present do a search to get the URL
// 4.  Once we have the URL, download the details
// 5.  Once we have the details, add to the database if necessary (case 1,2)
//     and show the information.
// 6.  Check for a refresh, and if so, go to 3.

bool CGUIWindowVideoBase::ShowIMDB(CFileItemPtr item, const ScraperPtr &info2, bool fromDB)
{
  /*
  CLog::Log(LOGDEBUG,"CGUIWindowVideoBase::ShowIMDB");
  CLog::Log(LOGDEBUG,"  strMovie  = [%s]", strMovie.c_str());
  CLog::Log(LOGDEBUG,"  strFile   = [%s]", strFile.c_str());
  CLog::Log(LOGDEBUG,"  strFolder = [%s]", strFolder.c_str());
  CLog::Log(LOGDEBUG,"  bFolder   = [%s]", ((int)bFolder ? "true" : "false"));
  */

  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  CGUIDialogVideoInfo* pDlgInfo = (CGUIDialogVideoInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_INFO);

  ScraperPtr info(info2); // use this as nfo might change it..

  if (!pDlgProgress) return false;
  if (!pDlgSelect) return false;
  if (!pDlgInfo) return false;

  // 1.  Check for already downloaded information, and if we have it, display our dialog
  //     Return if no Refresh is needed.
  bool bHasInfo=false;

  CVideoInfoTag movieDetails;
  if (info)
  {
    m_database.Open(); // since we can be called from the music library

    int dbId = item->HasVideoInfoTag() ? item->GetVideoInfoTag()->m_iDbId : -1;
    if (info->Content() == CONTENT_MOVIES)
    {
      bHasInfo = m_database.GetMovieInfo(item->GetPath(), movieDetails, dbId);
    }
    if (info->Content() == CONTENT_TVSHOWS)
    {
      if (item->m_bIsFolder)
      {
        bHasInfo = m_database.GetTvShowInfo(item->GetPath(), movieDetails, dbId);
      }
      else
      {
        bHasInfo = m_database.GetEpisodeInfo(item->GetPath(), movieDetails, dbId);
        if (!bHasInfo)
        {
          // !! WORKAROUND !!
          // As we cannot add an episode to a non-existing tvshow entry, we have to check the parent directory
          // to see if it`s already in our video database. If it's not yet part of the database we will exit here.
          // (Ticket #4764)
          //
          // NOTE: This will fail for episodes on multipath shares, as the parent path isn't what is stored in the
          //       database.  Possible solutions are to store the paths in the db separately and rely on the show
          //       stacking stuff, or to modify GetTvShowId to do support multipath:// shares
          std::string strParentDirectory;
          URIUtils::GetParentPath(item->GetPath(), strParentDirectory);
          if (m_database.GetTvShowId(strParentDirectory) < 0)
          {
            CLog::Log(LOGERROR,"%s: could not add episode [%s]. tvshow does not exist yet..", __FUNCTION__, item->GetPath().c_str());
            return false;
          }
        }
      }
    }
    if (info->Content() == CONTENT_MUSICVIDEOS)
    {
      bHasInfo = m_database.GetMusicVideoInfo(item->GetPath(), movieDetails);
    }
    m_database.Close();
  }
  else if(item->HasVideoInfoTag())
  {
    bHasInfo = true;
    movieDetails = *item->GetVideoInfoTag();
  }
  
  bool needsRefresh = false;
  if (bHasInfo)
  {
    if (!info || info->Content() == CONTENT_NONE) // disable refresh button
      movieDetails.SetUniqueID("xx"+movieDetails.GetUniqueID());
    *item->GetVideoInfoTag() = movieDetails;
    pDlgInfo->SetMovie(item.get());
    pDlgInfo->Open();
    if (pDlgInfo->HasUpdatedUserrating())
      return true;
    needsRefresh = pDlgInfo->NeedRefresh();
    if (!needsRefresh)
      return pDlgInfo->HasUpdatedThumb();
    // check if the item in the video info dialog has changed and if so, get the new item
    else if (pDlgInfo->GetCurrentListItem() != NULL)
    {
      item = pDlgInfo->GetCurrentListItem();

      if (item->IsVideoDb() && item->HasVideoInfoTag())
        item->SetPath(item->GetVideoInfoTag()->GetPath());
    }
  }

  // quietly return if Internet lookups are disabled
  if (!CProfilesManager::GetInstance().GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser)
    return false;

  if (!info)
    return false;

  if (g_application.IsVideoScanning())
  {
    CGUIDialogOK::ShowAndGetInput(CVariant{13346}, CVariant{14057});
    return false;
  }

  bool listNeedsUpdating = false;
  // 3. Run a loop so that if we Refresh we re-run this block
  do
  {
    if (!CVideoLibraryQueue::GetInstance().RefreshItemModal(item, needsRefresh, pDlgInfo->RefreshAll()))
      return listNeedsUpdating;

    // remove directory caches and reload images
    CUtil::DeleteVideoDatabaseDirectoryCache();
    CGUIMessage reload(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
    OnMessage(reload);

    pDlgInfo->SetMovie(item.get());
    pDlgInfo->Open();
    item->SetArt("thumb", pDlgInfo->GetThumbnail());
    needsRefresh = pDlgInfo->NeedRefresh();
    listNeedsUpdating = true;
  } while (needsRefresh);

  return listNeedsUpdating;
}

void CGUIWindowVideoBase::OnQueueItem(int iItem)
{
  // Determine the proper list to queue this element
  int playlist = g_playlistPlayer.GetCurrentPlaylist();
  if (playlist == PLAYLIST_NONE)
    playlist = g_application.m_pPlayer->GetPreferredPlaylist();
  if (playlist == PLAYLIST_NONE)
    playlist = PLAYLIST_VIDEO;

  // don't re-queue items from playlist window
  if ( iItem < 0 || iItem >= m_vecItems->Size() || GetID() == WINDOW_VIDEO_PLAYLIST ) return ;

  // we take a copy so that we can alter the queue state
  CFileItemPtr item(new CFileItem(*m_vecItems->Get(iItem)));
  if (item->IsRAR() || item->IsZIP())
    return;

  //  Allow queuing of unqueueable items
  //  when we try to queue them directly
  if (!item->CanQueue())
    item->SetCanQueue(true);

  CFileItemList queuedItems;
  AddItemToPlayList(item, queuedItems);
  // if party mode, add items but DONT start playing
  if (g_partyModeManager.IsEnabled(PARTYMODECONTEXT_VIDEO))
  {
    g_partyModeManager.AddUserSongs(queuedItems, false);
    return;
  }

  g_playlistPlayer.Add(playlist, queuedItems);
  g_playlistPlayer.SetCurrentPlaylist(playlist);
  // video does not auto play on queue like music
  m_viewControl.SetSelectedItem(iItem + 1);
}

void CGUIWindowVideoBase::AddItemToPlayList(const CFileItemPtr &pItem, CFileItemList &queuedItems)
{
  if (!pItem->CanQueue() || pItem->IsRAR() || pItem->IsZIP() || pItem->IsParentFolder()) // no zip/rar enques thank you!
    return;

  if (pItem->m_bIsFolder)
  {
    if (pItem->IsParentFolder())
      return;

    // check if it's a folder with dvd or bluray files, then just add the relevant file
    std::string mediapath(pItem->GetOpticalMediaPath());
    if (!mediapath.empty())
    {
      CFileItemPtr item(new CFileItem(mediapath, false));
      queuedItems.Add(item);
      return;
    }

    // Check if we add a locked share
    if ( pItem->m_bIsShareOrDrive )
    {
      CFileItem item = *pItem;
      if ( !g_passwordManager.IsItemUnlocked( &item, "video" ) )
        return;
    }

    // recursive
    CFileItemList items;
    GetDirectory(pItem->GetPath(), items);
    FormatAndSort(items);

    int watchedMode = CMediaSettings::GetInstance().GetWatchedMode(items.GetContent());
    bool unwatchedOnly = watchedMode == WatchedModeUnwatched;
    bool watchedOnly = watchedMode == WatchedModeWatched;
    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder)
      {
        std::string strPath = items[i]->GetPath();
        URIUtils::RemoveSlashAtEnd(strPath);
        if (StringUtils::EndsWithNoCase(strPath, "sample")) // skip sample folders
        {
          continue;
        }
      }
      else if (items[i]->HasVideoInfoTag() &&
       ((unwatchedOnly && items[i]->GetVideoInfoTag()->m_playCount > 0) ||
        (watchedOnly && items[i]->GetVideoInfoTag()->m_playCount <= 0)))
        continue;

      AddItemToPlayList(items[i], queuedItems);
    }
  }
  else
  {
    // just an item
    if (pItem->IsPlayList())
    {
      std::unique_ptr<CPlayList> pPlayList (CPlayListFactory::Create(*pItem));
      if (pPlayList.get())
      {
        // load it
        if (!pPlayList->Load(pItem->GetPath()))
        {
          CGUIDialogOK::ShowAndGetInput(CVariant{6}, CVariant{477});
          return; //hmmm unable to load playlist?
        }

        CPlayList playlist = *pPlayList;
        for (int i = 0; i < (int)playlist.size(); ++i)
        {
          AddItemToPlayList(playlist[i], queuedItems);
        }
        return;
      }
    }
    else if(pItem->IsInternetStream())
    { // just queue the internet stream, it will be expanded on play
      queuedItems.Add(pItem);
    }
    else if (pItem->IsPlugin() && pItem->GetProperty("isplayable") == "true")
    { // a playable python files
      queuedItems.Add(pItem);
    }
    else if (pItem->IsVideoDb())
    { // this case is needed unless we allow IsVideo() to return true for videodb items,
      // but then we have issues with playlists of videodb items
      CFileItemPtr item(new CFileItem(*pItem->GetVideoInfoTag()));
      queuedItems.Add(item);
    }
    else if (!pItem->IsNFO() && pItem->IsVideo())
    {
      queuedItems.Add(pItem);
    }
  }
}

void CGUIWindowVideoBase::GetResumeItemOffset(const CFileItem *item, int& startoffset, int& partNumber)
{
  // do not resume livetv
  if (item->IsLiveTV())
    return;

  startoffset = 0;
  partNumber = 0;

  if (!item->IsNFO() && !item->IsPlayList())
  {
    if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_resumePoint.IsSet())
    {
      startoffset = (int)(item->GetVideoInfoTag()->m_resumePoint.timeInSeconds*75);
      partNumber = item->GetVideoInfoTag()->m_resumePoint.partNumber;
    }
    else
    {
      CBookmark bookmark;
      std::string strPath = item->GetPath();
      if ((item->IsVideoDb() || item->IsDVD()) && item->HasVideoInfoTag())
        strPath = item->GetVideoInfoTag()->m_strFileNameAndPath;

      CVideoDatabase db;
      if (!db.Open())
      {
        CLog::Log(LOGERROR, "%s - Cannot open VideoDatabase", __FUNCTION__);
        return;
      }
      if (db.GetResumeBookMark(strPath, bookmark))
      {
        startoffset = (int)(bookmark.timeInSeconds*75);
        partNumber = bookmark.partNumber;
      }
      db.Close();
    }
  }
}

bool CGUIWindowVideoBase::HasResumeItemOffset(const CFileItem *item)
{
  int startoffset = 0, partNumber = 0;
  GetResumeItemOffset(item, startoffset, partNumber);
  return startoffset > 0;
}

bool CGUIWindowVideoBase::OnClick(int iItem, const std::string &player)
{
  return CGUIMediaWindow::OnClick(iItem, player);
}

bool CGUIWindowVideoBase::OnSelect(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  CFileItemPtr item = m_vecItems->Get(iItem);

  std::string path = item->GetPath();
  if (!item->m_bIsFolder && path != "add" &&
      !StringUtils::StartsWith(path, "newsmartplaylist://") &&
      !StringUtils::StartsWith(path, "newplaylist://") &&
      !StringUtils::StartsWith(path, "newtag://") &&
      !StringUtils::StartsWith(path, "script://"))
    return OnFileAction(iItem, CSettings::GetInstance().GetInt(CSettings::SETTING_MYVIDEOS_SELECTACTION), "");

  return CGUIMediaWindow::OnSelect(iItem);
}

bool CGUIWindowVideoBase::OnFileAction(int iItem, int action, std::string player)
{
  CFileItemPtr item = m_vecItems->Get(iItem);

  // Reset the current start offset. The actual resume
  // option is set in the switch, based on the action passed.
  item->m_lStartOffset = 0;
  
  switch (action)
  {
  case SELECT_ACTION_CHOOSE:
    {
      CContextButtons choices;

      if (item->IsVideoDb())
      {
        std::string itemPath(item->GetPath());
        itemPath = item->GetVideoInfoTag()->m_strFileNameAndPath;
        if (URIUtils::IsStack(itemPath) && CFileItem(CStackDirectory::GetFirstStackedFile(itemPath),false).IsDiscImage())
          choices.Add(SELECT_ACTION_PLAYPART, 20324); // Play Part
      }

      std::string resumeString = GetResumeString(*item);
      if (!resumeString.empty())
      {
        choices.Add(SELECT_ACTION_RESUME, resumeString);
        choices.Add(SELECT_ACTION_PLAY, 12021);   // Start from beginning
      }
      else
        choices.Add(SELECT_ACTION_PLAY, 208);   // Play

      choices.Add(SELECT_ACTION_INFO, 22081); // Info
      choices.Add(SELECT_ACTION_MORE, 22082); // More
      int value = CGUIDialogContextMenu::ShowAndGetChoice(choices);
      if (value < 0)
        return true;

      return OnFileAction(iItem, value, player);
    }
    break;
  case SELECT_ACTION_PLAY_OR_RESUME:
    return OnResumeItem(iItem, player);
  case SELECT_ACTION_INFO:
    if (OnItemInfo(iItem))
      return true;
    break;
  case SELECT_ACTION_MORE:
    OnPopupMenu(iItem);
    return true;
  case SELECT_ACTION_RESUME:
    item->m_lStartOffset = STARTOFFSET_RESUME;
    break;
  case SELECT_ACTION_PLAYPART:
    if (!OnPlayStackPart(iItem))
      return false;
    break;
  case SELECT_ACTION_PLAY:
  default:
    break;
  }
  return OnClick(iItem, player);
}

bool CGUIWindowVideoBase::OnItemInfo(int iItem) 
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  CFileItemPtr item = m_vecItems->Get(iItem);

  if (item->IsPath("add") || item->IsParentFolder() ||
     (item->IsPlayList() && !URIUtils::HasExtension(item->GetPath(), ".strm")))
    return false;

  if (!m_vecItems->IsPlugin() && (item->IsPlugin() || item->IsScript()))
    return CGUIDialogAddonInfo::ShowForItem(item);

  ADDON::ScraperPtr scraper;
  if (!m_vecItems->IsPlugin() && !m_vecItems->IsRSS() && !m_vecItems->IsLiveTV())
  {
    std::string strDir;
    if (item->IsVideoDb()       &&
        item->HasVideoInfoTag() &&
        !item->GetVideoInfoTag()->m_strPath.empty())
    {
      strDir = item->GetVideoInfoTag()->m_strPath;
    }
    else
      strDir = URIUtils::GetDirectory(item->GetPath());

    SScanSettings settings;
    bool foundDirectly = false;
    scraper = m_database.GetScraperForPath(strDir, settings, foundDirectly);

    if (!scraper &&
        !(m_database.HasMovieInfo(item->GetPath()) ||
          m_database.HasTvShowInfo(strDir)           ||
          m_database.HasEpisodeInfo(item->GetPath())))
    {
      return false;
    }

    if (scraper && scraper->Content() == CONTENT_TVSHOWS && foundDirectly && !settings.parent_name_root) // dont lookup on root tvshow folder
      return true;
  }

  OnItemInfo(*item, scraper);

  // Return whether or not we have information to display.
  // Note: This will cause the default select action to start
  // playback in case it's set to "Show information".
  return item->HasVideoInfoTag();
}

void CGUIWindowVideoBase::OnRestartItem(int iItem, const std::string &player)
{
  CGUIMediaWindow::OnClick(iItem, player);
}

std::string CGUIWindowVideoBase::GetResumeString(const CFileItem &item)
{
  std::string resumeString;
  int startOffset = 0, startPart = 0;
  GetResumeItemOffset(&item, startOffset, startPart);
  if (startOffset > 0)
  {
    resumeString = StringUtils::Format(g_localizeStrings.Get(12022).c_str(),
        StringUtils::SecondsToTimeString(startOffset/75, TIME_FORMAT_HH_MM_SS).c_str());
    if (startPart > 0)
    {
      std::string partString = StringUtils::Format(g_localizeStrings.Get(23051).c_str(), startPart);
      resumeString += " (" + partString + ")";
    }
  }
  return resumeString;
}

bool CGUIWindowVideoBase::ShowResumeMenu(CFileItem &item)
{
  if (!item.m_bIsFolder && !item.IsLiveTV())
  {
    std::string resumeString = GetResumeString(item);
    if (!resumeString.empty())
    { // prompt user whether they wish to resume
      CContextButtons choices;
      choices.Add(1, resumeString);
      choices.Add(2, 12021); // start from the beginning
      int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices);
      if (retVal < 0)
        return false; // don't do anything
      if (retVal == 1)
        item.m_lStartOffset = STARTOFFSET_RESUME;
    }
  }
  return true;
}

bool CGUIWindowVideoBase::OnResumeItem(int iItem, const std::string &player)
{
  if (iItem < 0 || iItem >= m_vecItems->Size()) return true;
  CFileItemPtr item = m_vecItems->Get(iItem);

  if (item->m_bIsFolder)
  {
    // resuming directories isn't supported yet. play.
    PlayItem(iItem, player);
    return true;
  }

  std::string resumeString = GetResumeString(*item);

  if (!resumeString.empty())
  {
    CContextButtons choices;
    choices.Add(SELECT_ACTION_RESUME, resumeString);
    choices.Add(SELECT_ACTION_PLAY, 12021);   // Start from beginning
    int value = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if (value < 0)
      return true;
    return OnFileAction(iItem, value, player);
  }

  return OnFileAction(iItem, SELECT_ACTION_PLAY, player);
}

void CGUIWindowVideoBase::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  // contextual buttons
  if (item)
  {
    if (!item->IsParentFolder())
    {
      std::string path(item->GetPath());
      if (item->IsVideoDb() && item->HasVideoInfoTag())
        path = item->GetVideoInfoTag()->m_strFileNameAndPath;

      if (!item->IsPath("add") && !item->IsPlugin() &&
          !item->IsScript() && !item->IsAddonsPath() && !item->IsLiveTV())
      {
        if (URIUtils::IsStack(path))
        {
          std::vector<int> times;
          if (m_database.GetStackTimes(path,times) || CFileItem(CStackDirectory::GetFirstStackedFile(path),false).IsDiscImage())
            buttons.Add(CONTEXT_BUTTON_PLAY_PART, 20324);
        }

        // allow a folder to be ad-hoc queued and played by the default player
        if (item->m_bIsFolder || (item->IsPlayList() &&
           !g_advancedSettings.m_playlistAsFolders))
        {
          buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 208);
        }

        if (!m_vecItems->GetPath().empty() && !StringUtils::StartsWithNoCase(item->GetPath(), "newsmartplaylist://") && !StringUtils::StartsWithNoCase(item->GetPath(), "newtag://")
            && !m_vecItems->IsSourcesPath())
        {
          buttons.Add(CONTEXT_BUTTON_QUEUE_ITEM, 13347);      // Add to Playlist
        }
      }

      if (!item->m_bIsFolder && !(item->IsPlayList() && !g_advancedSettings.m_playlistAsFolders))
      { // get players
        std::vector<std::string> players;
        if (item->IsVideoDb())
        {
          CFileItem item2(item->GetVideoInfoTag()->m_strFileNameAndPath, false);
          CPlayerCoreFactory::GetInstance().GetPlayers(item2, players);
        }
        else
          CPlayerCoreFactory::GetInstance().GetPlayers(*item, players);
        if (players.size() > 1)
          buttons.Add(CONTEXT_BUTTON_PLAY_WITH, 15213);
      }
      if (item->IsSmartPlayList())
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_PARTYMODE, 15216); // Play in Partymode
      }

      //if the item isn't a folder or script, is a member of a list rather than a single item
      //and we're not on the last element of the list, 
      //then add add either 'play from here' or 'play only this' depending on default behaviour
      if (!(item->m_bIsFolder || item->IsScript()) && m_vecItems->Size() > 1 && itemNumber < m_vecItems->Size()-1)
      {
        if (!CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOPLAYER_AUTOPLAYNEXTITEM))
          buttons.Add(CONTEXT_BUTTON_PLAY_AND_QUEUE, 13412);
        else
          buttons.Add(CONTEXT_BUTTON_PLAY_ONLY_THIS, 13434);
      }
      if (item->IsSmartPlayList() || m_vecItems->IsSmartPlayList())
        buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowVideoBase::OnPlayStackPart(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  CFileItemPtr stack = m_vecItems->Get(iItem);
  std::string path(stack->GetPath());
  if (stack->IsVideoDb())
    path = stack->GetVideoInfoTag()->m_strFileNameAndPath;

  if (!URIUtils::IsStack(path))
    return false;

  CFileItemList parts;
  CDirectory::GetDirectory(path, parts);

  for (int i = 0; i < parts.Size(); i++)
    parts[i]->SetLabel(StringUtils::Format(g_localizeStrings.Get(23051).c_str(), i+1));

  CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

  pDialog->Reset();
  pDialog->SetHeading(CVariant{20324});
  pDialog->SetItems(parts);
  pDialog->Open();

  if (!pDialog->IsConfirmed())
    return false;

  int selectedFile = pDialog->GetSelectedItem();
  if (selectedFile >= 0)
  {
    // ISO stack
    if (CFileItem(CStackDirectory::GetFirstStackedFile(path),false).IsDiscImage())
    {
      std::string resumeString = CGUIWindowVideoBase::GetResumeString(*(parts[selectedFile].get()));
      stack->m_lStartOffset = 0;
      if (!resumeString.empty()) 
      {
        CContextButtons choices;
        choices.Add(SELECT_ACTION_RESUME, resumeString);
        choices.Add(SELECT_ACTION_PLAY, 12021);   // Start from beginning
        int value = CGUIDialogContextMenu::ShowAndGetChoice(choices);
        if (value == SELECT_ACTION_RESUME)
          GetResumeItemOffset(parts[selectedFile].get(), stack->m_lStartOffset, stack->m_lStartPartNumber);
        else if (value != SELECT_ACTION_PLAY)
          return false; // if not selected PLAY, then we changed our mind so return
      }
      stack->m_lStartPartNumber = selectedFile + 1;
    }
    // regular stack
    else
    {
      if (selectedFile > 0)
      {
        std::vector<int> times;
        if (m_database.GetStackTimes(path,times))
          stack->m_lStartOffset = times[selectedFile - 1] * 75;
      }
      else
        stack->m_lStartOffset = 0;
    }


  }

  return true;
}

bool CGUIWindowVideoBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  switch (button)
  {
  case CONTEXT_BUTTON_SET_CONTENT:
    {
      OnAssignContent(item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_strPath.empty() ? item->GetVideoInfoTag()->m_strPath : static_cast<const std::string&>(item->GetPath()));
      return true;
    }
  case CONTEXT_BUTTON_PLAY_PART:
    {
      if (OnPlayStackPart(itemNumber)) 
      {
        // call CGUIMediaWindow::OnClick() as otherwise autoresume will kick in
        CGUIMediaWindow::OnClick(itemNumber);
        return true;
      }
      else
        return false;
    }
  case CONTEXT_BUTTON_QUEUE_ITEM:
    OnQueueItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_PLAY_ITEM:
    PlayItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_PLAY_WITH:
    {
      std::vector<std::string> players;
      if (item->IsVideoDb())
      {
        CFileItem item2(*item->GetVideoInfoTag());
        CPlayerCoreFactory::GetInstance().GetPlayers(item2, players);
      }
      else
        CPlayerCoreFactory::GetInstance().GetPlayers(*item, players);
      std:: string player = CPlayerCoreFactory::GetInstance().SelectPlayerDialog(players);
      if (!player.empty())
      {
        // any other select actions but play or resume, resume, play or playpart
        // don't make any sense here since the user already decided that he'd
        // like to play the item (just with a specific player)
        VideoSelectAction selectAction = (VideoSelectAction)CSettings::GetInstance().GetInt(CSettings::SETTING_MYVIDEOS_SELECTACTION);
        if (selectAction != SELECT_ACTION_PLAY_OR_RESUME &&
            selectAction != SELECT_ACTION_RESUME &&
            selectAction != SELECT_ACTION_PLAY &&
            selectAction != SELECT_ACTION_PLAYPART)
          selectAction = SELECT_ACTION_PLAY_OR_RESUME;
        return OnFileAction(itemNumber, selectAction, player);
      }
      return true;
    }

  case CONTEXT_BUTTON_PLAY_PARTYMODE:
    g_partyModeManager.Enable(PARTYMODECONTEXT_VIDEO, m_vecItems->Get(itemNumber)->GetPath());
    return true;

  case CONTEXT_BUTTON_SCAN:
    {
      if( !item)
        return false;
      ADDON::ScraperPtr info;
      SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);
      std::string strPath = item->GetPath();
      if (item->IsVideoDb() && (!item->m_bIsFolder || item->GetVideoInfoTag()->m_strPath.empty()))
        return false;

      if (item->IsVideoDb())
        strPath = item->GetVideoInfoTag()->m_strPath;

      if (!info || info->Content() == CONTENT_NONE)
        return false;

      if (item->m_bIsFolder)
      {
        OnScan(strPath, true);
      }
      else
        OnItemInfo(*item, info);

      return true;
    }
  case CONTEXT_BUTTON_DELETE:
    OnDeleteItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_EDIT_SMART_PLAYLIST:
    {
      std::string playlist = m_vecItems->Get(itemNumber)->IsSmartPlayList() ? m_vecItems->Get(itemNumber)->GetPath() : m_vecItems->GetPath(); // save path as activatewindow will destroy our items
      if (CGUIDialogSmartPlaylistEditor::EditPlaylist(playlist, "video"))
        Refresh(true); // need to update
      return true;
    }
  case CONTEXT_BUTTON_RENAME:
    OnRenameItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_PLAY_AND_QUEUE:
    return OnPlayAndQueueMedia(item);
  case CONTEXT_BUTTON_PLAY_ONLY_THIS:
    return OnPlayMedia(itemNumber);
  default:
    break;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowVideoBase::OnPlayMedia(int iItem, const std::string &player)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() )
    return false;

  CFileItemPtr pItem = m_vecItems->Get(iItem);

  // party mode
  if (g_partyModeManager.IsEnabled(PARTYMODECONTEXT_VIDEO))
  {
    CPlayList playlistTemp;
    playlistTemp.Add(pItem);
    g_partyModeManager.AddUserSongs(playlistTemp, true);
    return true;
  }

  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);

  CFileItem item(*pItem);
  if (pItem->IsVideoDb())
  {
    item.SetPath(pItem->GetVideoInfoTag()->m_strFileNameAndPath);
    item.SetProperty("original_listitem_url", pItem->GetPath());
  }
  CLog::Log(LOGDEBUG, "%s %s", __FUNCTION__, CURL::GetRedacted(item.GetPath()).c_str());


  //! @todo delete entire block in v18
  //! @deprecated m_strStreamURL is deprecated in v17
  if (item.IsPVR())
  {
    CPVRRecordingsPath path(item.GetPath());
    if (path.IsValid() && path.IsActive())
    {
      if (!g_PVRManager.IsStarted())
        return false;

      /* For recordings we check here for a available stream URL */
      CFileItemPtr tag = g_PVRRecordings->GetByPath(item.GetPath());
      if (tag && tag->HasPVRRecordingInfoTag() && !tag->GetPVRRecordingInfoTag()->m_strStreamURL.empty())
      {
        std::string stream = tag->GetPVRRecordingInfoTag()->m_strStreamURL;

        /* Isolate the folder from the filename */
        size_t found = stream.find_last_of("/");
        if (found == std::string::npos)
          found = stream.find_last_of("\\");

        if (found != std::string::npos)
        {
          /* Check here for asterix at the begin of the filename */
          if (stream[found+1] == '*')
          {
            /* Create a "stack://" url with all files matching the extension */
            std::string ext = URIUtils::GetExtension(stream);
            std::string dir = stream.substr(0, found).c_str();

            CFileItemList items;
            CDirectory::GetDirectory(dir, items);
            items.Sort(SortByFile, SortOrderAscending);

            std::vector<int> stack;
            for (int i = 0; i < items.Size(); ++i)
            {
              if (URIUtils::HasExtension(items[i]->GetPath(), ext))
                stack.push_back(i);
            }

            if (stack.size() > 0)
            {
              /* If we have a stack change the path of the item to it */
              CStackDirectory dir;
              std::string stackPath = dir.ConstructStackPath(items, stack);
              item.SetPath(stackPath);
            }
          }
          else
          {
            /* If no asterix is present play only the given stream URL */
            item.SetPath(stream);
          }
        }
        else
        {
          CLog::Log(LOGERROR, "CGUIWindowTV: Can't open recording, no valid filename!");
          CGUIDialogOK::ShowAndGetInput(CVariant{19033}, CVariant{19036});
          return false;
        }
      }
    }
  }

  PlayMovie(&item, player);

  return true;
}

bool CGUIWindowVideoBase::OnPlayAndQueueMedia(const CFileItemPtr &item, std::string player)
{
  // Get the current playlist and make sure it is not shuffled
  int iPlaylist = m_guiState->GetPlaylist();
  if (iPlaylist != PLAYLIST_NONE && g_playlistPlayer.IsShuffled(iPlaylist))
     g_playlistPlayer.SetShuffle(iPlaylist, false);

  CFileItemPtr movieItem(new CFileItem(*item));

  // Call the base method to actually queue the items
  // and start playing the given item
  return CGUIMediaWindow::OnPlayAndQueueMedia(movieItem, player);
}

void CGUIWindowVideoBase::PlayMovie(const CFileItem *item, const std::string &player)
{
  CFileItemPtr movieItem(new CFileItem(*item));

  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  playlist.Clear();
  playlist.Add(movieItem);

  if(m_thumbLoader.IsLoading())
    m_thumbLoader.StopAsync();

  // play movie...
  g_playlistPlayer.Play(0, player);

  if(!g_application.m_pPlayer->IsPlayingVideo())
    m_thumbLoader.Load(*m_vecItems);
}

void CGUIWindowVideoBase::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size())
    return;

  OnDeleteItem(m_vecItems->Get(iItem));

  Refresh(true);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowVideoBase::OnDeleteItem(CFileItemPtr item)
{
  // HACK: stacked files need to be treated as folders in order to be deleted
  if (item->IsStack())
    item->m_bIsFolder = true;
  if (CProfilesManager::GetInstance().GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE &&
      CProfilesManager::GetInstance().GetCurrentProfile().filesLocked())
  {
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;
  }

  if ((CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_ALLOWFILEDELETION) ||
       m_vecItems->IsPath("special://videoplaylists/")) &&
      CUtil::SupportsWriteFileOperations(item->GetPath()))
    CFileUtils::DeleteItem(item);
}

void CGUIWindowVideoBase::LoadPlayList(const std::string& strPlayList, int iPlayList /* = PLAYLIST_VIDEO */)
{
  // if partymode is active, we disable it
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Disable();

  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  std::unique_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if (pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(CVariant{6}, CVariant{477});
      return; //hmmm unable to load playlist?
    }
  }

  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, iPlayList))
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("playlistvideo://");
  }
}

void CGUIWindowVideoBase::PlayItem(int iItem, const std::string &player)
{
  // restrictions should be placed in the appropiate window code
  // only call the base code if the item passes since this clears
  // the currently playing temp playlist

  const CFileItemPtr pItem = m_vecItems->Get(iItem);
  // if its a folder, build a temp playlist
  if (pItem->m_bIsFolder && !pItem->IsPlugin())
  {
    // take a copy so we can alter the queue state
    CFileItemPtr item(new CFileItem(*m_vecItems->Get(iItem)));

    //  Allow queuing of unqueueable items
    //  when we try to queue them directly
    if (!item->CanQueue())
      item->SetCanQueue(true);

    // skip ".."
    if (item->IsParentFolder())
      return;

    // recursively add items to list
    CFileItemList queuedItems;
    AddItemToPlayList(item, queuedItems);

    g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_VIDEO, queuedItems);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
    g_playlistPlayer.Play();
  }
  else if (pItem->IsPlayList())
  {
    // load the playlist the old way
    LoadPlayList(pItem->GetPath(), PLAYLIST_VIDEO);
  }
  else
  {
    // single item, play it
    OnClick(iItem, player);
  }
}

bool CGUIWindowVideoBase::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory, updateFilterPath))
    return false;

  // might already be running from GetGroupedItems
  if (!m_thumbLoader.IsLoading())
    m_thumbLoader.Load(*m_vecItems);

  return true;
}

bool CGUIWindowVideoBase::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  bool bResult = CGUIMediaWindow::GetDirectory(strDirectory, items);

  // add in the "New Playlist" item if we're in the playlists folder
  if ((items.GetPath() == "special://videoplaylists/") && !items.Contains("newplaylist://"))
  {
    CFileItemPtr newPlaylist(new CFileItem(CProfilesManager::GetInstance().GetUserDataItem("PartyMode-Video.xsp"),false));
    newPlaylist->SetLabel(g_localizeStrings.Get(16035));
    newPlaylist->SetLabelPreformated(true);
    newPlaylist->m_bIsFolder = true;
    items.Add(newPlaylist);

/*    newPlaylist.reset(new CFileItem("newplaylist://", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(525));
    newPlaylist->SetLabelPreformated(true);
    items.Add(newPlaylist);
*/
    newPlaylist.reset(new CFileItem("newsmartplaylist://video", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(21437));  // "new smart playlist..."
    newPlaylist->SetLabelPreformated(true);
    items.Add(newPlaylist);
  }

  m_stackingAvailable = StackingAvailable(items);
  // we may also be in a tvshow files listing
  // (ideally this should be removed, and our stack regexps tidied up if necessary
  // No "normal" episodes should stack, and multi-parts should be supported)
  ADDON::ScraperPtr info = m_database.GetScraperForPath(strDirectory);
  if (info && info->Content() == CONTENT_TVSHOWS)
    m_stackingAvailable = false;

  if (m_stackingAvailable && !items.IsStack() && CSettings::GetInstance().GetBool(CSettings::SETTING_MYVIDEOS_STACKVIDEOS))
    items.Stack();

  return bResult;
}

bool CGUIWindowVideoBase::StackingAvailable(const CFileItemList &items)
{
  CURL url(items.GetPath());
  return !(items.IsPlugin() || items.IsAddonsPath()  ||
           items.IsRSS() || items.IsInternetStream() ||
           items.IsVideoDb() || url.IsProtocol("playlistvideo"));
}

void CGUIWindowVideoBase::GetGroupedItems(CFileItemList &items)
{
  CGUIMediaWindow::GetGroupedItems(items);

  std::string group;
  bool mixed = false;
  if (items.HasProperty(PROPERTY_GROUP_BY))
    group = items.GetProperty(PROPERTY_GROUP_BY).asString();
  if (items.HasProperty(PROPERTY_GROUP_MIXED))
    mixed = items.GetProperty(PROPERTY_GROUP_MIXED).asBoolean();

  // group == "none" completely supresses any grouping
  if (!StringUtils::EqualsNoCase(group, "none"))
  {
    CQueryParams params;
    CVideoDatabaseDirectory dir;
    dir.GetQueryParams(items.GetPath(), params);
    VIDEODATABASEDIRECTORY::NODE_TYPE nodeType = CVideoDatabaseDirectory::GetDirectoryChildType(m_strFilterPath);
    if (items.GetContent() == "movies" && params.GetSetId() <= 0 &&
        nodeType == NODE_TYPE_TITLE_MOVIES &&
       (CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOLIBRARY_GROUPMOVIESETS) || (StringUtils::EqualsNoCase(group, "sets") && mixed)))
    {
      CFileItemList groupedItems;
      GroupAttribute groupAttributes = CSettings::GetInstance().GetBool(CSettings::SETTING_VIDEOLIBRARY_GROUPSINGLEITEMSETS) ? GroupAttributeNone : GroupAttributeIgnoreSingleItems;
      if (GroupUtils::GroupAndMix(GroupBySet, m_strFilterPath, items, groupedItems, groupAttributes))
      {
        items.ClearItems();
        items.Append(groupedItems);
      }
    }
  }

  // reload thumbs after filtering and grouping
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  m_thumbLoader.Load(items);
}

bool CGUIWindowVideoBase::CheckFilterAdvanced(CFileItemList &items) const
{
  std::string content = items.GetContent();
  if ((items.IsVideoDb() || CanContainFilter(m_strFilterPath)) &&
      (StringUtils::EqualsNoCase(content, "movies")   ||
       StringUtils::EqualsNoCase(content, "tvshows")  ||
       StringUtils::EqualsNoCase(content, "episodes") ||
       StringUtils::EqualsNoCase(content, "musicvideos")))
    return true;

  return false;
}

bool CGUIWindowVideoBase::CanContainFilter(const std::string &strDirectory) const
{
  return URIUtils::IsProtocol(strDirectory, "videodb://");
}

void CGUIWindowVideoBase::AddToDatabase(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return;

  CFileItemPtr pItem = m_vecItems->Get(iItem);
  if (pItem->IsParentFolder() || pItem->m_bIsFolder)
    return;

  CVideoInfoTag movie;
  movie.Reset();

  // prompt for data
  // enter a new title
  std::string strTitle = pItem->GetLabel();
  if (!CGUIKeyboardFactory::ShowAndGetInput(strTitle, CVariant{g_localizeStrings.Get(528)}, false)) // Enter Title
    return;

  // pick genre
  CGUIDialogSelect* pSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!pSelect)
    return;

  pSelect->SetHeading(CVariant{530}); // Select Genre
  pSelect->Reset();
  CFileItemList items;
  if (!CDirectory::GetDirectory("videodb://movies/genres/", items))
    return;
  pSelect->SetItems(items);
  pSelect->EnableButton(true, 531); // New Genre
  pSelect->Open();
  std::string strGenre;
  int iSelected = pSelect->GetSelectedItem();
  if (iSelected >= 0)
    strGenre = items[iSelected]->GetLabel();
  else if (!pSelect->IsButtonPressed())
    return;

  // enter new genre string
  if (strGenre.empty())
  {
    strGenre = g_localizeStrings.Get(532); // Manual Addition
    if (!CGUIKeyboardFactory::ShowAndGetInput(strGenre, CVariant{g_localizeStrings.Get(533)}, false)) // Enter Genre
      return; // user backed out
    if (strGenre.empty())
      return; // no genre string
  }

  // set movie info
  movie.m_strTitle = strTitle;
  movie.m_genre = StringUtils::Split(strGenre, g_advancedSettings.m_videoItemSeparator);

  // everything is ok, so add to database
  m_database.Open();
  int idMovie = m_database.AddMovie(pItem->GetPath());
  movie.SetUniqueID(StringUtils::Format("xx%08i", idMovie));
  m_database.SetDetailsForMovie(pItem->GetPath(), movie, pItem->GetArt());
  m_database.Close();

  // done...
  CGUIDialogOK::ShowAndGetInput(CVariant{20177}, CVariant{movie.m_strTitle},
                                CVariant{StringUtils::Join(movie.m_genre, g_advancedSettings.m_videoItemSeparator)},
                                CVariant{movie.GetUniqueID()});

  // library view cache needs to be cleared
  CUtil::DeleteVideoDatabaseDirectoryCache();
}

/// \brief Search the current directory for a string got from the virtual keyboard
void CGUIWindowVideoBase::OnSearch()
{
  std::string strSearch;
  if (!CGUIKeyboardFactory::ShowAndGetInput(strSearch, CVariant{g_localizeStrings.Get(16017)}, false))
    return ;

  StringUtils::ToLower(strSearch);
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(CVariant{194});
    m_dlgProgress->SetLine(0, CVariant{strSearch});
    m_dlgProgress->SetLine(1, CVariant{""});
    m_dlgProgress->SetLine(2, CVariant{""});
    m_dlgProgress->Open();
    m_dlgProgress->Progress();
  }
  CFileItemList items;
  DoSearch(strSearch, items);

  if (m_dlgProgress)
    m_dlgProgress->Close();

  if (items.Size())
  {
    CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDlgSelect->Reset();
    pDlgSelect->SetHeading(CVariant{283});

    for (int i = 0; i < (int)items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      pDlgSelect->Add(pItem->GetLabel());
    }

    pDlgSelect->Open();

    int iItem = pDlgSelect->GetSelectedItem();
    if (iItem < 0)
      return;

    CFileItemPtr pSelItem = items[iItem];

    OnSearchItemFound(pSelItem.get());
  }
  else
  {
    CGUIDialogOK::ShowAndGetInput(CVariant{194}, CVariant{284});
  }
}

/// \brief React on the selected search item
/// \param pItem Search result item
void CGUIWindowVideoBase::OnSearchItemFound(const CFileItem* pSelItem)
{
  if (pSelItem->m_bIsFolder)
  {
    std::string strPath = pSelItem->GetPath();
    std::string strParentPath;
    URIUtils::GetParentPath(strPath, strParentPath);

    Update(strParentPath);

    if (pSelItem->IsVideoDb() && CSettings::GetInstance().GetBool(CSettings::SETTING_MYVIDEOS_FLATTEN))
      SetHistoryForPath("");
    else
      SetHistoryForPath(strParentPath);

    strPath = pSelItem->GetPath();
    CURL url(strPath);
    if (pSelItem->IsSmb() && !URIUtils::HasSlashAtEnd(strPath))
      strPath += "/";

    for (int i = 0; i < m_vecItems->Size(); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      if (pItem->GetPath() == strPath)
      {
        m_viewControl.SetSelectedItem(i);
        break;
      }
    }
  }
  else
  {
    std::string strPath = URIUtils::GetDirectory(pSelItem->GetPath());

    Update(strPath);

    if (pSelItem->IsVideoDb() && CSettings::GetInstance().GetBool(CSettings::SETTING_MYVIDEOS_FLATTEN))
      SetHistoryForPath("");
    else
      SetHistoryForPath(strPath);

    for (int i = 0; i < (int)m_vecItems->Size(); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      CURL url(pItem->GetPath());
      if (pSelItem->IsVideoDb())
        url.SetOptions("");
      if (url.Get() == pSelItem->GetPath())
      {
        m_viewControl.SetSelectedItem(i);
        break;
      }
    }
  }
  m_viewControl.SetFocused();
}

int CGUIWindowVideoBase::GetScraperForItem(CFileItem *item, ADDON::ScraperPtr &info, SScanSettings& settings)
{
  if (!item)
    return 0;

  if (m_vecItems->IsPlugin() || m_vecItems->IsRSS())
  {
    info.reset();
    return 0;
  }
  else if(m_vecItems->IsLiveTV())
  {
    info.reset();
    return 0;
  }

  bool foundDirectly = false;
  info = m_database.GetScraperForPath(item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_strPath.empty() ? std::string(item->GetVideoInfoTag()->m_strPath) : item->GetPath(), settings, foundDirectly);
  return foundDirectly ? 1 : 0;
}

void CGUIWindowVideoBase::OnScan(const std::string& strPath, bool scanAll)
{
    g_application.StartVideoScan(strPath, true, scanAll);
}

std::string CGUIWindowVideoBase::GetStartFolder(const std::string &dir)
{
  std::string lower(dir); StringUtils::ToLower(lower);
  if (lower == "$playlists" || lower == "playlists")
    return "special://videoplaylists/";
  else if (lower == "plugins" || lower == "addons")
    return "addons://sources/video/";
  return CGUIMediaWindow::GetStartFolder(dir);
}

void CGUIWindowVideoBase::AppendAndClearSearchItems(CFileItemList &searchItems, const std::string &prependLabel, CFileItemList &results)
{
  if (!searchItems.Size())
    return;

  searchItems.Sort(SortByLabel, SortOrderAscending, CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
  for (int i = 0; i < searchItems.Size(); i++)
    searchItems[i]->SetLabel(prependLabel + searchItems[i]->GetLabel());
  results.Append(searchItems);

  searchItems.Clear();
}

bool CGUIWindowVideoBase::OnUnAssignContent(const std::string &path, int header, int text)
{
  bool bCanceled;
  CVideoDatabase db;
  db.Open();
  if (CGUIDialogYesNo::ShowAndGetInput(CVariant{header}, CVariant{text}, bCanceled, CVariant{ "" }, CVariant{ "" }, CGUIDialogYesNo::NO_TIMEOUT))
  {
    CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    db.RemoveContentForPath(path, progress);
    db.Close();
    CUtil::DeleteVideoDatabaseDirectoryCache();
    return true;
  }
  else
  {
    if (!bCanceled)
    {
      ADDON::ScraperPtr info;
      SScanSettings settings;
      settings.exclude = true;
      db.SetScraperForPath(path,info,settings);
    }
  }
  db.Close();
  
  return false;
}

void CGUIWindowVideoBase::OnAssignContent(const std::string &path)
{
  bool bScan=false;
  CVideoDatabase db;
  db.Open();

  SScanSettings settings;
  ADDON::ScraperPtr info = db.GetScraperForPath(path, settings);

  ADDON::ScraperPtr info2(info);
  
  if (CGUIDialogContentSettings::Show(info, settings))
  {
    if(settings.exclude || (!info && info2))
    {
      OnUnAssignContent(path, 20375, 20340);
    }
    else if (info != info2)
    {
      if (OnUnAssignContent(path, 20442, 20443))
        bScan = true;
    }
    db.SetScraperForPath(path, info, settings);
  }

  if (bScan)
  {
    g_application.StartVideoScan(path, true, true);
  }
}
