/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationPlayLists.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "filesystem/PlaylistFileDirectory.h"
#include "music/MusicFileItemClassify.h"
#include "network/NetworkFileItemClassify.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListEntryRules.h"
#include "playlists/PlayListFactory.h"
#include "playlists/PlayListFile.h"
#include "playlists/PlayListFileItemClassify.h"
#include "playlists/PlayListShuffle.h"
#include "pvr/channels/PVRChannel.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ItemProperties.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <mutex>
#include <set>
#include <utility>

using namespace KODI;
using namespace KODI::MESSAGING;
using namespace KODI::PLAYLIST;
using KODI::APPLICATION::StartsRun;

namespace
{
//! How many entries a fed playlist keeps placed ahead of the current one.
constexpr int FEED_DEPTH = 10;
//! How deep playlist files inside playlist files are opened; the limit stops two that hold each
//! other.
constexpr int EXPANSION_DEPTH = 5;

/*!
 * \brief Replace a playlist file's entry with the file's entries; a game's list of discs stays
 * whole.
 * \return The first of the new entries, or NO_ENTRY if nothing was expanded.
 */
EntryId ExpandEntry(CPlayList& playList, EntryId entry)
{
  const std::shared_ptr<CFileItem> item = playList.GetItem(entry);
  // a game's list of discs goes to the player whole
  if (!item || item->HasGameInfoTag())
    return NO_ENTRY;

  // the factory fills in a stream's mime type, so it is given a copy
  const CFileItem probe(*item);
  std::unique_ptr<CPlayListFile> playlist(CPlayListFactory::Create(probe));
  if (playlist == nullptr)
    return NO_ENTRY;

  std::string path = item->GetDynPath();

  if (!playlist->Load(path))
    return NO_ENTRY;

  std::vector<std::shared_ptr<CFileItem>> expansion;
  for (const std::shared_ptr<CFileItem>& loaded : playlist->GetItems())
  {
    // an entry pointing back at the playlist would expand for ever
    if (StringUtils::EqualsNoCase(loaded->GetPath(), path))
      continue;

    // What plays is still the entry that was expanded, such as a radio station's .pls, so each
    // stream keeps that entry's path as its identity and plays from its own.
    loaded->SetDynPath(loaded->GetPath());
    loaded->SetPath(item->GetDynPath());
    // Only propagate parent's start offset if the loaded item doesn't already
    // have its own (e.g. a CUE sheet offset loaded from the playlist file).
    if (!loaded->HasProperty(ITEM::PROPERTY::ITEM_START))
      loaded->SetStartOffset(item->GetStartOffset());
    if (!loaded->HasProperty("BasePath"))
      loaded->SetProperty("BasePath", playlist->GetBasePath());
    expansion.push_back(loaded);
  }

  // the entry is found again, since other edits may have moved it while the file was read
  return playList.ReplaceEntry(entry, item, expansion);
}

size_t Index(Type type)
{
  return type == PLAYLIST::Video ? 0 : 1;
}

} // namespace

CApplicationPlayLists::CApplicationPlayLists(std::unique_ptr<IPlayback> playback)
  : m_playback(std::move(playback)),
    m_currentItem(std::make_shared<CFileItem>()),
    m_failedEntriesStart(std::chrono::steady_clock::now())
{
  for (const Type type : {PLAYLIST::Video, PLAYLIST::Audio})
  {
    auto& playList = m_playLists[Index(type)];
    playList = std::make_unique<CPlayList>();
    playList->SetObserver([this, type](const std::vector<PlayListChange>& changes)
                          { OnPlayListChanged(type, changes); });
  }
}

CApplicationPlayLists::~CApplicationPlayLists() = default;

CPlayList& CApplicationPlayLists::EditPlayList(Type type)
{
  return *m_playLists[Index(type)];
}

const CPlayList& CApplicationPlayLists::GetPlayList(Type type) const
{
  return *m_playLists[Index(type)];
}

void CApplicationPlayLists::OnPlayListChanged(Type type, const std::vector<PlayListChange>& changes)
{
  bool shuffled = false;
  bool repeatMayHaveChanged = false;
  bool refresh = false;
  bool topUp = false;
  bool feedChanged = false;
  IObserver* observer = m_observer;
  for (const auto& change : changes)
  {
    if (observer)
      observer->OnListChanged(type, change);
    switch (change.type)
    {
      case PlayListChange::Type::Shuffled:
        shuffled = true;
        refresh = true;
        break;
      case PlayListChange::Type::Current:
        repeatMayHaveChanged = true;
        topUp = true;
        break;
      case PlayListChange::Type::Repeat:
      case PlayListChange::Type::Wrap:
        repeatMayHaveChanged = true;
        break;
      case PlayListChange::Type::Cleared:
        repeatMayHaveChanged = true;
        refresh = true;
        break;
      case PlayListChange::Type::Removed:
        refresh = true;
        topUp = true;
        break;
      case PlayListChange::Type::Feed:
        feedChanged = true;
        break;
      default:
        refresh = true;
        break;
    }
  }

  if (feedChanged && !GetPlayList(type).GetFeed())
    RestoreSavedPlayOrder(type);

  if (feedChanged && observer && !m_composingFeed)
    observer->OnFeed(GetFedType().has_value());

  if (topUp)
    TopUp(type);

  if (shuffled && observer)
    observer->OnShuffled(type, IsShuffled(type));

  if (repeatMayHaveChanged && !m_composingRepeat && ReportRepeat(type))
    refresh = true;

  if (refresh)
    ReportPlayListsChanged();
}

