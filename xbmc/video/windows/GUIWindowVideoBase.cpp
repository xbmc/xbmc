/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowVideoBase.h"

#include "Autorun.h"
#include "ContextMenuManager.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/gui/GUIDialogAddonInfo.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "network/NetworkFileItemClassify.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListFactory.h"
#include "playlists/PlayListFileItemClassify.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/dialogs/GUIDialogContentSettings.h"
#include "storage/MediaManager.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/GroupUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoLibraryQueue.h"
#include "video/VideoManagerTypes.h"
#include "video/VideoUtils.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/guilib/VideoGUIUtils.h"
#include "video/guilib/VideoPlayActionProcessor.h"
#include "video/guilib/VideoSelectActionProcessor.h"
#include "video/guilib/VideoVersionHelper.h"
#include "view/GUIViewState.h"

#include <memory>

using namespace XFILE;
using namespace VIDEODATABASEDIRECTORY;
using namespace ADDON;
using namespace KODI;
using namespace PVR;
using namespace KODI::MESSAGING;

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

CGUIWindowVideoBase::~CGUIWindowVideoBase() = default;

bool CGUIWindowVideoBase::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SCAN_ITEM)
    return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_SCAN);
  else if (action.GetID() == ACTION_SHOW_PLAYLIST)
  {
    if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST::Id::TYPE_VIDEO ||
        CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST::Id::TYPE_VIDEO).size() > 0)
    {
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_PLAYLIST);
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
      m_dlgProgress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
      return CGUIMediaWindow::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
