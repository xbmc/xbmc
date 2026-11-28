/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoGUIUtils.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "PartyMode.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayLists.h"
#include "application/ApplicationPlayer.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/DiscDirectoryHelper.h"
#include "filesystem/LibraryPaths.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory/DirectoryNode.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "music/MusicFileItemClassify.h"
#include "playlists/PlayListEntryRules.h"
#include "playlists/PlayListFileItemClassify.h"
#include "profiles/ProfileManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/IRunnable.h"
#include "utils/FileUtils.h"
#include "utils/ItemProperties.h"
#include "utils/PlaceholderPaths.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"
#include "video/VideoLibrary.h"
#include "video/VideoUtils.h"
#include "view/GUIViewState.h"

#include <optional>

using KODI::MEDIA::MediaSection;

namespace KODI
{

namespace
{
class CAsyncGetItemsForPlaylist : public IRunnable, private PLAYLIST::IEntryRules
{
public:
  CAsyncGetItemsForPlaylist(const std::shared_ptr<CFileItem>& item,
                            CFileItemList& queuedItems,
                            ContentUtils::PlayMode mode,
                            const std::shared_ptr<CFileItem>& startAt)
    : m_item(item),
      m_resume((item->GetStartOffset() == STARTOFFSET_RESUME) &&
               VIDEO::UTILS::GetItemResumeInformation(*item).isResumable),
      m_queuedItems(queuedItems),
      m_mode(mode),
      m_startAt(startAt)
  {
  }

  ~CAsyncGetItemsForPlaylist() override = default;

  void Run() override
  {
    m_startPosition =
        CApplicationPlayLists::ExpandToEntries(m_item, *this, m_startAt, m_queuedItems);
  }

  int GetStartPosition() const { return m_startPosition.value_or(-1); }

private:
  std::shared_ptr<CFileItem> Redirect(const std::shared_ptr<CFileItem>& folder) override;
  bool IsUnlocked(CFileItem& source) override;
  void Arrange(const CFileItem& folder,
               CFileItemList& items,
               std::shared_ptr<CFileItem>& startAt) override;
  std::shared_ptr<CFileItem> Accept(const std::shared_ptr<CFileItem>& file,
                                    const CFileItemList& entries) override;

  const std::shared_ptr<CFileItem> m_item;
  const bool m_resume{false};
  CFileItemList& m_queuedItems;
  const ContentUtils::PlayMode m_mode{ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM};
  const std::shared_ptr<CFileItem> m_startAt;
  std::optional<int> m_startPosition;
};

SortDescription GetSortDescription(const CGUIViewState& state, const CFileItemList& items)
{
  SortDescription sortDescDate;

  auto sortDescriptions = state.GetSortDescriptions();
  for (auto& sortDescription : sortDescriptions)
  {
    if (sortDescription.sortBy == SortBy::EPISODE_NUMBER)
    {
      // check whether at least one item has actually an episode number set
      for (const auto& item : items)
      {
        if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iEpisode > 0)
        {
          // first choice for folders containing episodes
          sortDescription.sortOrder = SortOrder::ASCENDING;
          return sortDescription;
        }
      }
      continue;
    }
    else if (sortDescription.sortBy == SortBy::YEAR)
    {
      // check whether at least one item has actually a year set
      for (const auto& item : items)
      {
        if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->HasYear())
        {
          // first choice for folders containing movies
          sortDescription.sortOrder = SortOrder::ASCENDING;
          return sortDescription;
        }
      }
    }
    else if (sortDescription.sortBy == SortBy::DATE)
    {
      // check whether at least one item has actually a valid date set
      for (const auto& item : items)
      {
        if (item->GetDateTime().IsValid())
        {
          // fallback, if neither ByEpisode nor ByYear is available
          sortDescDate = sortDescription;
          sortDescDate.sortOrder = SortOrder::ASCENDING;
          break; // leave items loop. we can still find ByEpisode or ByYear. so, no return here.
        }
      }
    }
  }

  if (sortDescDate.sortBy != SortBy::NONE)
    return sortDescDate;
  else
    return state.GetSortMethod(); // last resort
}

std::shared_ptr<CFileItem> CAsyncGetItemsForPlaylist::Redirect(const std::shared_ptr<CFileItem>& folder)
{
  if (folder->IsPlugin())
    return folder;

  // a folder with dvd or bluray files plays the relevant file
  const std::string mediapath = VIDEO::UTILS::GetOpticalMediaPath(*folder);
  if (mediapath.empty())
    return folder;
  return std::make_shared<CFileItem>(mediapath, false);
}