bool CApplicationPlayLists::ReportRepeat(Type type)
{
  // the repeat state is composed from two settings of the playlist, so it is reported when the
  // composition changes, not on each of the two steps that set it
  const Repeat repeat = GetRepeat(type);
  {
    std::unique_lock lock(m_critSection);
    if (std::exchange(m_reportedRepeat[Index(type)], repeat) == repeat)
      return false;
  }
  if (IObserver* observer = m_observer; observer)
    observer->OnRepeat(type, repeat);
  return true;
}

std::optional<Type> CApplicationPlayLists::GetPlayingType() const
{
  std::unique_lock lock(m_critSection);
  return m_playingType;
}

std::optional<Type> CApplicationPlayLists::GetTypeToShow(std::optional<Type> preferred) const
{
  if (const std::optional<Type> playing = GetPlayingType(); playing)
    return playing;
  if (preferred && !GetPlayList(*preferred).IsEmpty())
    return preferred;
  for (const Type type : {PLAYLIST::Video, PLAYLIST::Audio})
    if (!GetPlayList(type).IsEmpty())
      return type;
  return std::nullopt;
}

void CApplicationPlayLists::SetPlayingType(Type type)
{
  ChangePlayingType(type);
}

void CApplicationPlayLists::ChangePlayingType(std::optional<Type> type)
{
  {
    std::unique_lock lock(m_critSection);
    if (type == m_playingType)
      return;
  }

  // a feed plays on the playing playlist, so it is dropped while that is still the one playing
  if (const std::optional<Type> fed = GetFedType(); fed && fed != type)
    DropFeed();

  std::unique_lock lock(m_critSection);
  m_playingType = type;
}

MEDIA::Streams CApplicationPlayLists::GetStreams(Type type, EntryId entry) const
{
  return GetPlayList(type).GetStreams(entry).value_or(
      type == PLAYLIST::Video ? MEDIA::Streams::VideoAndAudio : MEDIA::Streams::Audio);
}

bool CApplicationPlayLists::IsPlayingAsAudio() const
{
  if (const std::optional<MEDIA::Streams> streams = GetPlayingStreams(); streams)
    return *streams == MEDIA::Streams::Audio;
  return m_playback->IsPlayingAudio();
}

std::optional<MEDIA::Streams> CApplicationPlayLists::GetPlayingStreams() const
{
  if (const std::optional<Type> type = GetPlayingType(); type)
    return GetStreams(*type, GetPlayList(*type).GetCurrent());
  return std::nullopt;
}

bool CApplicationPlayLists::IsStartingAsAudio(const CFileItem& item) const
{
  // what starts is the current entry, unless the file was started outside the playlists
  if (const std::optional<Type> type = GetPlayingType(); type)
  {
    const EntryId current = GetPlayList(*type).GetCurrent();
    if (const auto entry = GetPlayList(*type).GetItem(current); entry && entry->IsSamePath(&item))
      return GetStreams(*type, current) == MEDIA::Streams::Audio;
  }
  return TypeFor(item) == PLAYLIST::Audio;
}

int CApplicationPlayLists::GetPlayingPosition(Type type, int offset /* = 0 */) const
{
  if (GetPlayingType() != type)
    return -1;
  const CPlayList& playList = GetPlayList(type);
  return playList.GetPosition(playList.PeekOffset(offset));
}

int CApplicationPlayLists::GetPlayingDisplayPosition(Type type) const
{
  if (GetPlayingType() != type)
    return -1;
  const CPlayList& playList = GetPlayList(type);
  return playList.GetPlayOrderPosition(playList.GetCurrent());
}

EntryId CApplicationPlayLists::GetPlayingEntry(Type type) const
{
  return GetPlayingType() == type ? GetPlayList(type).GetCurrent() : NO_ENTRY;
}

bool CApplicationPlayLists::HasNext(Type type) const
{
  return GetPlayingType() == type && GetPlayList(type).PeekNext(Advance::User) != NO_ENTRY;
}

bool CApplicationPlayLists::HasPrevious(Type type) const
{
  return GetPlayingType() == type && GetPlayList(type).PeekPrevious() != NO_ENTRY;
}

bool CApplicationPlayLists::HasEntry(Type type, int offset) const
{
  return GetPlayingType() == type && GetPlayList(type).PeekOffset(offset) != NO_ENTRY;
}

Type CApplicationPlayLists::GetQueueType(Type requested) const
{
  return requested == PLAYLIST::Audio && IsAudioFollowingVideo() ? PLAYLIST::Video : requested;
}

bool CApplicationPlayLists::IsAudioFollowingVideo() const
{
  std::unique_lock lock(m_critSection);
  return m_audioFollowsVideo;
}

std::optional<CApplicationPlayLists::Step> CApplicationPlayLists::Previous()
{
  if (m_playback->RestartsOnPrevious())
    return std::nullopt;
  if (IsSingleItemNonRepeatPlaylist())
    return std::nullopt;
  return PlayPrevious();
}