#if defined(HAS_OPTICAL_DRIVE)
      if (iControl == CONTROL_PLAY_DVD)
      {
        // play movie...
        MEDIA_DETECT::CAutorun::PlayDiscAskResume(
            CServiceBroker::GetMediaManager().TranslateDevicePath(""));
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
        else if (iAction == ACTION_QUEUE_ITEM_NEXT)
        {
          OnQueueItem(iItem, true);
          return true;
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          return OnItemInfo(iItem);
        }
        else if (iAction == ACTION_PLAYER_PLAY)
        {
          const auto& components = CServiceBroker::GetAppComponents();
          const auto appPlayer = components.GetComponent<CApplicationPlayer>();
          // if playback is paused or playback speed != 1, return
          if (appPlayer->IsPlayingVideo())
          {
            if (appPlayer->IsPausedPlayback())
              return false;
            if (appPlayer->GetPlaySpeed() != 1)
              return false;
          }

          // not playing video, or playback speed == 1
          return OnPlayOrResumeItem(iItem);
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

          // is delete allowed?
          if (profileManager->GetCurrentProfile().canWriteDatabases())
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

bool CGUIWindowVideoBase::OnItemInfo(const CFileItem& fileItem)
{
  if (fileItem.IsParentFolder() || fileItem.m_bIsShareOrDrive || fileItem.IsPath("add") ||
      (PLAYLIST::IsPlayList(fileItem) && !URIUtils::HasExtension(fileItem.GetDynPath(), ".strm")))
    return false;

  // "Videos/Video Add-ons" lists addons in the video window
  if (fileItem.HasAddonInfo())
    return CGUIDialogAddonInfo::ShowForItem(std::make_shared<CFileItem>(fileItem));

  // Video version
  if (fileItem.HasVideoInfoTag() && fileItem.GetVideoInfoTag()->m_type == MediaTypeVideoVersion)
    return false;

  // Movie set
  if (fileItem.m_bIsFolder && VIDEO::IsVideoDb(fileItem) &&
      fileItem.GetPath() != "videodb://movies/sets/" &&
      StringUtils::StartsWith(fileItem.GetPath(), "videodb://movies/sets/"))
    return ShowInfoAndRefresh(std::make_shared<CFileItem>(fileItem), nullptr);

  // Music video. Match visibility test of CMusicInfo::IsVisible
  if (VIDEO::IsVideoDb(fileItem) && fileItem.HasVideoInfoTag() &&
      (fileItem.HasProperty("artist_musicid") || fileItem.HasProperty("album_musicid")))
  {
    CGUIDialogMusicInfo::ShowFor(std::make_shared<CFileItem>(fileItem).get());
    return true;
  }

  std::string strDir;
  if (VIDEO::IsVideoDb(fileItem) && fileItem.HasVideoInfoTag() &&
      !fileItem.GetVideoInfoTag()->m_strPath.empty())
    strDir = fileItem.GetVideoInfoTag()->m_strPath;
  else
    strDir = URIUtils::GetDirectory(fileItem.GetPath());

  m_database.Open();

  VIDEO::SScanSettings settings;
  bool foundDirectly = false;
  const ADDON::ScraperPtr scraper = m_database.GetScraperForPath(strDir, settings, foundDirectly);

  if (!fileItem.HasVideoInfoTag() && !scraper && !(fileItem.IsPlugin() || fileItem.IsScript()) &&
      !KODI::VIDEO::UTILS::HasItemVideoDbInformation(fileItem))
  {
    // We have no chance to fill a video info tag, neither scraper nor db data available.
    HELPERS::ShowOKDialogText(CVariant{20176}, // Show video information
                              CVariant{19055}); // no information available
    m_database.Close();
    return false;
  }

  m_database.Close();

  if (scraper && scraper->Content() == CONTENT_TVSHOWS && foundDirectly &&
      !settings.parent_name_root) // dont lookup on root tvshow folder
    return true;

  CFileItem item(fileItem);
  if ((VIDEO::IsVideoDb(item) && item.HasVideoInfoTag()) ||
      (item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_iDbId != -1))
  {
    if (item.GetVideoInfoTag()->m_type == MediaTypeSeason)
    { // clear out the art - we're really grabbing the info on the show here
      item.ClearArt();
      item.GetVideoInfoTag()->m_iDbId = item.GetVideoInfoTag()->m_iIdShow;
    }
  }
  else
  {
    if (item.m_bIsFolder && scraper && scraper->Content() != CONTENT_TVSHOWS)
    {
      CFileItemList items;
      const std::string fileExts = CServiceBroker::GetFileExtensionProvider().GetVideoExtensions();
      CDirectory::GetDirectory(item.GetPath(), items, fileExts, DIR_FLAG_DEFAULTS);

      // Check for cases 1_dir/1_dir/.../file (e.g. by packages where have a extra folder)
      while (items.Size() == 1 && items[0]->m_bIsFolder &&
             VIDEO::UTILS::GetOpticalMediaPath(*items[0]).empty())
      {
        const std::string path = items[0]->GetPath();
        items.Clear();
        CDirectory::GetDirectory(path, items, fileExts, DIR_FLAG_DEFAULTS);
      }

      items.Stack();

      // check for media files
      bool bFoundFile(false);
      const std::vector<std::string>& excludeFromScan = CServiceBroker::GetSettingsComponent()
                                                            ->GetAdvancedSettings()
                                                            ->m_moviesExcludeFromScanRegExps;
      for (const auto& i : items)
      {
        if (VIDEO::IsVideo(*i) && !PLAYLIST::IsPlayList(*i) &&
            !CUtil::ExcludeFileOrFolder(i->GetPath(), excludeFromScan))
        {
          item.SetPath(i->GetPath());
          item.m_bIsFolder = false;
          bFoundFile = true;
          break;
        }
      }

      // no video file in this folder
      if (!bFoundFile)
      {
        HELPERS::ShowOKDialogText(CVariant{13346}, CVariant{20349});
        return false;
      }
    }
  }

  // we need to also request any thumbs be applied to the folder item
  if (fileItem.m_bIsFolder)
    item.SetProperty("set_folder_thumb", fileItem.GetPath());

  return ShowInfoAndRefresh(std::make_shared<CFileItem>(item), scraper);
}

// ShowInfo is called as follows:
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

bool CGUIWindowVideoBase::ShowInfo(const CFileItemPtr& item2, const ScraperPtr& info2)
{
  CGUIDialogVideoInfo* pDlgInfo = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogVideoInfo>(WINDOW_DIALOG_VIDEO_INFO);
  if (!pDlgInfo)
    return false;

  const ScraperPtr& info(info2); // use this as nfo might change it..
  CFileItemPtr item(item2); // we might replace item..

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
      const int versionId{item->HasVideoInfoTag() ? item->GetVideoInfoTag()->GetAssetInfo().GetId()
                                                  : -1};
      bHasInfo = m_database.GetMovieInfo(item->GetPath(), movieDetails, dbId, versionId);
    }
    if (info->Content() == CONTENT_TVSHOWS)
    {
      if (item->m_bIsFolder)
      {
        const CVideoInfoTag* videoTag = item->GetVideoInfoTag();
        if (videoTag && videoTag->m_type == MediaTypeSeason && videoTag->m_iSeason != -1)
          bHasInfo = m_database.GetSeasonInfo(videoTag->m_iIdSeason, movieDetails);
        if (!bHasInfo)
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
            CLog::Log(LOGERROR, "{}: could not add episode [{}]. tvshow does not exist yet..",
                      __FUNCTION__, item->GetPath());
            return false;
          }
        }
      }
    }
    if (info->Content() == CONTENT_MUSICVIDEOS)
    {
      bHasInfo = m_database.GetMusicVideoInfo(item->GetDynPath(), movieDetails);
    }
    m_database.Close();
  }
  else if (item->HasVideoInfoTag() && !item->GetVideoInfoTag()->IsEmpty())
  {
    bHasInfo = true;
    movieDetails = *item->GetVideoInfoTag();
  }

  bool needsRefresh = false;
  if (bHasInfo)
  {
    // @todo add support to refresh movie version information
    if (!info || info->Content() == CONTENT_NONE || VIDEO::IsVideoAssetFile(*item))
      item->SetProperty("xxuniqueid", "xx" + movieDetails.GetUniqueID()); // disable refresh button
    item->SetProperty("CheckAutoPlayNextItem", IsActive());
    *item->GetVideoInfoTag() = movieDetails;
    pDlgInfo->SetMovie(item.get());
    pDlgInfo->Open();
    if (pDlgInfo->HasUpdatedUserrating())
      return true;
    needsRefresh = pDlgInfo->NeedRefresh();
    if (!needsRefresh)
      return (pDlgInfo->HasUpdatedThumb() || pDlgInfo->HasUpdatedItems());
    // check if the item in the video info dialog has changed and if so, get the new item
    else if (pDlgInfo->GetCurrentListItem() != NULL)
    {
      item = pDlgInfo->GetCurrentListItem();

      if (VIDEO::IsVideoDb(*item) && item->HasVideoInfoTag())
        item->SetPath(item->GetVideoInfoTag()->GetPath());
      else if (!VIDEO::IsVideoDb(*item) && item->m_bIsFolder)
      {
        // Info on folder containing a movie needs dyn path as path for refresh with correct name
        //! @todo get rid of "videos with versions as folder" hack to be able to fix in CFileItem::GetBaseMoviePath()
        item->SetPath(item->GetVideoInfoTag()->GetPath());
      }
    }
  }

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  // quietly return if Internet lookups are disabled
  if (!profileManager->GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser)
    return false;

  if (!info)
    return false;

  if (CVideoLibraryQueue::GetInstance().IsScanningLibrary())
  {
    HELPERS::ShowOKDialogText(CVariant{13346}, CVariant{14057});
    return false;
  }

  bool listNeedsUpdating = false;
  // 3. Run a loop so that if we Refresh we re-run this block
  do
  {
    // reload images
    //! !todo we need this to update images in video info dialog immediatly after refresh, but why?
    CGUIMessage reload(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
    OnMessage(reload);

    if (!CVideoLibraryQueue::GetInstance().RefreshItemModal(item, needsRefresh,
                                                            pDlgInfo->RefreshAll()))
      break;

    // remove directory caches and reload images
    CUtil::DeleteVideoDatabaseDirectoryCache();
    OnMessage(reload);

    pDlgInfo->SetMovie(item.get());
    pDlgInfo->Open();
    item->SetArt("thumb", pDlgInfo->GetThumbnail());
    needsRefresh = pDlgInfo->NeedRefresh();
    if (needsRefresh && pDlgInfo->GetCurrentListItem() != nullptr)
    {
      item = pDlgInfo->GetCurrentListItem();

      if (VIDEO::IsVideoDb(*item) && item->HasVideoInfoTag())
        item->SetPath(item->GetVideoInfoTag()->GetPath());
    }
    listNeedsUpdating = true;
  } while (needsRefresh);

  return listNeedsUpdating;
}

