/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayList.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "PlayListFileItemClassify.h"
#include "PlayListShuffle.h"
#include "utils/ItemProperties.h"
#include "utils/log.h"

#include <algorithm>
#include <deque>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace MUSIC_INFO;

namespace KODI::PLAYLIST
{

CPlayList::CPlayList() : m_shuffle(std::make_unique<CPlayListNoShuffle>())
{
}

CPlayList::~CPlayList() = default;

void CPlayList::SetObserver(Observer observer)
{
  std::unique_lock lock(m_critSection);
  m_observer = std::move(observer);
}

void CPlayList::Notify(const Changes& changes) const
{
  if (changes.empty())
    return;

  Observer observer;
  {
    std::unique_lock lock(m_critSection);
    observer = m_observer;
  }
  if (observer)
    observer(changes);
}

int CPlayList::FindLocked(EntryId entry) const
{
  if (entry == NO_ENTRY)
    return -1;

  const auto fits = [this](int position, EntryId id)
  { return position < static_cast<int>(m_entries.size()) && m_entries[position].id == id; };

  if (const auto it = m_positions.find(entry); it != m_positions.end() && fits(it->second, entry))
    return it->second;

  m_positions.clear();
  for (int position = 0; position < static_cast<int>(m_entries.size()); ++position)
    m_positions.emplace(m_entries[position].id, position);

  const auto it = m_positions.find(entry);
  return it == m_positions.end() ? -1 : it->second;
}

PlayListEntry CPlayList::MakeEntryLocked(const std::shared_ptr<CFileItem>& item) const
{
  // the playlist owns its items, so nothing outside it changes one in place
  auto owned = std::make_shared<CFileItem>(*item);

  // set 'IsPlayable' property - needed for properly handling plugin:// URLs
  owned->SetProperty(ITEM::PROPERTY::IS_PLAYABLE, true);

  PlayListEntry entry;
  entry.item = std::move(owned);
  entry.streams = StreamsOf(*entry.item);
  return entry;
}

EntryId CPlayList::InsertLocked(const std::shared_ptr<CFileItem>& item, int position,
                                Changes& changes)
{
  const int size = static_cast<int>(m_entries.size());
  if (position < 0 || position > size)
    position = size;

  PlayListEntry entry = MakeEntryLocked(item);
  entry.id = ++m_lastId;
  m_entries.insert(m_entries.begin() + position, entry);
  m_shuffle->OnAdded(entry.id, position, m_current);

  changes.push_back({PlayListChange::Type::Added, entry.id, position, entry.item});
  return entry.id;
}

void CPlayList::RemoveLocked(int position, Changes& changes)
{
  const EntryId entry = m_entries[position].id;

  // Leave the cursor on the entry before, so that what followed the removed entry still follows.
  const bool wasCurrent = entry == m_current;
  if (wasCurrent)
    m_current = m_shuffle->Preceding(entry);

  m_entries.erase(m_entries.begin() + position);
  m_shuffle->OnRemoved(entry);

  changes.push_back({PlayListChange::Type::Removed, entry, position, nullptr});
  if (wasCurrent)
    changes.push_back({PlayListChange::Type::Current, m_current, FindLocked(m_current), nullptr});
}

void CPlayList::RemoveIfLocked(const std::function<bool(const PlayListEntry&)>& remove,
                               Changes& changes)
{
  for (int position = 0; position < static_cast<int>(m_entries.size());)
  {
    if (remove(m_entries[position]))
      RemoveLocked(position, changes);
    else
      ++position;
  }
}

EntryId CPlayList::Add(const std::shared_ptr<CFileItem>& item)
{
  return Insert(item, -1);
}

void CPlayList::Add(const CFileItemList& items)
{
  Insert(items, -1);
}

void CPlayList::AddFromFeed(const std::vector<std::shared_ptr<CFileItem>>& items,
                            const std::shared_ptr<IFeed>& feed)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    if (!feed || m_feed != feed)
      return;
    for (const auto& item : items)
      InsertLocked(item, -1, changes);
  }
  Notify(changes);
}