std::optional<CApplicationPlayLists::Step> CApplicationPlayLists::Next()
{
  if (IsSingleItemNonRepeatPlaylist())
    return std::nullopt;
  return PlayNext();
}

bool CApplicationPlayLists::IsSingleItemNonRepeatPlaylist() const
{
  const std::optional<Type> type = GetPlayingType();
  if (!type)
    return true;

  const CPlayList& playList = GetPlayList(*type);
  return IsPlayingChannel() ||
         (playList.Size() <= 1 && !playList.IsRepeatCurrent() && playList.GetWrap() == Wrap::Off);
}

bool CApplicationPlayLists::IsPlayingChannel() const
{
  const std::optional<Type> type = GetPlayingType();
  if (!type)
    return false;
  const auto item = GetPlayList(*type).GetCurrentItem();
  return item && item->HasPVRChannelInfoTag();
}

bool CApplicationPlayLists::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_QUEUE_NEXT_ITEM:
      QueueNextEntry();
      return true;

    case GUI_MSG_NOTIFY_ALL:
      if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
      {
        const auto item = std::static_pointer_cast<CFileItem>(message.GetItem());
        for (const auto& playList : m_playLists)
          playList->UpdateItem(*item);
      }
      break;

    case GUI_MSG_PLAYBACK_STARTED:
    {
      const std::shared_ptr<CFileItem> started = OnStarted(message);
      {
        std::unique_lock lock(m_critSection);
        m_currentItem = started ? started : std::make_shared<CFileItem>();
        m_playbackStarted = true;
        m_audioFollowsVideo = false;
        if (m_playingType == PLAYLIST::Video)
        {
          const EntryId current = GetPlayList(PLAYLIST::Video).GetCurrent();
          m_audioFollowsVideo =
              current != NO_ENTRY && GetStreams(PLAYLIST::Video, current) != MEDIA::Streams::Video;
        }
      }
      if (IObserver* observer = m_observer; observer)
        observer->OnStarted(started);
      break;
    }

    case GUI_MSG_PLAYBACK_ENDED:
      // the playlist plays on until the next entry starts or nothing follows
      break;

    case GUI_MSG_PLAYBACK_ERROR:
    {
      std::shared_ptr<const CFileItem> failed;
      {
        std::unique_lock lock(m_critSection);
        m_audioFollowsVideo = false;
        failed = m_currentItem;
      }
      // the stop that follows carries no error, so this is the only point the failure is known
      ReportFailed(failed, FailReason::Error);
      break;
    }

    case GUI_MSG_PLAYBACK_STOPPED:
    {
      bool wasPlaying;
      {
        std::unique_lock lock(m_critSection);
        m_audioFollowsVideo = false;
        wasPlaying = m_playingType && m_playbackStarted;
      }
      if (wasPlaying)
      {
        EndPlayback(false);
        return true;
      }
      break;
    }
  }

  return false;
}

void CApplicationPlayLists::EndPlayback(bool clearPlayList)
{
  std::optional<Type> type;
  {
    std::unique_lock lock(m_critSection);
    type = m_playingType;
    m_playingType.reset();
    m_playbackStarted = false;
    m_audioFollowsVideo = false;
    m_queued = NO_ENTRY;
  }

  if (IGUIListener* listener = m_guiListener; listener)
    listener->OnStopped();

  if (type)
  {
    if (clearPlayList)
      EditPlayList(*type).Clear();
    else
      EditPlayList(*type).ClearCurrent();
    EditPlayList(*type).SetFeed(nullptr);
  }

  ReportPlayListsChanged();
}

bool CApplicationPlayLists::PlayFrom(Type type,
                                     std::optional<int> position /* = std::nullopt */,
                                     const PlayOptions& options /* = {} */)
{
  return StartPlaying(type, position, options, false);
}

bool CApplicationPlayLists::StartPlaying(Type type,
                                         std::optional<int> position,
                                         const PlayOptions& options,
                                         bool newContents)
{
  CPlayList& playList = EditPlayList(type);
  if (playList.IsEmpty())
    return false;

  // on the playlist already playing, the play order is kept unless the contents are new, and
  // the screen stays as the user is watching
  const bool playing = GetPlayingType() == type;
  const bool newOrder = newContents || !playing;
  if (newOrder && options.inOrder)
    SetShuffle(type, false, Persist::No);
  SetPlayingType(type);

  EntryId entry;
  if (position)
  {
    entry = playList.GetEntryId(std::clamp(*position, 0, playList.Size() - 1));
    playList.SetCurrent(entry);
    if (newOrder)
      playList.ResetShuffle();
  }
  else
  {
    playList.ClearCurrent();
    entry = playList.PeekNext(Advance::User);
  }
  return PlayEntry(type, entry, options, playing ? StartsRun::No : StartsRun::Yes);
}

bool CApplicationPlayLists::PlayItems(Type type,
                                      const CFileItemList& items,
                                      std::optional<int> position /* = std::nullopt */,
                                      const PlayOptions& options /* = {} */,
                                      const std::string& sourcePath /* = "" */)
{
  CFileItemList entries;
  const std::optional<int> start = EntriesOf(items, position, entries);
  EditPlayList(type).Assign(entries, sourcePath);
  return StartPlaying(type, start, options, true);
}