bool CGUIWindowVideoBase::ShowInfoAndRefresh(const CFileItemPtr& item, const ScraperPtr& info)
{
  const int ret{ShowInfo(item, info)};

  // Test IsActive() since we can be called from other windows (music, home) we need this check
  if (ret && IsActive())
  {
    const int itemNumber{m_viewControl.GetSelectedItem()};
    Refresh();
    m_viewControl.SetSelectedItem(itemNumber);
  }

  return ret;
}

void CGUIWindowVideoBase::OnQueueItem(int iItem, bool first)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return;

  OnQueueItem(m_vecItems->Get(iItem), iItem, first);
}

void CGUIWindowVideoBase::OnQueueItem(const std::shared_ptr<CFileItem>& item, int iItem, bool first)
{
  // don't re-queue items from playlist window
  if (GetID() == WINDOW_VIDEO_PLAYLIST)
    return;

  if (item->IsRAR() || item->IsZIP())
    return;

  VIDEO::UTILS::QueueItem(item, first ? VIDEO::UTILS::QueuePosition::POSITION_BEGIN
                                      : VIDEO::UTILS::QueuePosition::POSITION_END);

  // select next item
  m_viewControl.SetSelectedItem(iItem + 1);
}

bool CGUIWindowVideoBase::OnSelect(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  const std::shared_ptr<CFileItem> item = m_vecItems->Get(iItem);

  const std::string path = item->GetPath();
  if (!item->m_bIsFolder && path != "add" &&
      ((!StringUtils::StartsWith(path, "newsmartplaylist://") &&
        !StringUtils::StartsWith(path, "newplaylist://") &&
        !StringUtils::StartsWith(path, "newtag://") &&
        !StringUtils::StartsWith(path, "script://") &&
        !StringUtils::StartsWith(path, "plugin://")) ||
       (StringUtils::StartsWith(path, "plugin://") &&
        item->GetProperty("IsPlayable").asBoolean(false))))
    return OnFileAction(
        iItem, VIDEO::GUILIB::CVideoSelectActionProcessorBase::GetDefaultSelectAction(), "");

  return CGUIMediaWindow::OnSelect(iItem);
}

namespace
{
class CVideoSelectActionProcessor : public VIDEO::GUILIB::CVideoSelectActionProcessorBase
{
public:
  CVideoSelectActionProcessor(CGUIWindowVideoBase& window,
                              const std::shared_ptr<CFileItem>& item,
                              int itemIndex,
                              const std::string& player)
    : CVideoSelectActionProcessorBase(item),
      m_window(window),
      m_itemIndex(itemIndex),
      m_player(player)
  {
  }

protected:
  bool OnPlayPartSelected(unsigned int part) override
  {
    return m_window.OnPlayStackPart(m_item, part);
  }

  bool OnResumeSelected() override
  {
    m_item->SetStartOffset(STARTOFFSET_RESUME);
    return m_window.PlayItem(m_item, m_player);
  }

  bool OnPlaySelected() override
  {
    m_item->SetStartOffset(0);
    return m_window.PlayItem(m_item, m_player);
  }

  bool OnQueueSelected() override
  {
    m_window.OnQueueItem(m_item, m_itemIndex);
    return true;
  }

  bool OnInfoSelected() override { return m_window.OnItemInfo(*m_item); }