bool CAsyncGetItemsForPlaylist::IsUnlocked(CFileItem& source)
{
  return source.IsPVR() || g_passwordManager.IsItemUnlocked(&source, MediaSection::VIDEO);
}

void CAsyncGetItemsForPlaylist::Arrange(const CFileItem& folder,
                                        CFileItemList& items,
                                        std::shared_ptr<CFileItem>& startAt)
{
  int viewStateWindowId = WINDOW_VIDEO_NAV;
  if (URIUtils::IsPVRRadioRecordingFileOrFolder(folder.GetPath()))
    viewStateWindowId = WINDOW_RADIO_RECORDINGS;
  else if (URIUtils::IsPVRTVRecordingFileOrFolder(folder.GetPath()))
    viewStateWindowId = WINDOW_TV_RECORDINGS;

  const std::unique_ptr<CGUIViewState> state(CGUIViewState::GetViewState(viewStateWindowId, items));
  if (state)
  {
    LABEL_MASKS labelMasks;
    state->GetSortMethodLabelMasks(labelMasks);
    CLabelFormatter::FormatItemLabels(items, labelMasks);

    SortDescription sortDesc;
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == viewStateWindowId)
    {
      sortDesc = state->GetSortMethod();

      // It makes no sense to play from younger to older, except "play from here"
      // mode where order of listing has to be kept.
      if (m_mode != ContentUtils::PlayMode::PLAY_FROM_HERE &&
          (sortDesc.sortBy == SortBy::DATE || sortDesc.sortBy == SortBy::YEAR ||
           sortDesc.sortBy == SortBy::EPISODE_NUMBER))
        sortDesc.sortOrder = SortOrder::ASCENDING;
    }
    else
      sortDesc = GetSortDescription(*state, items);

    if (sortDesc.sortBy == SortBy::LABEL)
      items.ClearSortState();

    items.Sort(sortDesc);
  }

  bool fetchedPlayCounts = false;
  if (items.GetContent().empty() && !VIDEO::IsVideoDb(items) && !items.IsVirtualDirectoryRoot() &&
      !items.IsSourcesPath() && !items.IsLibraryFolder())
  {
    CVideoDatabase db;
    if (db.Open())
    {
      std::string content = db.GetContentForPath(items.GetPath());
      if (content.empty() && !items.IsPlugin())
        content = "files";

      items.SetContent(content);

      // Get play counts and resume bookmarks for the items.
      db.GetPlayCounts(items.GetPath(), items);
      fetchedPlayCounts = true;
    }
  }

  if (m_resume)
  {
    // start at the last played item; add start offsets for videos
    std::shared_ptr<CFileItem> lastPlayedItem;
    CDateTime lastPlayed;
    for (const auto& i : items)
    {
      if (!i->HasVideoInfoTag())
        continue;

      const auto videoTag = i->GetVideoInfoTag();

      const CBookmark& bookmark = videoTag->GetResumePoint();
      if (bookmark.IsSet())
      {
        i->SetStartOffset(CUtil::ConvertSecsToMilliSecs(bookmark.timeInSeconds));

        const CDateTime& currLastPlayed = videoTag->m_lastPlayed;
        if (currLastPlayed.IsValid() && (!lastPlayed.IsValid() || (lastPlayed < currLastPlayed)))
        {
          lastPlayedItem = i;
          lastPlayed = currLastPlayed;
        }
      }
    }

    if (lastPlayedItem && !startAt)
      startAt = lastPlayedItem;
  }

  WatchedMode watchedMode;
  if (m_resume)
    watchedMode = WatchedMode::UNWATCHED;
  else
    watchedMode = CMediaSettings::GetInstance().GetWatchedMode(items.GetContent());

  const bool unwatchedOnly = watchedMode == WatchedMode::UNWATCHED;
  const bool watchedOnly = watchedMode == WatchedMode::WATCHED;
  for (int n = 0; n < items.Size();)
  {
    const auto& i = items[n];
    bool keep = true;
    if (i->IsFolder())
    {
      std::string path = i->GetPath();
      URIUtils::RemoveSlashAtEnd(path);
      keep = !StringUtils::EndsWithNoCase(path, "sample"); // skip sample folders
    }
    else
    {
      if (!fetchedPlayCounts && (!i->HasVideoInfoTag() || !i->GetVideoInfoTag()->IsPlayCountSet()))
      {
        fetchedPlayCounts = VIDEO::LIBRARY::WithDatabase(
            [&items](CVideoDatabase& db)
            {
              db.GetPlayCounts(items.GetPath(), items);
              return true;
            });
      }
      if (i->HasVideoInfoTag() && i->GetVideoInfoTag()->IsPlayCountSet())
      {
        const int playCount = i->GetVideoInfoTag()->GetPlayCount();
        keep = !((unwatchedOnly && playCount > 0) || (watchedOnly && playCount <= 0));
      }
    }
    if (keep)
      ++n;
    else
      items.Remove(n);
  }
}