bool CApplicationPlayLists::PlayExpanded(Type type,
                                         const CFileItemList& items,
                                         int start,
                                         const std::shared_ptr<CFileItem>& chosen,
                                         const PlayOptions& options)
{
  if (chosen && start < 0)
    return PlayItem(type, chosen, options);
  return PlayItems(type, items, start < 0 ? std::nullopt : std::optional<int>(start), options);
}

namespace
{
class CEntryExpansion
{
public:
  CEntryExpansion(const std::shared_ptr<CFileItem>& root,
                  IEntryRules& rules,
                  std::shared_ptr<CFileItem> startAt,
                  CFileItemList& entries)
    : m_root(root),
      m_rules(rules),
      m_startAt(std::move(startAt)),
      m_entries(entries)
  {
  }

  std::optional<int> Run()
  {
    m_entries.SetFastLookup(true);
    Expand(m_root, *m_root);
    return m_start;
  }

private:
  // listed is what the user sees for item, which a folder may have been redirected from
  void Expand(const std::shared_ptr<CFileItem>& item, const CFileItem& listed)
  {
    // the item asked for is queued whatever its own flag says; what it contains follows the flag
    if (YieldsNoEntries(*item) || (item != m_root && !item->CanQueue()))
      return;

    if (item->IsFolder() || IsSmartPlayList(*item))
    {
      if (const auto redirected = m_rules.Redirect(item); redirected != item)
      {
        if (redirected)
          Expand(redirected, listed);
        return;
      }
      if (!m_opened.insert(item->GetPath()).second)
        return;
      if (item->IsShareOrDrive() && !m_rules.IsUnlocked(*item))
        return;

      CFileItemList items;
      XFILE::CDirectory::GetDirectory(item->GetPath(), items, "", XFILE::DIR_FLAG_DEFAULTS);
      m_rules.Arrange(*item, items, m_startAt);
      for (const auto& child : items)
        Expand(child, *child);
    }
    else if (IsPlayList(*item))
    {
      if (!m_opened.insert(item->GetPath()).second)
        return;

      CFileItemList items;
      XFILE::CPlaylistFileDirectory().GetDirectory(CURL(item->GetPath()), items);
      for (const auto& child : items)
        Expand(child, *child);
    }
    else if ((NETWORK::IsInternetStream(*item) && !MUSIC::IsMusicDb(*item)) ||
             (item->IsPlugin() && item->GetProperty(ITEM::PROPERTY::IS_PLAYABLE).asBoolean()))
    {
      // resolved when it plays
      Add(listed, item);
    }
    else if (const auto entry = m_rules.Accept(item, m_entries); entry)
    {
      Add(listed, entry);
    }
  }

  void Add(const CFileItem& listed, const std::shared_ptr<CFileItem>& entry)
  {
    if (!m_start && m_startAt && listed.IsSamePath(m_startAt.get()))
    {
      m_start = m_entries.Size();
      entry->SetStartPartNumber(m_startAt->GetStartPartNumber());
    }
    m_entries.Add(entry);
  }

  const std::shared_ptr<CFileItem> m_root;
  IEntryRules& m_rules;
  std::shared_ptr<CFileItem> m_startAt;
  CFileItemList& m_entries;
  std::optional<int> m_start;
  //! Folders and playlist files already opened, so one that lists itself is opened once.
  std::set<std::string, std::less<>> m_opened;
};
} // namespace

std::optional<int> CApplicationPlayLists::ExpandToEntries(const std::shared_ptr<CFileItem>& item,
                                                          IEntryRules& rules,
                                                          std::shared_ptr<CFileItem> startAt,
                                                          CFileItemList& entries)
{
  return CEntryExpansion(item, rules, std::move(startAt), entries).Run();
}

bool CEntriesAsListed::IsUnlocked(CFileItem& source)
{
  return true;
}

void CEntriesAsListed::Arrange(const CFileItem& folder,
                               CFileItemList& items,
                               std::shared_ptr<CFileItem>& startAt)
{
}

std::shared_ptr<CFileItem> CEntriesAsListed::Accept(const std::shared_ptr<CFileItem>& file,
                                                    const CFileItemList& entries)
{
  return CanBeEntry(*file) ? file : nullptr;
}

std::optional<int> CApplicationPlayLists::EntriesOf(const CFileItemList& items,
                                                    std::optional<int> position,
                                                    CFileItemList& entries)
{
  std::optional<int> kept;
  for (int i = 0; i < items.Size(); ++i)
  {
    if (!PLAYLIST::CanBeEntry(*items[i]))
      continue;
    if (position == i)
      kept = entries.Size();
    entries.Add(items[i]);
  }
  return kept;
}

std::string CApplicationPlayLists::GetPlayingSourcePath() const
{
  const std::optional<Type> type = GetPlayingType();
  if (!type)
    return {};
  return GetPlayList(*type).GetSourcePath();
}