  bool OnChooseSelected() override
  {
    // window only shows the default version, so no window specific context menu items available
    if (m_item->HasVideoVersions() && !m_item->GetVideoInfoTag()->IsDefaultVideoVersion())
      return CONTEXTMENU::ShowFor(m_item, CContextMenuManager::MAIN);

    m_window.OnPopupMenu(m_itemIndex);
    return true;
  }

private:
  CGUIWindowVideoBase& m_window;
  const int m_itemIndex{-1};
  const std::string m_player;
};
} // namespace

bool CGUIWindowVideoBase::OnFileAction(int iItem,
                                       VIDEO::GUILIB::Action action,
                                       const std::string& player)
{
  const std::shared_ptr<CFileItem> item = m_vecItems->Get(iItem);
  if (!item)
    return false;

  CVideoSelectActionProcessor proc(*this, item, iItem, player);
  return proc.ProcessAction(action);
}

bool CGUIWindowVideoBase::OnItemInfo(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  return OnItemInfo(*m_vecItems->Get(iItem));
}

void CGUIWindowVideoBase::OnRestartItem(int iItem, const std::string &player)
{
  CGUIMediaWindow::OnClick(iItem, player);
}

void CGUIWindowVideoBase::LoadVideoInfo(CFileItemList& items,
                                        CVideoDatabase& database,
                                        bool allowReplaceLabels)
{
  //! @todo this could possibly be threaded as per the music info loading,
  //!       we could also cache the info
  if (!items.GetContent().empty() && !items.IsPlugin())
    return; // don't load for listings that have content set and weren't created from plugins

  std::string content = items.GetContent();
  // determine content only if it isn't set
  if (content.empty())
  {
    content = database.GetContentForPath(items.GetPath());
    items.SetContent((content.empty() && !items.IsPlugin()) ? "files" : content);
  }

  /*
    If we have a matching item in the library, so we can assign the metadata to it. In addition, we can choose
    * whether the item is stacked down (eg in the case of folders representing a single item)
    * whether or not we assign the library's labels to the item, or leave the item as is.

    As certain users (read: certain developers) don't want either of these to occur, we compromise by stacking
    items down only if stacking is available and enabled.

    Similarly, we assign the "clean" library labels to the item only if the "Replace filenames with library titles"
    setting is enabled.
    */
  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const bool stackItems =
      items.GetProperty("isstacked").asBoolean() ||
      (StackingAvailable(items) && settings->GetBool(CSettings::SETTING_MYVIDEOS_STACKVIDEOS));
  const bool replaceLabels =
      allowReplaceLabels && settings->GetBool(CSettings::SETTING_MYVIDEOS_REPLACELABELS);

  CFileItemList dbItems;
  /* NOTE: In the future when GetItemsForPath returns all items regardless of whether they're "in the library"
           we won't need the fetchedPlayCounts code, and can "simply" do this directly on absence of content. */
  bool fetchedPlayCounts = false;
  if (!content.empty())
  {
    database.GetItemsForPath(content, items.GetPath(), dbItems);
    dbItems.SetFastLookup(true);
  }

  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr pItem = items[i];
    CFileItemPtr match;

    if (pItem->m_bIsFolder && !pItem->IsParentFolder())
    {
      // we need this for enabling the right context menu entries, like mark watched / unwatched
      pItem->SetProperty("IsVideoFolder", true);
    }

    if (!content
             .empty()) /* optical media will be stacked down, so it's path won't match the base path */
    {
      std::string pathToMatch =
          pItem->IsOpticalMediaFile() ? pItem->GetLocalMetadataPath() : pItem->GetPath();
      if (URIUtils::IsMultiPath(pathToMatch))
        pathToMatch = CMultiPathDirectory::GetFirstPath(pathToMatch);
      match = dbItems.Get(pathToMatch);
    }
    if (match)
    {
      pItem->UpdateInfo(*match, replaceLabels);

      if (stackItems)
      {
        if (match->m_bIsFolder)
          pItem->SetPath(match->GetVideoInfoTag()->m_strPath);
        else
          pItem->SetPath(match->GetVideoInfoTag()->m_strFileNameAndPath);
        // if we switch from a file to a folder item it means we really shouldn't be sorting files and
        // folders separately
        if (pItem->m_bIsFolder != match->m_bIsFolder)
        {
          items.SetSortIgnoreFolders(true);
          pItem->m_bIsFolder = match->m_bIsFolder;
        }
      }
    }
    else
    {
      /* NOTE: Currently we GetPlayCounts on our items regardless of whether content is set
                as if content is set, GetItemsForPaths doesn't return anything not in the content tables.
                This code can be removed once the content tables are always filled */
      if (!pItem->m_bIsFolder && !fetchedPlayCounts)
      {
        database.GetPlayCounts(items.GetPath(), items);
        fetchedPlayCounts = true;
      }

      // set the watched overlay
      if (VIDEO::IsVideo(*pItem))
        pItem->SetOverlayImage(pItem->HasVideoInfoTag() &&
                                       pItem->GetVideoInfoTag()->GetPlayCount() > 0
                                   ? CGUIListItem::ICON_OVERLAY_WATCHED
                                   : CGUIListItem::ICON_OVERLAY_UNWATCHED);
    }
  }
}