void CPlayList::Insert(const CFileItemList& items, int iPosition /* = -1 */)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    if (iPosition < 0 || iPosition > static_cast<int>(m_entries.size()))
      iPosition = static_cast<int>(m_entries.size());
    for (int i = 0; i < items.Size(); i++)
      InsertLocked(items[i], iPosition++, changes);
  }
  Notify(changes);
}

EntryId CPlayList::Insert(const std::shared_ptr<CFileItem>& item, int iPosition /* = -1 */)
{
  Changes changes;
  EntryId entry;
  {
    std::unique_lock lock(m_critSection);
    entry = InsertLocked(item, iPosition, changes);
  }
  Notify(changes);
  return entry;
}

void CPlayList::Clear()
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    ClearLocked(changes);
  }
  Notify(changes);
}

void CPlayList::ClearLocked(Changes& changes)
{
  if (!m_entries.empty())
    changes.push_back({PlayListChange::Type::Cleared, NO_ENTRY, -1, nullptr});

  m_entries.clear();
  m_current = NO_ENTRY;
  m_requests.clear();
  m_shuffle->Reset({}, NO_ENTRY);
  m_sourcePath.clear();
  if (std::exchange(m_feed, nullptr))
    changes.push_back({PlayListChange::Type::Feed, NO_ENTRY, -1, nullptr});
}

void CPlayList::Assign(const CFileItemList& items, const std::string& sourcePath /* = "" */)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    ClearLocked(changes);
    for (const auto& item : items)
      InsertLocked(item, -1, changes);
    m_sourcePath = sourcePath;
  }
  Notify(changes);
}

void CPlayList::Replace(const CFileItemList& items)
{
  const auto sameFile = [](const CFileItem& a, const CFileItem& b)
  { return a.GetPath() == b.GetPath() && a.GetStartOffset() == b.GetStartOffset(); };

  Changes changes;
  {
    std::unique_lock lock(m_critSection);

    // the same entries rearranged keep what they carry
    std::vector<PlayListEntry> rearranged;
    std::vector<bool> taken(m_entries.size(), false);
    bool same = static_cast<int>(m_entries.size()) == items.Size();
    for (int i = 0; same && i < items.Size(); ++i)
    {
      same = false;
      for (size_t n = 0; n < m_entries.size(); ++n)
      {
        if (taken[n] || !sameFile(*m_entries[n].item, *items[i]))
          continue;
        taken[n] = true;
        rearranged.push_back(m_entries[n]);
        same = true;
        break;
      }
    }

    if (same)
    {
      for (int position = 0; position < static_cast<int>(rearranged.size()); ++position)
      {
        if (rearranged[position].id == m_entries[position].id)
          continue;
        m_entries[position] = rearranged[position];
        m_shuffle->OnMoved(m_entries[position].id, position);
        changes.push_back({PlayListChange::Type::Moved, m_entries[position].id, position, nullptr});
      }
    }
    else
    {
      // an entry whose file is still listed keeps its id, so what refers to it still finds it
      std::vector<PlayListEntry> old = std::move(m_entries);
      std::fill(taken.begin(), taken.end(), false);
      if (!old.empty())
        changes.push_back({PlayListChange::Type::Cleared, NO_ENTRY, -1, nullptr});
      m_entries.clear();
      if (std::exchange(m_feed, nullptr))
        changes.push_back({PlayListChange::Type::Feed, NO_ENTRY, -1, nullptr});

      std::vector<EntryId> listOrder;
      for (const auto& item : items)
      {
        PlayListEntry entry = MakeEntryLocked(item);
        for (size_t n = 0; n < old.size(); ++n)
        {
          if (taken[n] || !sameFile(*old[n].item, *item))
            continue;
          taken[n] = true;
          entry.id = old[n].id;
          entry.playable = old[n].playable;
          break;
        }
        if (entry.id == NO_ENTRY)
          entry.id = ++m_lastId;
        m_entries.push_back(entry);
        listOrder.push_back(entry.id);
        changes.push_back({PlayListChange::Type::Added, entry.id,
                           static_cast<int>(m_entries.size()) - 1, entry.item});
      }

      m_requests.clear();
      if (FindLocked(m_current) < 0)
        MoveCurrentLocked(NO_ENTRY, changes);
      // a fresh deal, led by the current entry
      m_shuffle->Reset(listOrder, m_current);
    }
  }
  Notify(changes);
}