int CApplicationPlayLists::Queue(Type type, const CFileItemList& queued, Placement placement)
{
  CFileItemList items;
  EntriesOf(queued, std::nullopt, items);
  if (items.IsEmpty())
    return -1;

  // a fed playlist plays in order, so what the user adds goes ahead of what the feed places
  if (GetPlayList(type).GetFeed())
    placement = Placement::Next;

  CPlayList& playList = EditPlayList(type);
  playList.ClearSourcePath();
  if (placement == Placement::Next && GetPlayingType() == type)
    return playList.GetPosition(playList.QueueNext(items));

  const int first = playList.Size();
  playList.Add(items);
  return first;
}

bool CApplicationPlayLists::Remove(Type type, int position)
{
  CPlayList& playList = EditPlayList(type);
  return playList.RemoveEntry(playList.GetEntryId(position), GetKeptEntries(type));
}

int CApplicationPlayLists::Queue(Type type,
                                 const std::shared_ptr<CFileItem>& item,
                                 Placement placement /* = Placement::End */)
{
  CFileItemList items;
  items.Add(item);
  return Queue(type, items, placement);
}

void CApplicationPlayLists::Insert(Type type, const CFileItemList& items, int position)
{
  CFileItemList entries;
  EntriesOf(items, std::nullopt, entries);
  CPlayList& playList = EditPlayList(type);
  playList.ClearSourcePath();
  playList.Insert(entries, position);
}

void CApplicationPlayLists::Remove(Type type, const std::string& path)
{
  EditPlayList(type).Remove(path, GetKeptEntries(type));
}

std::vector<EntryId> CApplicationPlayLists::GetKeptEntries(Type type) const
{
  std::vector<EntryId> kept{GetPlayingEntry(type)};
  std::unique_lock lock(m_critSection);
  // what is handed on belongs to the playing playlist; the other numbers its entries alike
  if (m_playingType == type && m_queued != NO_ENTRY)
    kept.push_back(m_queued);
  return kept;
}

void CApplicationPlayLists::ReplaceItem(Type type, EntryId entry, const CFileItem& item)
{
  EditPlayList(type).ReplaceItem(entry, item);
}

void CApplicationPlayLists::UpdateItem(Type type, const CFileItem& item)
{
  EditPlayList(type).UpdateItem(item);
}

void CApplicationPlayLists::RemoveDiscItems()
{
  for (const auto& playList : m_playLists)
    playList->RemoveIf([](const CFileItem& item) { return MUSIC::IsCDDA(item) || item.IsOnDVD(); });
}

bool CApplicationPlayLists::Swap(Type type, int position1, int position2)
{
  return EditPlayList(type).Swap(position1, position2);
}

bool CApplicationPlayLists::Move(Type type, int from, int to)
{
  return EditPlayList(type).Move(from, to);
}

void CApplicationPlayLists::Clear(Type type)
{
  EditPlayList(type).Clear();
}

void CApplicationPlayLists::Replace(Type type, const CFileItemList& items)
{
  CFileItemList entries;
  EntriesOf(items, std::nullopt, entries);
  EditPlayList(type).Replace(entries);
}

bool CApplicationPlayLists::PlayItem(std::optional<Type> named,
                                     const std::shared_ptr<CFileItem>& item,
                                     const PlayOptions& options /* = {} */)
{
  if (!CanBeEntry(*item))
    return false;

  const Type type = named ? *named : TypeFor(*item);
  if (GetPlayList(type).GetFeed() && GetPlayingType() == type)
  {
    Queue(type, item, Placement::Next);
    return PlayNextEntry(Advance::User, StartsRun::No, options) == Step::Played;
  }

  CFileItemList single;
  single.Add(item);
  EditPlayList(type).Assign(single);
  return StartPlaying(type, std::nullopt, options, true);
}

bool CApplicationPlayLists::PlayEntry(Type type,
                                      EntryId entry,
                                      const PlayOptions& options,
                                      StartsRun startsRun)
{
  CPlayList& playList = EditPlayList(type);
  if (playList.GetPosition(entry) < 0)
    return false;

  for (int i = 0; i < EXPANSION_DEPTH; i++)
  {
    const EntryId expanded = ExpandEntry(playList, entry);
    if (expanded == NO_ENTRY)
      break;
    entry = expanded;
  }
  if (!playList.SetCurrent(entry))
    return false;

  std::shared_ptr<CFileItem> item = playList.GetItem(entry);
  if (!item)
    return false;

  if (auto tagged = std::make_shared<CFileItem>(*item); m_playback->LoadLibraryTag(*tagged))
  {
    playList.ReplaceItem(entry, *tagged);
    item = std::move(tagged);
  }

  {
    std::unique_lock lock(m_critSection);
    m_playbackStarted = false;
  }

  const auto playAttempt = std::chrono::steady_clock::now();
  if (!m_playback->Open(*item, options, startsRun))
  {
    CLog::Log(LOGERROR, "Playlist: skipping unplayable entry: {}, path [{}]", entry,
              CURL::GetRedacted(item->GetDynPath()));
    playList.SetUnPlayable(entry);
    ReportFailed(item, FailReason::Unplayable);

    bool abort;
    {
      std::unique_lock lock(m_critSection);
      if (!m_failedEntries)
        m_failedEntriesStart = playAttempt;
      m_failedEntries++;

      const auto advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
      const int retries = advancedSettings->m_playlistRetries;
      const int timeout = advancedSettings->m_playlistTimeout;
      abort = (retries >= 0 && m_failedEntries >= retries) ||
              (timeout > 0 && std::chrono::steady_clock::now() - m_failedEntriesStart >=
                                  std::chrono::seconds(timeout));
      if (abort)
      {
        m_failedEntries = 0;
        m_failedEntriesStart = std::chrono::steady_clock::now();
      }
    }

    if (abort)
    {
      CLog::Log(LOGDEBUG, "Playlist: one or more items failed to play... aborting playback");
      if (IGUIListener* listener = m_guiListener; listener)
        listener->OnEntriesFailed();
      EndPlayback(true);
      return false;
    }

    if (playList.GetPlayable() > 0)
      return (options.onFail == OnFail::Previous
                  ? PlayPreviousEntry(startsRun)
                  : PlayNextEntry(Advance::Automatic, startsRun)) == Step::Played;

    CLog::Log(LOGDEBUG, "Playlist: no more playable items... aborting playback");
    EndPlayback(false);
    return false;
  }

  // a resume applies to this play only; the entry played again starts from the beginning
  if (item->GetStartOffset() == STARTOFFSET_RESUME)
  {
    CFileItem fromStart(*item);
    fromStart.SetStartOffset(0);
    playList.ReplaceItem(entry, fromStart);
  }

  std::unique_lock lock(m_critSection);
  m_failedEntries = 0;
  m_failedEntriesStart = std::chrono::steady_clock::now();
  return true;
}