namespace
{
class CVideoPlayActionProcessor : public VIDEO::GUILIB::CVideoPlayActionProcessorBase
{
public:
  CVideoPlayActionProcessor(CGUIWindowVideoBase& window,
                            const std::shared_ptr<CFileItem>& item,
                            const std::string& player)
    : CVideoPlayActionProcessorBase(item), m_window(window), m_player(player)
  {
  }

protected:
  bool OnResumeSelected() override
  {
    m_item->SetStartOffset(STARTOFFSET_RESUME);
    return m_window.PlayItem(m_item, m_player);
  }

  bool OnPlaySelected() override
  {
    m_item->SetStartOffset(0);
    return m_window.PlayItem(m_item, m_player);
  }

private:
  CGUIWindowVideoBase& m_window;
  const std::string m_player;
};
} // namespace

bool CGUIWindowVideoBase::OnPlayOrResumeItem(int iItem, const std::string& player)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  CVideoPlayActionProcessor proc{*this, m_vecItems->Get(iItem), player};
  return proc.ProcessDefaultAction();
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
      if (VIDEO::IsVideoDb(*item) && item->HasVideoInfoTag())
        path = item->GetVideoInfoTag()->m_strFileNameAndPath;

      if (!item->IsPath("add") && !item->IsPlugin() &&
          !item->IsScript() && !item->IsAddonsPath() && !item->IsLiveTV())
      {
        if (URIUtils::IsStack(path))
        {
          std::vector<uint64_t> times;
          if (m_database.GetStackTimes(path, times) || URIUtils::IsDiscImageStack(path))
            buttons.Add(CONTEXT_BUTTON_PLAY_PART, 20324);
        }
      }

      if (PLAYLIST::IsSmartPlayList(*item))
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_PARTYMODE, 15216); // Play in Partymode
      }

      // if the item isn't a folder or script, is not explicitly marked as not playable,
      // is a member of a list rather than a single item and we're not on the last element of the list,
      // then add either 'play from here' or 'play only this' depending on default behaviour
      if (!(item->m_bIsFolder || item->IsScript()) &&
          (!item->HasProperty("IsPlayable") || item->GetProperty("IsPlayable").asBoolean()) &&
          m_vecItems->Size() > 1 && itemNumber < m_vecItems->Size() - 1)
      {
        if (VIDEO::UTILS::IsAutoPlayNextItem(*item))
          buttons.Add(CONTEXT_BUTTON_PLAY_ONLY_THIS, 13434);
        else
          buttons.Add(CONTEXT_BUTTON_PLAY_AND_QUEUE, 13412);
      }
      if (PLAYLIST::IsSmartPlayList(*item) || PLAYLIST::IsSmartPlayList(*m_vecItems))
        buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);
      if (VIDEO::IsBlurayPlaylist(*item))
        buttons.Add(CONTEXT_BUTTON_CHOOSE_PLAYLIST, 13424);
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowVideoBase::OnPlayStackPart(const std::shared_ptr<CFileItem>& item,
                                          unsigned int partNumber)
{
  // part numbers are 1-based.
  if (partNumber < 1)
    return false;

  const std::string path = item->GetDynPath();

  if (!URIUtils::IsStack(path))
    return false;

  if (URIUtils::IsDiscImageStack(path))
  {
    // disc image stack
    CFileItemList parts;
    CDirectory::GetDirectory(path, parts, "", DIR_FLAG_DEFAULTS);

    const int value = CVideoSelectActionProcessor::ChoosePlayOrResume(*parts[partNumber - 1]);
    if (value == VIDEO::GUILIB::ACTION_RESUME)
    {
      const VIDEO::UTILS::ResumeInformation resumeInfo =
          VIDEO::UTILS::GetItemResumeInformation(*parts[partNumber - 1]);
      item->SetStartOffset(resumeInfo.startOffset);
    }
    else if (value != VIDEO::GUILIB::ACTION_PLAY_FROM_BEGINNING)
      return false; // if not selected PLAY, then we changed our mind so return

    item->m_lStartPartNumber = partNumber;
  }
  else
  {
    // video file stack
    if (partNumber > 1)
    {
      std::vector<uint64_t> times;
      if (m_database.GetStackTimes(path, times))
        item->SetStartOffset(times[partNumber - 1]);
    }
    else
    {
      item->SetStartOffset(0);
    }
  }
  // play the video
  return PlayItem(item, "");
}