int CPlayList::Size() const
{
  std::unique_lock lock(m_critSection);
  return static_cast<int>(m_entries.size());
}

std::vector<PlayListEntry> CPlayList::GetEntries() const
{
  std::unique_lock lock(m_critSection);
  return m_entries;
}

void CPlayList::GetItems(CFileItemList& items) const
{
  std::unique_lock lock(m_critSection);
  items.Reserve(items.Size() + static_cast<int>(m_entries.size()));
  for (const PlayListEntry& entry : m_entries)
    items.Add(entry.item);
}

std::shared_ptr<CFileItem> CPlayList::operator[](int iItem) const
{
  std::unique_lock lock(m_critSection);
  if (iItem < 0 || iItem >= static_cast<int>(m_entries.size()))
  {
    CLog::Log(LOGERROR, "Error trying to retrieve an item that's out of range");
    return {};
  }
  return m_entries[iItem].item;
}

EntryId CPlayList::GetEntryId(int position) const
{
  std::unique_lock lock(m_critSection);
  if (position < 0 || position >= static_cast<int>(m_entries.size()))
    return NO_ENTRY;
  return m_entries[position].id;
}

int CPlayList::GetPosition(EntryId entry) const
{
  std::unique_lock lock(m_critSection);
  return FindLocked(entry);
}

std::shared_ptr<CFileItem> CPlayList::GetItem(EntryId entry) const
{
  std::unique_lock lock(m_critSection);
  const int position = FindLocked(entry);
  return position < 0 ? nullptr : m_entries[position].item;
}

std::optional<MEDIA::Streams> CPlayList::GetStreams(EntryId entry) const
{
  std::unique_lock lock(m_critSection);
  const int position = FindLocked(entry);
  return position < 0 ? std::nullopt : m_entries[position].streams;
}

EntryId CPlayList::GetCurrent() const
{
  std::unique_lock lock(m_critSection);
  return m_current;
}

int CPlayList::GetCurrentPosition() const
{
  std::unique_lock lock(m_critSection);
  return FindLocked(m_current);
}

std::shared_ptr<CFileItem> CPlayList::GetCurrentItem() const
{
  std::unique_lock lock(m_critSection);
  const int position = FindLocked(m_current);
  return position < 0 ? nullptr : m_entries[position].item;
}

bool CPlayList::SetCurrent(EntryId entry)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    if (FindLocked(entry) < 0)
      return false;

    DropStaleRequestsLocked(m_requests);
    if (!m_requests.empty() && m_requests.front().entry == entry)
    {
      m_requests.pop_front();
      MoveCurrentLocked(entry, changes);
    }
    else
    {
      std::erase_if(m_requests, [entry](const Request& request) { return request.entry == entry; });
      MoveCurrentLocked(entry, changes);
      PlaceRequestsLocked(changes);
    }
  }
  Notify(changes);
  return true;
}

void CPlayList::PlaceRequestsLocked(Changes& changes)
{
  // requests play before the rest, so after a jump they follow the new current entry
  EntryId after = m_current;
  for (const Request& request : m_requests)
  {
    const int from = FindLocked(request.entry);
    if (from < 0)
      continue;
    if (m_shuffle->IsListOrder())
    {
      const int afterPosition = FindLocked(after);
      MoveLocked(from, from > afterPosition ? afterPosition + 1 : afterPosition, changes);
    }
    else
    {
      m_shuffle->PlayAfter(request.entry, after);
    }
    after = request.entry;
  }
}

void CPlayList::ClearCurrent()
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    m_requests.clear();
    MoveCurrentLocked(NO_ENTRY, changes);
  }
  Notify(changes);
}

void CPlayList::MoveCurrentLocked(EntryId entry, Changes& changes)
{
  if (entry == m_current)
    return;
  m_current = entry;
  changes.push_back({PlayListChange::Type::Current, entry, FindLocked(entry), nullptr});
}

void CPlayList::DropStaleRequestsLocked(std::deque<Request>& requests) const
{
  while (!requests.empty() && FindLocked(requests.front().entry) < 0)
    requests.pop_front();
}