bool CApplicationPlayLists::OnEntryEnded(const CFileItem& ended)
{
  // an EPG playlist item keeps the player open for continuous viewing
  if (ended.GetProperty(ITEM::PROPERTY::EPG_PLAYLIST_ITEM).asBoolean(false))
    return false;

  if (PlayNext(Advance::Automatic) != Step::Played)
    m_playback->Close();
  return true;
}

CApplicationPlayLists::Step CApplicationPlayLists::PlayNext(Advance advance /* = Advance::User */)
{
  return PlayNextEntry(advance, StartsRun::No);
}

CApplicationPlayLists::Step CApplicationPlayLists::PlayNextEntry(
    Advance advance, StartsRun startsRun, const PlayOptions& options /* = {} */)
{
  const std::optional<Type> type = GetPlayingType();
  // what follows a channel is PVR's business: its channel navigator and EPG, never the playlist
  EntryId next = NO_ENTRY;
  if (type && !IsPlayingChannel() && GetPlayList(*type).GetPlayable() > 0)
    next = EditPlayList(*type).Next(advance);

  if (next == NO_ENTRY)
  {
    EndPlayback(false);
    return Step::NothingThere;
  }

  // with no player named, what follows plays in the player already in use
  PlayOptions nextOptions = options;
  if (nextOptions.player.empty())
    nextOptions.player = m_playback->GetName();
  return PlayEntry(*type, next, nextOptions, startsRun) ? Step::Played : Step::Failed;
}

CApplicationPlayLists::Step CApplicationPlayLists::PlayNext(Type type)
{
  const StartsRun startsRun = GetPlayingType() != type ? StartsRun::Yes : StartsRun::No;
  SetPlayingType(type);
  return PlayNextEntry(Advance::User, startsRun);
}

CApplicationPlayLists::Step CApplicationPlayLists::PlayPrevious(Type type)
{
  const StartsRun startsRun = GetPlayingType() != type ? StartsRun::Yes : StartsRun::No;
  SetPlayingType(type);
  return PlayPreviousEntry(startsRun);
}

CApplicationPlayLists::Step CApplicationPlayLists::PlayPrevious()
{
  return PlayPreviousEntry(StartsRun::No);
}

CApplicationPlayLists::Step CApplicationPlayLists::PlayPreviousEntry(StartsRun startsRun)
{
  const std::optional<Type> type = GetPlayingType();
  if (!type)
    return Step::Failed;

  const EntryId previous = EditPlayList(*type).Previous();
  if (previous == NO_ENTRY)
    return Step::NothingThere;

  return PlayEntry(*type, previous, {.onFail = OnFail::Previous}, startsRun) ? Step::Played
                                                                             : Step::Failed;
}

CApplicationPlayLists::Step CApplicationPlayLists::PlayOffset(int offset)
{
  const std::optional<Type> type = GetPlayingType();
  if (!type)
    return Step::Failed;

  const EntryId entry = GetPlayList(*type).PeekOffset(offset, Advance::User);
  if (entry == NO_ENTRY)
  {
    EndPlayback(false);
    return Step::NothingThere;
  }

  return PlayEntry(*type, entry, {.player = m_playback->GetName()}, StartsRun::No) ? Step::Played
                                                                                   : Step::Failed;
}

CApplicationPlayLists::Step CApplicationPlayLists::PlayOffset(Type type, int offset)
{
  if (GetPlayingType() != type)
  {
    m_playback->Stop();
    SetPlayingType(type);
  }
  if (m_playback->IsPlaying())
    return PlayOffset(offset);
  return PlayFrom(type, offset) ? Step::Played : Step::Failed;
}