bool CGUIWindowVideoBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  m_forceSelection = false;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  switch (button)
  {
  case CONTEXT_BUTTON_SET_CONTENT:
    {
      OnAssignContent(item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_strPath.empty() ? item->GetVideoInfoTag()->m_strPath : item->GetPath());
      return true;
    }
  case CONTEXT_BUTTON_PLAY_PART:
    {
      return OnFileAction(itemNumber, VIDEO::GUILIB::ACTION_PLAYPART, "");
    }

  case CONTEXT_BUTTON_PLAY_PARTYMODE:
    g_partyModeManager.Enable(PARTYMODECONTEXT_VIDEO, m_vecItems->Get(itemNumber)->GetPath());
    return true;

  case CONTEXT_BUTTON_SCAN:
    if (!item)
      return false;

    if (VIDEO::IsVideoDb(*item) &&
        (!item->m_bIsFolder || item->GetVideoInfoTag()->m_strPath.empty()))
      return false;

    if (item->m_bIsFolder)
    {
      const std::string strPath =
          VIDEO::IsVideoDb(*item) ? item->GetVideoInfoTag()->m_strPath : item->GetPath();
      OnScan(strPath, true);
    }
    else
      OnItemInfo(*item);

    return true;

  case CONTEXT_BUTTON_DELETE:
    OnDeleteItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_EDIT_SMART_PLAYLIST:
    {
      const std::string playlist =
          PLAYLIST::IsSmartPlayList(*m_vecItems->Get(itemNumber))
              ? m_vecItems->Get(itemNumber)->GetPath()
              : m_vecItems->GetPath(); // save path as activatewindow will destroy our items
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
  case CONTEXT_BUTTON_CHOOSE_PLAYLIST:
  {
    m_forceSelection = true;
    return OnPlayMedia(itemNumber);
  }
  default:
    break;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowVideoBase::OnPlayMedia(const std::shared_ptr<CFileItem>& pItem,
                                      const std::string& player)
{
  // party mode
  if (g_partyModeManager.IsEnabled(PARTYMODECONTEXT_VIDEO))
  {
    PLAYLIST::CPlayList playlistTemp;
    playlistTemp.Add(pItem);
    g_partyModeManager.AddUserSongs(playlistTemp, true);
    return true;
  }

  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  CServiceBroker::GetPlaylistPlayer().Reset();
  CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::Id::TYPE_NONE);

  auto itemCopy = std::make_shared<CFileItem>(*pItem);

  if (VIDEO::IsVideoDb(*pItem))
  {
    itemCopy->SetPath(pItem->GetVideoInfoTag()->m_strFileNameAndPath);
    itemCopy->SetProperty("original_listitem_url", pItem->GetPath());
  }
  CLog::Log(LOGDEBUG, "{} {}", __FUNCTION__, CURL::GetRedacted(itemCopy->GetPath()));

  itemCopy->SetProperty("playlist_type_hint", static_cast<int>(m_guiState->GetPlaylist()));

  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopAsync();

  CServiceBroker::GetPlaylistPlayer().Play(itemCopy, player, m_forceSelection);

  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (!appPlayer->IsPlayingVideo())
    m_thumbLoader.Load(*m_vecItems);

  return true;
}

bool CGUIWindowVideoBase::OnPlayMedia(int iItem, const std::string& player)
{
  if (iItem < 0 || iItem >= m_vecItems->Size())
    return false;

  return OnPlayMedia(m_vecItems->Get(iItem), player);
}

bool CGUIWindowVideoBase::OnPlayAndQueueMedia(const CFileItemPtr& item, const std::string& player)
{
  // Get the current playlist and make sure it is not shuffled
  PLAYLIST::Id playlistId = m_guiState->GetPlaylist();
  if (playlistId != PLAYLIST::Id::TYPE_NONE &&
      CServiceBroker::GetPlaylistPlayer().IsShuffled(playlistId))
  {
    CServiceBroker::GetPlaylistPlayer().SetShuffle(playlistId, false);
  }

  CFileItemPtr movieItem(new CFileItem(*item));

  // Call the base method to actually queue the items
  // and start playing the given item
  return CGUIMediaWindow::OnPlayAndQueueMedia(movieItem, player);
}

void CGUIWindowVideoBase::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size())
    return;

  OnDeleteItem(m_vecItems->Get(iItem));

  Refresh(true);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowVideoBase::OnDeleteItem(const CFileItemPtr& item)
{
  // HACK: stacked files need to be treated as folders in order to be deleted
  if (item->IsStack())
    item->m_bIsFolder = true;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  if (profileManager->GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE &&
      profileManager->GetCurrentProfile().filesLocked())
  {
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;
  }

  if ((CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_ALLOWFILEDELETION) ||
       m_vecItems->IsPath("special://videoplaylists/")) &&
      CUtil::SupportsWriteFileOperations(item->GetPath()))
  {
    CGUIComponent *gui = CServiceBroker::GetGUI();
    if (gui && gui->ConfirmDelete(item->GetPath()))
      CFileUtils::DeleteItem(item);
  }
}

void CGUIWindowVideoBase::LoadPlayList(const std::string& strPlayList,
                                       PLAYLIST::Id playlistId /* = PLAYLIST::TYPE_VIDEO */)
{
  // if partymode is active, we disable it
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Disable();

  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  std::unique_ptr<PLAYLIST::CPlayList> pPlayList(PLAYLIST::CPlayListFactory::Create(strPlayList));
  if (pPlayList)
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      HELPERS::ShowOKDialogText(CVariant{6}, CVariant{477});
      return; //hmmm unable to load playlist?
    }
  }

  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, playlistId))
  {
    if (m_guiState)
      m_guiState->SetPlaylistDirectory("playlistvideo://");
  }
}