EntryId CPlayList::StepLocked(EntryId from, std::deque<Request>& requests, Advance advance) const
{
  DropStaleRequestsLocked(requests);
  if (!requests.empty())
  {
    const EntryId requested = requests.front().entry;
    requests.pop_front();
    return requested;
  }

  if (advance == Advance::Automatic)
  {
    if (const int position = FindLocked(from); position >= 0 && m_repeatCurrent)
    {
      if (!m_entries[position].playable)
      {
        CLog::Log(LOGERROR, "Playlist: repeating entry is unplayable: {}, path [{}]", from,
                  m_entries[position].item->GetPath());
        return NO_ENTRY;
      }
      return from;
    }
  }

  if (const EntryId following = m_shuffle->Following(from); following != NO_ENTRY)
    return following;

  return m_wrap == Wrap::ToStart ? m_shuffle->Following(NO_ENTRY) : NO_ENTRY;
}

EntryId CPlayList::PeekNext(Advance advance, int steps /* = 1 */) const
{
  std::unique_lock lock(m_critSection);
  std::deque<Request> requests = m_requests;
  EntryId entry = m_current;
  for (int i = 0; i < steps && (i == 0 || entry != NO_ENTRY); i++)
  {
    entry = StepLocked(entry, requests, advance);
    advance = Advance::Automatic;
  }
  return entry;
}

EntryId CPlayList::PeekOffset(int offset, Advance advance /* = Advance::Automatic */) const
{
  return offset >= 0 ? PeekNext(advance, offset) : PeekPrevious(-offset);
}

EntryId CPlayList::Next(Advance advance)
{
  Changes changes;
  EntryId next;
  {
    std::unique_lock lock(m_critSection);
    next = StepLocked(m_current, m_requests, advance);
    if (next != NO_ENTRY)
      MoveCurrentLocked(next, changes);
  }
  Notify(changes);
  return next;
}

EntryId CPlayList::StepBackLocked(EntryId from) const
{
  if (from == NO_ENTRY)
    return NO_ENTRY;

  if (const EntryId preceding = m_shuffle->Preceding(from); preceding != NO_ENTRY)
    return preceding;

  if (m_wrap == Wrap::ToStart)
    return m_shuffle->Preceding(NO_ENTRY);

  return NO_ENTRY;
}

EntryId CPlayList::PeekPrevious(int steps /* = 1 */) const
{
  std::unique_lock lock(m_critSection);
  EntryId entry = m_current;
  for (int i = 0; i < steps && entry != NO_ENTRY; i++)
    entry = StepBackLocked(entry);
  return entry;
}

EntryId CPlayList::Previous()
{
  Changes changes;
  EntryId previous;
  {
    std::unique_lock lock(m_critSection);
    previous = StepBackLocked(m_current);
    if (previous != NO_ENTRY)
      MoveCurrentLocked(previous, changes);
  }
  Notify(changes);
  return previous;
}

void CPlayList::SetRepeatCurrent(bool repeat)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    if (std::exchange(m_repeatCurrent, repeat) != repeat)
      changes.push_back({PlayListChange::Type::Repeat, NO_ENTRY, -1, nullptr});
  }
  Notify(changes);
}

bool CPlayList::IsRepeatCurrent() const
{
  std::unique_lock lock(m_critSection);
  return m_repeatCurrent;
}

void CPlayList::SetWrap(Wrap wrap)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    if (m_wrap != wrap)
      changes.push_back({PlayListChange::Type::Wrap, NO_ENTRY, -1, nullptr});
    m_wrap = wrap;
  }
  Notify(changes);
}

Wrap CPlayList::GetWrap() const
{
  std::unique_lock lock(m_critSection);
  return m_wrap;
}

CPlayList::NextPlace CPlayList::NextPlaceLocked() const
{
  const auto lastInsert =
      std::ranges::find_if(m_requests.rbegin(), m_requests.rend(), [this](const Request& request)
                           { return request.inserted && FindLocked(request.entry) >= 0; });
  if (lastInsert != m_requests.rend())
    return {FindLocked(lastInsert->entry) + 1, lastInsert->entry};
  if (const int current = FindLocked(m_current); current >= 0)
    return {current + 1, m_current};
  return {0, m_current};
}