void CApplicationPlayLists::OnNextQueued(EntryId entry)
{
  std::unique_lock lock(m_critSection);
  m_queued = entry;
}

void CApplicationPlayLists::SkipQueued(EntryId entry)
{
  if (const std::optional<Type> type = GetPlayingType(); type)
    EditPlayList(*type).SetCurrent(entry);
}

void CApplicationPlayLists::QueueNextEntry()
{
  const std::optional<Type> type = GetPlayingType();
  const EntryId next = type ? GetPlayList(*type).PeekNext(Advance::Automatic) : NO_ENTRY;
  const std::shared_ptr<CFileItem> nextItem = type ? GetPlayList(*type).GetItem(next) : nullptr;

  // the player only moves on by itself to the same kind of media
  const MEDIA::Streams playing =
      m_playback->IsPlayingVideo() ? MEDIA::Streams::VideoAndAudio : MEDIA::Streams::Audio;
  if (!nextItem || GetStreams(*type, next) != playing)
  {
    m_playback->NothingToQueue();
    return;
  }

  // a refused entry is still moved past, so the player can be offered the one after it
  switch (m_playback->QueueNext(*nextItem))
  {
    case IPlayback::Queued::Yes:
      OnNextQueued(next);
      break;
    case IPlayback::Queued::Refused:
      SkipQueued(next);
      break;
    case IPlayback::Queued::Unresolved:
      break;
  }
}

void CApplicationPlayLists::ReportFailed(const std::shared_ptr<const CFileItem>& item,
                                         FailReason reason) const
{
  if (IObserver* observer = m_observer; observer)
    observer->OnFailed(item, reason);
}

void CApplicationPlayLists::ClearQueued()
{
  std::unique_lock lock(m_critSection);
  m_queued = NO_ENTRY;
}

std::shared_ptr<CFileItem> CApplicationPlayLists::OnStarted(const CGUIMessage& message)
{
  std::optional<Type> type;
  EntryId queued;
  {
    std::unique_lock lock(m_critSection);
    type = m_playingType;
    queued = m_queued;
    m_queued = NO_ENTRY;
  }

  const auto reported = std::static_pointer_cast<CFileItem>(message.GetItem());
  const std::shared_ptr<CFileItem> started =
      reported ? std::make_shared<CFileItem>(*reported) : nullptr;
  if (queued == NO_ENTRY || !type)
    return started;

  // the player started what it was handed to follow on; if that has left the playlist since, what
  // the player reports is still what started
  CPlayList& playList = EditPlayList(*type);
  const std::shared_ptr<CFileItem> item = playList.GetItem(queued);
  if (!item || !playList.SetCurrent(queued))
    return started;
  return std::make_shared<CFileItem>(*item);
}

std::shared_ptr<CFileItem> CApplicationPlayLists::GetCurrentItem() const
{
  std::unique_lock lock(m_critSection);
  return m_currentItem;
}

void CApplicationPlayLists::ResetCurrentItem()
{
  std::unique_lock lock(m_critSection);
  m_currentItem = std::make_shared<CFileItem>();
}

void CApplicationPlayLists::SetShuffle(Type type, bool shuffle, Persist persist)
{
  CPlayList& playList = EditPlayList(type);
  if (shuffle)
    playList.SetShuffle(std::make_unique<CPlayListRandomShuffle>());
  else
    playList.SetShuffled(false);

  if (persist == Persist::No)
    return;
  CMediaSettings::GetInstance().SetPlayListShuffled(type, IsShuffled(type));
  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
}

void CApplicationPlayLists::ToggleShuffle(Type type, Persist persist)
{
  SetShuffle(type, !IsShuffled(type), persist);
}

void CApplicationPlayLists::CycleRepeat(Type type, Persist persist)
{
  Repeat next = Repeat::Off;
  switch (GetRepeat(type))
  {
    case Repeat::Off:
      next = Repeat::All;
      break;
    case Repeat::All:
      next = Repeat::One;
      break;
    case Repeat::One:
      break;
  }
  SetRepeat(type, next, persist);
}

uint32_t CApplicationPlayLists::RepeatLabel(Repeat repeat, RepeatWording wording)
{
  // in the order of Repeat: Off, One, All
  static constexpr std::array<uint32_t, 3> STATE{594, 592, 593};
  static constexpr std::array<uint32_t, 3> WITH_SETTING{595, 596, 597};
  const auto index = static_cast<size_t>(repeat);
  return wording == RepeatWording::State ? STATE[index] : WITH_SETTING[index];
}

bool CApplicationPlayLists::IsShuffled(Type type) const
{
  return GetPlayList(type).IsShuffled();
}

void CApplicationPlayLists::SetRepeat(Type type, Repeat repeat, Persist persist)
{
  CPlayList& playList = EditPlayList(type);
  m_composingRepeat = true;
  playList.SetRepeatCurrent(repeat == Repeat::One);
  playList.SetWrap(repeat == Repeat::All ? Wrap::ToStart : Wrap::Off);
  m_composingRepeat = false;
  if (ReportRepeat(type))
    ReportPlayListsChanged();

  if (persist == Persist::No)
    return;
  CMediaSettings::GetInstance().SetPlayListRepeat(type, GetRepeat(type) == Repeat::All);
  CServiceBroker::GetSettingsComponent()->GetSettings()->Save();
}