std::shared_ptr<CFileItem> CAsyncGetItemsForPlaylist::Accept(const std::shared_ptr<CFileItem>& file,
                                                             const CFileItemList& entries)
{
  if (VIDEO::IsVideoDb(*file))
  {
    // this case is needed unless we allow IsVideo() to return true for videodb items,
    // but then we have issues with playlists of videodb items
    const auto itemCopy = std::make_shared<CFileItem>(*file->GetVideoInfoTag());
    itemCopy->SetStartOffset(file->GetStartOffset());
    return itemCopy;
  }
  if (PLAYLIST::CanBeEntry(*file) && VIDEO::IsVideo(*file))
    return file;
  return nullptr;
}

std::string GetVideoDbItemPath(const CFileItem& item)
{
  std::string path = item.GetPath();
  if (!URIUtils::IsVideoDb(path))
    path = item.GetProperty(ITEM::PROPERTY::ORIGINAL_LISTITEM_URL).asString();

  if (URIUtils::IsVideoDb(path))
    return path;

  return {};
}

void AddItemToPlayListAndPlay(const std::shared_ptr<CFileItem>& itemToQueue,
                              const std::shared_ptr<CFileItem>& itemToPlay,
                              const std::string& player,
                              ContentUtils::PlayMode mode)
{
  CFileItemList queuedItems;
  int start = -1;
  VIDEO::UTILS::GetItemsForPlayList(itemToQueue, queuedItems, mode, itemToPlay, &start);

  // The enumeration's watched filter can drop the chosen item.
  if (itemToPlay && start < 0)
  {
    queuedItems.AddFront(itemToPlay, 0);
    start = 0;
  }

  CServiceBroker::GetPlayLists()->PlayExpanded(PLAYLIST::Video, queuedItems, start, itemToPlay,
                                               {.player = player});
}

} // unnamed namespace

} // namespace KODI