bool CGUIWindowVideoBase::PlayItem(const std::shared_ptr<CFileItem>& pItem,
                                   const std::string& player)
{
  if (!pItem->m_bIsFolder && VIDEO::IsVideoDb(*pItem) && !pItem->Exists())
  {
    CLog::LogF(LOGDEBUG, "File '{}' for library item '{}' doesn't exist.", pItem->GetDynPath(),
               pItem->GetPath());

    const auto profileManager{CServiceBroker::GetSettingsComponent()->GetProfileManager()};

    if (profileManager->GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser)
    {
      if (CGUIDialogVideoInfo::DeleteVideoItemFromDatabase(pItem, true))
      {
        int itemIndex{0};
        const std::string path{pItem->GetPath()};
        for (int i = 0; i < m_vecItems->Size(); ++i)
        {
          if (m_vecItems->Get(i)->GetPath() == path)
          {
            itemIndex = i;
            break;
          }
        }

        // update list
        Refresh(true);
        m_viewControl.SetSelectedItem(itemIndex);
      }
    }
    else
    {
      HELPERS::ShowOKDialogText(CVariant{257}, // Error
                                CVariant{662}); // This file is no longer available.
    }
    return true;
  }

  m_forceSelection = false;

  //! @todo get rid of "videos with versions as folder" hack!
  if (pItem->m_bIsFolder && !pItem->IsPlugin() &&
      !(pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->IsDefaultVideoVersion()))
  {
    // take a copy so we can alter the queue state
    const auto item{std::make_shared<CFileItem>(*pItem)};

    //  Allow queuing of unqueueable items
    //  when we try to queue them directly
    if (!item->CanQueue())
      item->SetCanQueue(true);

    // recursively add items to list
    CFileItemList queuedItems;
    VIDEO::UTILS::GetItemsForPlayList(item, queuedItems);

    CServiceBroker::GetPlaylistPlayer().ClearPlaylist(PLAYLIST::Id::TYPE_VIDEO);
    CServiceBroker::GetPlaylistPlayer().Reset();
    CServiceBroker::GetPlaylistPlayer().Add(PLAYLIST::Id::TYPE_VIDEO, queuedItems);
    CServiceBroker::GetPlaylistPlayer().SetCurrentPlaylist(PLAYLIST::Id::TYPE_VIDEO);
    CServiceBroker::GetPlaylistPlayer().Play();
    return true;
  }
  else if (PLAYLIST::IsPlayList(*pItem) && !pItem->IsType(".strm"))
  {
    // Note: strm files being somehow special playlists need to be handled in OnPlay*Media

    // load the playlist the old way
    LoadPlayList(pItem->GetDynPath(), PLAYLIST::Id::TYPE_VIDEO);
    return true;
  }
  else if (m_guiState.get() && m_guiState->AutoPlayNextItem() && !g_partyModeManager.IsEnabled())
    return OnPlayAndQueueMedia(pItem, player);
  else
    return OnPlayMedia(pItem, player);
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

  UpdateVideoVersionItems();

  UpdateVideoVersionItemsLabel(strDirectory);

  return true;
}

bool CGUIWindowVideoBase::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  bool bResult = CGUIMediaWindow::GetDirectory(strDirectory, items);

  // add in the "New Playlist" item if we're in the playlists folder
  if ((items.GetPath() == "special://videoplaylists/") && !items.Contains("newplaylist://"))
  {
    const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

    CFileItemPtr newPlaylist(new CFileItem(profileManager->GetUserDataItem("PartyMode-Video.xsp"),false));
    newPlaylist->SetLabel(g_localizeStrings.Get(16035));
    newPlaylist->SetLabelPreformatted(true);
    newPlaylist->SetArt("icon", "DefaultPartyMode.png");
    newPlaylist->m_bIsFolder = true;
    items.Add(newPlaylist);

/*    newPlaylist.reset(new CFileItem("newplaylist://", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(525));
    newPlaylist->SetLabelPreformatted(true);
    items.Add(newPlaylist);
*/
    newPlaylist = std::make_shared<CFileItem>("newsmartplaylist://video", false);
    newPlaylist->SetLabel(g_localizeStrings.Get(21437));  // "new smart playlist..."
    newPlaylist->SetArt("icon", "DefaultAddSource.png");
    newPlaylist->SetLabelPreformatted(true);
    items.Add(newPlaylist);
  }

  m_stackingAvailable = StackingAvailable(items);
  // we may also be in a tvshow files listing
  // (ideally this should be removed, and our stack regexps tidied up if necessary
  // No "normal" episodes should stack, and multi-parts should be supported)
  ADDON::ScraperPtr info = m_database.GetScraperForPath(strDirectory);
  if (info && info->Content() == CONTENT_TVSHOWS)
    m_stackingAvailable = false;

  if (m_stackingAvailable && !items.IsStack() && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MYVIDEOS_STACKVIDEOS))
    items.Stack();

  return bResult;
}

bool CGUIWindowVideoBase::StackingAvailable(const CFileItemList &items)
{
  CURL url(items.GetPath());
  return !(items.IsPlugin() || items.IsAddonsPath() || items.IsRSS() ||
           NETWORK::IsInternetStream(items) || VIDEO::IsVideoDb(items) ||
           url.IsProtocol("playlistvideo"));
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

  // group == "none" completely suppresses any grouping
  if (!StringUtils::EqualsNoCase(group, "none"))
  {
    CQueryParams params;
    CVideoDatabaseDirectory dir;
    dir.GetQueryParams(items.GetPath(), params);
    VIDEODATABASEDIRECTORY::NODE_TYPE nodeType = CVideoDatabaseDirectory::GetDirectoryChildType(m_strFilterPath);
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (items.GetContent() == "movies" && params.GetSetId() <= 0 &&
        params.GetVideoVersionId() < 0 && nodeType == NODE_TYPE_TITLE_MOVIES &&
        (settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_GROUPMOVIESETS) ||
         (StringUtils::EqualsNoCase(group, "sets") && mixed)))
    {
      CFileItemList groupedItems;
      GroupAttribute groupAttributes = settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_GROUPSINGLEITEMSETS) ? GroupAttributeNone : GroupAttributeIgnoreSingleItems;
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
  const std::string& content = items.GetContent();
  if ((VIDEO::IsVideoDb(items) || CanContainFilter(m_strFilterPath)) &&
      (StringUtils::EqualsNoCase(content, "movies") ||
       StringUtils::EqualsNoCase(content, "tvshows") ||
       StringUtils::EqualsNoCase(content, "episodes") ||
       StringUtils::EqualsNoCase(content, "musicvideos")))
    return true;

  return false;
}