EntryId CPlayList::QueueNextLocked(const std::shared_ptr<CFileItem>& item,
                                   NextPlace& place,
                                   Changes& changes)
{
  const EntryId entry = InsertLocked(item, place.position, changes);
  // in play order too, or the shuffle would carry on from wherever it placed the entry
  m_shuffle->PlayAfter(entry, place.after);
  m_requests.push_back({entry, true});
  place = {place.position + 1, entry};
  return entry;
}

EntryId CPlayList::QueueNext(const std::shared_ptr<CFileItem>& item)
{
  Changes changes;
  EntryId entry;
  {
    std::unique_lock lock(m_critSection);
    NextPlace place = NextPlaceLocked();
    entry = QueueNextLocked(item, place, changes);
  }
  Notify(changes);
  return entry;
}

EntryId CPlayList::QueueNext(const CFileItemList& items)
{
  Changes changes;
  EntryId first = NO_ENTRY;
  {
    std::unique_lock lock(m_critSection);
    // found once: each item then goes straight after the one before it
    NextPlace place = NextPlaceLocked();
    for (int i = 0; i < items.Size(); i++)
    {
      const EntryId entry = QueueNextLocked(items[i], place, changes);
      if (first == NO_ENTRY)
        first = entry;
    }
  }
  Notify(changes);
  return first;
}

void CPlayList::SetShuffle(std::unique_ptr<IPlayListShuffle> shuffle)
{
  {
    std::unique_lock lock(m_critSection);
    m_shuffle = std::move(shuffle);
    ResetShuffle();
  }
  Notify({{PlayListChange::Type::Shuffled, NO_ENTRY, -1, nullptr}});
}

void CPlayList::ResetShuffle()
{
  std::unique_lock lock(m_critSection);
  std::vector<EntryId> listOrder;
  listOrder.reserve(m_entries.size());
  for (const auto& entry : m_entries)
    listOrder.emplace_back(entry.id);

  m_shuffle->Reset(listOrder, m_current);

  // what was asked to play next still does, straight after the current entry
  std::deque<Request> requests = m_requests;
  EntryId after = m_current;
  for (DropStaleRequestsLocked(requests); !requests.empty(); DropStaleRequestsLocked(requests))
  {
    m_shuffle->PlayAfter(requests.front().entry, after);
    after = requests.front().entry;
    requests.pop_front();
  }
}

std::vector<EntryId> CPlayList::GetPlayOrder() const
{
  std::unique_lock lock(m_critSection);
  return m_shuffle->GetOrder();
}

int CPlayList::GetPlayOrderPosition(EntryId entry) const
{
  if (entry == NO_ENTRY)
    return -1;
  std::unique_lock lock(m_critSection);
  return m_shuffle->GetOrderPosition(entry);
}

void CPlayList::SetShuffled(bool shuffled)
{
  if (shuffled == IsShuffled())
    return;

  if (shuffled)
    SetShuffle(std::make_unique<CPlayListRandomShuffle>());
  else
    SetShuffle(std::make_unique<CPlayListNoShuffle>());
}

bool CPlayList::IsShuffled() const
{
  std::unique_lock lock(m_critSection);
  return !m_shuffle->IsListOrder();
}

void CPlayList::ClearSourcePath()
{
  std::unique_lock lock(m_critSection);
  m_sourcePath.clear();
}

std::string CPlayList::GetSourcePath() const
{
  std::unique_lock lock(m_critSection);
  return m_sourcePath;
}

void CPlayList::SetFeed(std::shared_ptr<IFeed> feed)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    if (feed == m_feed)
      return;
    m_feed = std::move(feed);
    changes.push_back({PlayListChange::Type::Feed, NO_ENTRY, -1, nullptr});
  }
  Notify(changes);
}

std::shared_ptr<IFeed> CPlayList::GetFeed() const
{
  std::unique_lock lock(m_critSection);
  return m_feed;
}

void CPlayList::Remove(const std::string& path, const std::vector<EntryId>& keep /* = {} */)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    RemoveIfLocked(
        [&path, &keep](const PlayListEntry& entry)
        {
          return entry.item->GetPath() == path && std::ranges::find(keep, entry.id) == keep.end();
        },
        changes);
  }
  Notify(changes);
}

bool CPlayList::RemoveEntry(EntryId entry, const std::vector<EntryId>& keep /* = {} */)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    const int position = FindLocked(entry);
    if (position < 0 || std::ranges::find(keep, entry) != keep.end())
      return false;
    RemoveLocked(position, changes);
  }
  Notify(changes);
  return true;
}