Repeat CApplicationPlayLists::GetRepeat(Type type) const
{
  const CPlayList& playList = GetPlayList(type);
  if (playList.IsRepeatCurrent())
    return Repeat::One;
  if (playList.GetWrap() == Wrap::ToStart)
    return Repeat::All;
  return Repeat::Off;
}

void CApplicationPlayLists::RestoreSavedPlayOrder()
{
  RestoreSavedPlayOrder(PLAYLIST::Audio);
  RestoreSavedPlayOrder(PLAYLIST::Video);
}

void CApplicationPlayLists::RestoreSavedPlayOrder(Type type)
{
  const CMediaSettings& saved = CMediaSettings::GetInstance();
  SetRepeat(type, saved.GetPlayListRepeat(type) ? Repeat::All : Repeat::Off, Persist::No);
  SetShuffle(type, saved.GetPlayListShuffled(type), Persist::No);
}

bool CApplicationPlayLists::PlayFeed(Type type, std::shared_ptr<IFeed> feed, Repeat repeat)
{
  const bool wasFed = GetFedType().has_value();
  m_composingFeed = true;
  DropFeed();
  CPlayList& playList = EditPlayList(type);
  playList.Clear();
  playList.SetFeed(std::move(feed));
  SetShuffle(type, false, Persist::No);
  SetRepeat(type, repeat, Persist::No);
  TopUp(type);
  if (playList.IsEmpty())
    playList.SetFeed(nullptr);
  const bool playing = !playList.IsEmpty() && StartPlaying(type, 0, {}, true);
  m_composingFeed = false;

  // reported once the feed has been swapped, and once something plays there is a player to report
  // it on
  if (const bool fed = GetFedType().has_value(); fed != wasFed || fed)
  {
    if (IObserver* observer = m_observer; observer)
      observer->OnFeed(fed);
  }
  return playing;
}

std::optional<Type> CApplicationPlayLists::GetFedType() const
{
  for (const Type type : {PLAYLIST::Video, PLAYLIST::Audio})
    if (GetPlayList(type).GetFeed())
      return type;
  return std::nullopt;
}

void CApplicationPlayLists::DropFeed()
{
  if (const std::optional<Type> type = GetFedType(); type)
    EditPlayList(*type).Clear();
}

std::shared_ptr<IFeed> CApplicationPlayLists::GetFeed() const
{
  const std::optional<Type> type = GetFedType();
  return type ? GetPlayList(*type).GetFeed() : nullptr;
}

int CApplicationPlayLists::GetFeedTotal() const
{
  const std::shared_ptr<IFeed> feed = GetFeed();
  return feed ? feed->GetTotal() : -1;
}

int CApplicationPlayLists::GetFeedLeft() const
{
  const std::shared_ptr<IFeed> feed = GetFeed();
  return feed ? feed->GetLeft() : -1;
}

void CApplicationPlayLists::TopUp(Type type)
{
  std::atomic<int>& requests = m_topUpRequests[Index(type)];
  if (requests.fetch_add(1) > 0)
    return;

  int served = 1;
  while (true)
  {
    TopUpOnce(type);
    const int arrived = requests.fetch_sub(served) - served;
    if (arrived == 0)
      break;
    served = arrived;
  }
}

void CApplicationPlayLists::TopUpOnce(Type type)
{
  const std::shared_ptr<IFeed> feed = GetPlayList(type).GetFeed();
  if (!feed)
    return;

  CPlayList& playList = EditPlayList(type);
  // counted in play order, which is what plays next whether or not the list is shuffled
  const int missing =
      FEED_DEPTH - (playList.Size() - playList.GetPlayOrderPosition(playList.GetCurrent()) - 1);
  if (missing > 0)
  {
    std::vector<std::shared_ptr<CFileItem>> items = feed->Take(missing);
    if (static_cast<int>(items.size()) < missing && playList.GetWrap() == Wrap::ToStart &&
        feed->GetTotal() > 0)
    {
      feed->Restart();
      for (auto& item : feed->Take(missing - static_cast<int>(items.size())))
        items.push_back(std::move(item));
    }
    // the feed may have been dropped while it was taking
    playList.AddFromFeed(items, feed);
  }
}

void CApplicationPlayLists::ClearPlayLists()
{
  for (const auto& playList : m_playLists)
    playList->Clear();
}

void CApplicationPlayLists::Reset()
{
  ClearPlayLists();
  ChangePlayingType(std::nullopt);
}

void CApplicationPlayLists::SetObserver(IObserver* observer)
{
  m_observer = observer;
}

void CApplicationPlayLists::SetGUIListener(IGUIListener* listener)
{
  m_guiListener = listener;
}

void CApplicationPlayLists::ReportPlayListsChanged() const
{
  if (IGUIListener* listener = m_guiListener; listener)
    listener->OnPlayListsChanged();
}

void CApplicationPlayLists::SetSlideShowRunning(bool running)
{
  std::unique_lock lock(m_critSection);
  m_slideShowRunning = running;
}

bool CApplicationPlayLists::IsSlideShowRunning() const
{
  std::unique_lock lock(m_critSection);
  return m_slideShowRunning;
}