bool CGUIWindowVideoBase::CanContainFilter(const std::string &strDirectory) const
{
  return URIUtils::IsProtocol(strDirectory, "videodb://");
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
    CGUIDialogSelect* pDlgSelect = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
    pDlgSelect->Reset();
    pDlgSelect->SetHeading(CVariant{283});

    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      pDlgSelect->Add(pItem->GetLabel());
    }

    pDlgSelect->Open();

    int iItem = pDlgSelect->GetSelectedItem();
    if (iItem < 0)
      return;

    OnSearchItemFound(items[iItem].get());
  }
  else
  {
    HELPERS::ShowOKDialogText(CVariant{194}, CVariant{284});
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

    if (VIDEO::IsVideoDb(*pSelItem) &&
        CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MYVIDEOS_FLATTEN))
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

    if (VIDEO::IsVideoDb(*pSelItem) &&
        CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
            CSettings::SETTING_MYVIDEOS_FLATTEN))
      SetHistoryForPath("");
    else
      SetHistoryForPath(strPath);

    for (int i = 0; i < m_vecItems->Size(); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      CURL url(pItem->GetPath());
      if (VIDEO::IsVideoDb(*pSelItem))
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

int CGUIWindowVideoBase::GetScraperForItem(CFileItem* item,
                                           ADDON::ScraperPtr& info,
                                           VIDEO::SScanSettings& settings)
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
  info = m_database.GetScraperForPath(item->HasVideoInfoTag() && !item->GetVideoInfoTag()->m_strPath.empty() ? item->GetVideoInfoTag()->m_strPath : item->GetPath(), settings, foundDirectly);
  return foundDirectly ? 1 : 0;
}

void CGUIWindowVideoBase::OnScan(const std::string& strPath, bool scanAll)
{
  CVideoLibraryQueue::GetInstance().ScanLibrary(strPath, scanAll, true);
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

  searchItems.Sort(SortByLabel, SortOrderAscending, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
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
    CGUIDialogProgress *progress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
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
      VIDEO::SScanSettings settings;
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

  VIDEO::SScanSettings settings;
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
    CVideoLibraryQueue::GetInstance().ScanLibrary(path, true, true);
  }
}

void CGUIWindowVideoBase::UpdateVideoVersionItems()
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_VIDEOLIBRARY_SHOWVIDEOVERSIONSASFOLDER))
    return;

  for (const auto& item : *m_vecItems)
  {
    if (item->m_bIsFolder || !item->HasVideoInfoTag())
      continue;

    MediaType type = item->GetVideoInfoTag()->m_type;
    if (type == MediaTypeVideoVersion)
    {
      continue;
    }
    else if (type == MediaTypeMovie)
    {
      //! @todo patching the items after loading is a hack, which only works in the video window,
      //! not for example for home screen widgets!

      int videoVersionId{-1};
      if (VIDEO::IsVideoDb(*item) && item->GetVideoInfoTag()->HasVideoVersions())
      {
        if (item->GetProperty("has_resolved_video_asset").asBoolean(false))
        {
          // certain version of the movie
          videoVersionId = item->GetVideoInfoTag()->GetAssetInfo().GetId();
        }
        else
        {
          // movie node representing all versions of this movie
          videoVersionId = VIDEO_VERSION_ID_ALL;
          item->GetVideoInfoTag()->GetAssetInfo().SetId(videoVersionId);
          item->m_bIsFolder = true;
        }

        CVideoDbUrl itemUrl;
        itemUrl.FromString(
            StringUtils::Format("videodb://movies/videoversions/{}", videoVersionId));
        itemUrl.AddOption("mediaid", item->GetVideoInfoTag()->m_iDbId);
        item->SetPath(itemUrl.ToString());
      }
    }
  }
}

void CGUIWindowVideoBase::UpdateVideoVersionItemsLabel(const std::string& directory)
{
  bool isVersionsFolderView{false};

  CVideoDbUrl videoUrl;
  if (videoUrl.FromString(directory) && videoUrl.HasOption("videoversionid"))
  {
    CVariant value;
    if (videoUrl.GetOption("videoversionid", value))
    {
      const int idVideoVersion{static_cast<int>(value.asInteger(-1))};
      if (idVideoVersion == VIDEO_VERSION_ID_ALL && videoUrl.GetOption("mediaid", value))
      {
        const int idMedia{static_cast<int>(value.asInteger(-1))};
        if (idMedia != -1)
        {
          // adjust breadcrumb to display the correct movie title
          m_vecItems->SetLabel(
              m_database.GetVideoItemTitle(m_vecItems->GetVideoContentType(), idMedia));
          isVersionsFolderView = true;
        }
      }
    }
  }

  SetProperty("VideoVersionsFolderView", isVersionsFolderView);
}