void CPlayList::ReplaceItem(EntryId entry, const CFileItem& item)
{
  std::unique_lock lock(m_critSection);
  if (const int position = FindLocked(entry); position >= 0)
    m_entries[position].item = std::make_shared<CFileItem>(item);
}

void CPlayList::Remove(int position)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    if (position >= 0 && position < static_cast<int>(m_entries.size()))
      RemoveLocked(position, changes);
  }
  Notify(changes);
}

void CPlayList::RemoveIf(const std::function<bool(const CFileItem&)>& remove)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    RemoveIfLocked([&remove](const PlayListEntry& entry) { return remove(*entry.item); }, changes);
  }
  Notify(changes);
}

bool CPlayList::Swap(int position1, int position2)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    const int size = static_cast<int>(m_entries.size());
    if (position1 < 0 || position2 < 0 || position1 >= size || position2 >= size)
      return false;

    std::swap(m_entries[position1], m_entries[position2]);
    m_shuffle->OnMoved(m_entries[position1].id, position1);
    m_shuffle->OnMoved(m_entries[position2].id, position2);
    changes.push_back({PlayListChange::Type::Moved, m_entries[position1].id, position1, nullptr});
    changes.push_back({PlayListChange::Type::Moved, m_entries[position2].id, position2, nullptr});
  }
  Notify(changes);
  return true;
}

bool CPlayList::Move(int from, int to)
{
  Changes changes;
  {
    std::unique_lock lock(m_critSection);
    const int size = static_cast<int>(m_entries.size());
    if (from < 0 || to < 0 || from >= size || to >= size)
      return false;
    MoveLocked(from, to, changes);
  }
  Notify(changes);
  return true;
}

void CPlayList::MoveLocked(int from, int to, Changes& changes)
{
  if (from == to)
    return;
  const auto first = m_entries.begin();
  if (from < to)
    std::rotate(first + from, first + from + 1, first + to + 1);
  else
    std::rotate(first + to, first + from, first + from + 1);
  for (int position = std::min(from, to); position <= std::max(from, to); ++position)
  {
    m_shuffle->OnMoved(m_entries[position].id, position);
    changes.push_back({PlayListChange::Type::Moved, m_entries[position].id, position, nullptr});
  }
}

void CPlayList::SetUnPlayable(EntryId entry)
{
  std::unique_lock lock(m_critSection);
  const int position = FindLocked(entry);
  if (position < 0)
  {
    CLog::Log(LOGWARNING, "Attempt to set unplayable entry {}", entry);
    return;
  }
  m_entries[position].playable = false;
}

int CPlayList::GetPlayable() const
{
  std::unique_lock lock(m_critSection);
  return static_cast<int>(std::ranges::count(m_entries, true, &PlayListEntry::playable));
}

EntryId CPlayList::ReplaceEntry(EntryId entry,
                                const std::shared_ptr<CFileItem>& held,
                                const std::vector<std::shared_ptr<CFileItem>>& items)
{
  if (items.empty())
    return NO_ENTRY;

  Changes changes;
  EntryId first = NO_ENTRY;
  {
    std::unique_lock lock(m_critSection);
    const int position = FindLocked(entry);
    if (position < 0 || m_entries[position].item != held)
      return NO_ENTRY;

    const bool wasCurrent = entry == m_current;
    RemoveLocked(position, changes);

    int insertAt = position;
    for (const auto& loaded : items)
    {
      const EntryId added = InsertLocked(loaded, insertAt++, changes);
      if (first == NO_ENTRY)
        first = added;
    }
    if (wasCurrent)
      MoveCurrentLocked(first, changes);
  }
  Notify(changes);
  return first;
}

void CPlayList::UpdateItem(const CFileItem& item)
{
  std::unique_lock lock(m_critSection);
  for (auto& entry : m_entries)
  {
    if (!entry.item->IsSamePath(&item))
      continue;
    auto replacement = std::make_shared<CFileItem>(item);
    replacement->SetPath(entry.item->GetPath());
    entry.item = std::move(replacement);
  }
}

} // namespace KODI::PLAYLIST