namespace KODI::VIDEO::UTILS
{
void PlayItem(const std::shared_ptr<CFileItem>& item,
              const std::string& player,
              ContentUtils::PlayMode mode /* = ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM */)
{
  if (item->IsFolder() && !item->IsPlugin())
  {
    AddItemToPlayListAndPlay(item, nullptr, player, mode);
  }
  else if (VIDEO::IsVideo(*item))
  {
    if (mode == ContentUtils::PlayMode::PLAY_FROM_HERE ||
        (mode == ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM && IsAutoPlayNextItem(*item)))
    {
      // Add item and all its siblings to the playlist and play. Prefer videodb path if available,
      // because it provides more information than just a plain file system path for example.
      std::string parentPath = item->GetProperty(ITEM::PROPERTY::PARENT_PATH).asString();
      if (parentPath.empty())
      {
        std::string path = GetVideoDbItemPath(*item);
        if (path.empty())
          path = item->GetPath();

        URIUtils::GetParentPath(path, parentPath);

        if (parentPath.empty())
        {
          CLog::LogF(LOGERROR, "Unable to obtain parent path for '{}'", item->GetPath());
          return;
        }
      }

      const auto parentItem = std::make_shared<CFileItem>(parentPath, true);
      parentItem->SetProperty(ITEM::PROPERTY::IS_VIDEO_FOLDER, true);
      parentItem->LoadDetails();
      if (item->GetStartOffset() == STARTOFFSET_RESUME)
        parentItem->SetStartOffset(STARTOFFSET_RESUME);

      AddItemToPlayListAndPlay(parentItem, item, player, mode);
    }
    else // mode == PlayMode::PLAY_ONLY_THIS
    {
      // single item, play it
      CServiceBroker::GetPlayLists()->PlayItem(PLAYLIST::Video, item, {.player = player});
    }
  }
  else
  {
    CLog::LogF(LOGERROR, "Unable to play item {}", item->GetPath());
  }
}

void QueueItem(const std::shared_ptr<CFileItem>& item, QueuePosition pos)
{
  const auto playLists = CServiceBroker::GetPlayLists();

  // Determine the proper list to queue this element
  const PLAYLIST::Type type = playLists->GetQueueType(PLAYLIST::Video);

  CFileItemList queuedItems;
  GetItemsForPlayList(item, queuedItems, ContentUtils::PlayMode::CHECK_AUTO_PLAY_NEXT_ITEM);

  playLists->Queue(type, queuedItems,
                   pos == QueuePosition::POSITION_BEGIN ? CApplicationPlayLists::Placement::Next
                                                        : CApplicationPlayLists::Placement::End);

  // Note: video does not auto play on queue like music
}

bool GetItemsForPlayList(const std::shared_ptr<CFileItem>& item,
                         CFileItemList& queuedItems,
                         ContentUtils::PlayMode mode,
                         const std::shared_ptr<CFileItem>& startAt /* = nullptr */,
                         int* startPosition /* = nullptr */)
{
  CAsyncGetItemsForPlaylist getItems(item, queuedItems, mode, startAt);
  const bool done = CGUIDialogBusy::Wait(&getItems,
                                         500, // 500ms before busy dialog appears
                                         true); // can be cancelled
  if (startPosition)
    *startPosition = getItems.GetStartPosition();
  return done;
}

namespace
{
bool IsNonExistingUserPartyModePlaylist(const CFileItem& item)
{
  if (!PLAYLIST::IsSmartPlayList(item))
    return false;

  const std::string& path{item.GetPath()};
  return path == PARTYMODE::RulesPath(PLAYLIST::Video) && !CFileUtils::Exists(path);
}

bool IsEmptyVideoItem(const CFileItem& item)
{
  return item.HasVideoInfoTag() && item.GetVideoInfoTag()->IsEmpty();
}
} // unnamed namespace

bool IsItemPlayable(const CFileItem& item)
{
  if (item.IsParentFolder())
    return false;

  if (item.IsDeleted())
    return false;

  // Include all PVR recordings and recordings folders
  if (URIUtils::IsPVRRecordingFileOrFolder(item.GetPath()))
    return true;

  // Include Live TV
  if (!item.IsFolder() && (item.IsLiveTV() || item.IsEPG()))
    return true;

  // Exclude all music library items
  if (MUSIC::IsMusicDb(item) ||
      StringUtils::StartsWithNoCase(item.GetPath(), MEDIA::LIBRARY_PATH::MUSIC))
    return false;

  // Exclude add-ons
  if (item.IsAddonsPath())
    return false;

  // Exclude special items
  if (KODI::ITEM::PLACEHOLDER::IsNewItem(item.GetPath()))
    return false;

  // Include playlists located at one of the possible video/mixed playlist locations
  if (PLAYLIST::IsPlayList(item))
  {
    if (StringUtils::StartsWithNoCase(item.GetMimeType(), "video/"))
      return true;

    if (CUtil::IsInPlaylistsFolder(item.GetPath(), MediaSection::VIDEO))
      return true;

    if (!item.IsFolder() && !item.HasVideoInfoTag())
    {
      // Unknown location. Type cannot be determined for non-folder items.
      return false;
    }
  }

  if (IsNonExistingUserPartyModePlaylist(item))
    return false;

  if (item.IsFolder() && (IsVideoDb(item) || StringUtils::StartsWithNoCase(
                                                 item.GetPath(), MEDIA::LIBRARY_PATH::VIDEO)))
  {
    // Exclude top level nodes - eg can't play 'genres' just a specific genre etc
    const auto node = XFILE::CVideoDatabaseDirectory::GetDirectoryParentType(item.GetPath());
    if (node == XFILE::VIDEODATABASEDIRECTORY::NodeType::OVERVIEW ||
        node == XFILE::VIDEODATABASEDIRECTORY::NodeType::MOVIES_OVERVIEW ||
        node == XFILE::VIDEODATABASEDIRECTORY::NodeType::TVSHOWS_OVERVIEW ||
        node == XFILE::VIDEODATABASEDIRECTORY::NodeType::MUSICVIDEOS_OVERVIEW)
      return false;

    return true;
  }

  if (item.IsPlugin() && IsVideo(item) && !IsEmptyVideoItem(item) &&
      item.GetProperty(ITEM::PROPERTY::IS_PLAYABLE).asBoolean(false))
  {
    return true;
  }
  else if (item.HasVideoInfoTag() && item.CanQueue() && !item.IsPlugin() && !item.IsScript())
  {
    return true;
  }
  else if ((!item.IsFolder() && IsVideo(item) && !IsEmptyVideoItem(item)) || item.IsDVD() ||
           MUSIC::IsCDDA(item))
  {
    return true;
  }
  else if (item.IsFolder() && !item.IsPlugin() && !item.IsScript())
  {
    // Not a video-specific folder (like file:// or nfs://). Allow play if context is Video window.
    if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VIDEO_NAV &&
        item.GetPath() != KODI::ITEM::PLACEHOLDER::ADD_SOURCE) // Exclude "Add video source" item
      return true;
  }

  return false;
}

std::string GetResumeString(const CFileItem& item)
{
  const ResumeInformation resumeInfo = GetItemResumeInformation(item);
  if (resumeInfo.isResumable)
  {
    return GetResumeString(resumeInfo.startOffset, resumeInfo.partNumber);
  }
  else
  {
    return {};
  }
}

std::string GetResumeString(int64_t startOffset, unsigned int partNumber)
{
  std::string resumeString;
  if (startOffset > 0)
  {
    resumeString =
        StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(12022),
                            StringUtils::SecondsToTimeString(
                                static_cast<long>(CUtil::ConvertMilliSecsToSecsInt(startOffset)),
                                TIME_FORMAT_HH_MM_SS)); // Resume from ##:##:##
  }
  else
  {
    if (partNumber > 0)
      resumeString =
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(12023); // Resume from
    else
      resumeString = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
          13362); // Continue watching
  }
  if (partNumber > 0)
  {
    const std::string partString{
        StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(23051),
                            partNumber)}; // Part #
    resumeString += startOffset > 0 ? " (" + partString + ")" : " " + partString;
  }
  return resumeString;
}

bool ChooseDiscPlaylist(CFileItem& item)
{
  item.SetProperty(ITEM::PROPERTY::FORCE_PLAYLIST_SELECTION, true);

  CFileItemList items;
  const bool chosen{XFILE::CDiscDirectoryHelper::GetOrShowPlaylistSelection(
                        item, items, XFILE::MenuDecision::SHOW_SIMPLE_MENU,
                        /* forPlayback */ false) &&
                    !items.IsEmpty()};
  if (chosen)
    item = *items[0];

  // The flag is copied, so clear it as choice made
  item.ClearProperty(ITEM::PROPERTY::FORCE_PLAYLIST_SELECTION);

  return chosen;
}

void NotifyItemPathChanged(const CFileItem& item, const std::string& oldPath, int oldFileId)
{
  CFileItem oldItem{item};
  oldItem.SetPath(oldPath);
  if (oldFileId > 0 && item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_iFileId != oldFileId)
    oldItem.SetProperty(ITEM::PROPERTY::REPLACED_FILE_ID, oldFileId);
  CGUIMessage msg{GUI_MSG_NOTIFY_ALL,
                  0,
                  0,
                  GUI_MSG_UPDATE_ITEM,
                  GUI_MSG_FLAG_FORCE_UPDATE,
                  std::make_shared<CFileItem>(oldItem)};
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void MakeVideoSettingsDefault(const std::function<void(CVideoSettings&)>& forget)
{
  const std::shared_ptr<CProfileManager> profileManager{
      CServiceBroker::GetSettingsComponent()->GetProfileManager()};
  if (profileManager->GetMasterProfile().getLockMode() != LockMode::EVERYONE &&
      !g_passwordManager.CheckSettingLevelLock(::SettingLevel::Expert))
    return;

  if (!CGUIDialogYesNo::ShowAndGetInput(CVariant{12376}, CVariant{12377}))
    return;

  if (!LIBRARY::WithDatabase(
          [](CVideoDatabase& db)
          {
            db.EraseAllVideoSettings();
            return true;
          }))
    return;

  const auto appPlayer{CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>()};
  CVideoSettings& defaults{CMediaSettings::GetInstance().GetDefaultVideoSettings()};
  defaults = appPlayer->GetVideoSettings();
  forget(defaults);
  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
}

} // namespace KODI::VIDEO::UTILS
