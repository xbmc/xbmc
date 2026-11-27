/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PlayListFeed.h"
#include "PlayListTypes.h"
#include "media/MediaStreams.h"
#include "threads/CriticalSection.h"

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class CFileItem;
class CFileItemList;

namespace KODI::PLAYLIST
{

class IPlayListShuffle;

/*!
 * \brief One position in a playlist. Two copies of one item are two entries.
 */
struct PlayListEntry
{
  EntryId id{NO_ENTRY};
  std::shared_ptr<CFileItem> item;
  //! What the item holds, or none if the item does not say.
  std::optional<MEDIA::Streams> streams;
  bool playable{true};
};

/*!
 * \brief What changed, reported to the observer once the playlist has released its lock: an edit
 * to the entries, or a change to the shuffle, the repeat, the wrap, the cursor or the feed.
 */
struct PlayListChange
{
  enum class Type
  {
    Added,
    Removed,
    Moved,
    Cleared,
    Shuffled,
    Repeat,
    Wrap,
    Current,
    Feed
  };

  Type type;
  EntryId entry{NO_ENTRY};
  int position{-1};
  std::shared_ptr<CFileItem> item{};
};

/*!
 * \brief An ordered list of entries, and what plays next.
 *
 * The list is never reordered by shuffling: play order is the business of the installed
 * IPlayListShuffle. The playlist keeps its own cursor, and decides what follows it, in this
 * order: the first queued play-next request, the current entry again when the playlist repeats
 * it and it ended by itself, what the shuffle says follows, the wrap, and otherwise nothing.
 *
 * Editing and reading the entries is safe from any thread. The list owns its items, and they are
 * shared with whoever reads them: nothing changes one in place; UpdateItem() and ReplaceItem()
 * replace it.
 */
class CPlayList final
{
public:
  using Observer = std::function<void(const std::vector<PlayListChange>&)>;

  CPlayList();
  ~CPlayList();
  CPlayList(const CPlayList&) = delete;
  CPlayList& operator=(const CPlayList&) = delete;

  EntryId Add(const std::shared_ptr<CFileItem>& item);
  void Add(const CFileItemList& items);
  void Insert(const CFileItemList& items, int iPosition = -1);
  EntryId Insert(const std::shared_ptr<CFileItem>& item, int iPosition = -1);

  /*!
   * \brief The playlist file, smart playlist or folder the entries were read from, if any. Set by
   * Assign() and cleared with the entries.
   */
  void ClearSourcePath();
  std::string GetSourcePath() const;

  /*!
   * \brief Hold a feed that entries are placed from, or none. Cleared with the entries. The
   * playlist never takes from it: whoever keeps the playlist filled does, outside its lock.
   */
  void SetFeed(std::shared_ptr<IFeed> feed);
  std::shared_ptr<IFeed> GetFeed() const;
  //! Add what a feed placed, in one step, if it is still this playlist's feed.
  void AddFromFeed(const std::vector<std::shared_ptr<CFileItem>>& items,
                   const std::shared_ptr<IFeed>& feed);

  /*!
   * \brief Remove every entry for this path, except the entries to keep.
   */
  void Remove(const std::string& path, const std::vector<EntryId>& keep = {});
  void Remove(int position);
  //! Remove this entry unless it is one to keep. \return Whether it was removed.
  bool RemoveEntry(EntryId entry, const std::vector<EntryId>& keep = {});
  //! Remove every entry whose item this says to.
  void RemoveIf(const std::function<bool(const CFileItem&)>& remove);
  //! Give this entry a copy of the item in place of the one it holds.
  void ReplaceItem(EntryId entry, const CFileItem& item);
  bool Swap(int position1, int position2);
  //! Move the entry at one position in list order to another, shifting those between.
  bool Move(int from, int to);
  /*!
   * \brief Put these items in place of an entry, wherever it now is, if it still holds \p held.
   * The first of them is current if the entry was.
   * \return The first of the new entries, or NO_ENTRY if nothing was replaced.
   */
  EntryId ReplaceEntry(EntryId entry,
                       const std::shared_ptr<CFileItem>& held,
                       const std::vector<std::shared_ptr<CFileItem>>& items);
  void Clear();
  /*!
   * \brief Hold these items, in this order, in one step. When they are the entries already held,
   * rearranged, the entries keep their ids, unplayable flags and play-next requests and report
   * moves. Otherwise the list is rebuilt, keeping its source path: an entry whose item is still
   * listed (the same file at the same offset) keeps its id, and the play order is dealt again,
   * led by the current entry.
   */
  void Replace(const CFileItemList& items);
  //! Hold only these items, in one step: the list is emptied, as by Clear(), and refilled.
  void Assign(const CFileItemList& items, const std::string& sourcePath = "");
  int Size() const;
  bool IsEmpty() const { return Size() == 0; }

  std::shared_ptr<CFileItem> operator[](int iItem) const;

  EntryId GetEntryId(int position) const;
  int GetPosition(EntryId entry) const;
  std::shared_ptr<CFileItem> GetItem(EntryId entry) const;
  std::optional<MEDIA::Streams> GetStreams(EntryId entry) const;

  /*!
   * \return The entries in list order, taken at once, so another thread's edit cannot land midway.
   */
  std::vector<PlayListEntry> GetEntries() const;

  /*!
   * \brief Append the entries' items to the list, in list order, taken at once. The counterpart of
   * Add(const CFileItemList&).
   */
  void GetItems(CFileItemList& items) const;

  EntryId GetCurrent() const;
  int GetCurrentPosition() const;
  std::shared_ptr<CFileItem> GetCurrentItem() const;

  /*!
   * \brief Move the cursor. Consumes the first play-next request if it names this entry; for any
   * other entry the requests still play first, straight after it.
   * \return false if the entry is not in the playlist, and the cursor is left where it was.
   */
  bool SetCurrent(EntryId entry);
  //! Take the cursor off the list; the play-next requests, made relative to it, go too.
  void ClearCurrent();

  /*!
   * \brief What would play after the current entry, without moving to it.
   * \param advance Why the first step is taken; later steps are the list playing on by itself.
   * \param steps How many entries to look ahead.
   */
  EntryId PeekNext(Advance advance, int steps = 1) const;
  //! The entry the given number of steps behind the current one in play order, or NO_ENTRY.
  EntryId PeekPrevious(int steps = 1) const;

  /*!
   * \brief The entry the given number of entries ahead of the current one in play order, or
   * behind it for a negative offset; 0 is the current entry.
   * \param offset How many entries ahead, or behind for a negative offset.
   * \param advance As for PeekNext().
   */
  EntryId PeekOffset(int offset, Advance advance = Advance::Automatic) const;

  /*!
   * \brief Move to what plays after the current entry.
   * \return The new current entry, or NO_ENTRY if nothing follows, in which case the cursor is
   * left where it was.
   */
  EntryId Next(Advance advance);
  EntryId Previous();

  //! Play whatever entry is current again when it ends by itself; a user skip still moves on.
  void SetRepeatCurrent(bool repeat);
  bool IsRepeatCurrent() const;

  void SetWrap(Wrap wrap);
  Wrap GetWrap() const;

  /*!
   * \brief Insert a new item directly after the current entry, or after the last queued insert,
   * and play it next. Requests are served first in, first out, and one whose entry is gone is
   * dropped.
   */
  EntryId QueueNext(const std::shared_ptr<CFileItem>& item);
  /*!
   * \return The first of the entries inserted, or NO_ENTRY if there were none.
   */
  EntryId QueueNext(const CFileItemList& items);

  //! Play in the order this routine deals; it starts over from the whole list.
  void SetShuffle(std::unique_ptr<IPlayListShuffle> shuffle);
  //! Switch between the random order and list order, keeping the routine if it already fits.
  void SetShuffled(bool shuffled);
  bool IsShuffled() const;

  /*!
   * \brief Rebuild the play order from the whole list.
   */
  void ResetShuffle();

  /*!
   * \brief Every entry in play order, for presenting the list as it plays; it is list order when
   * not shuffled. Positions in it are not positions in the list.
   */
  std::vector<EntryId> GetPlayOrder() const;
  //! An entry's place in play order, from 0, or -1 if it is not on the list.
  int GetPlayOrderPosition(EntryId entry) const;

  void SetUnPlayable(EntryId entry);
  int GetPlayable() const;

  /*!
   * \brief Replace every entry's item for this path with a copy of the given one, keeping the
   * entry's own path.
   */
  void UpdateItem(const CFileItem& item);

  /*!
   * \brief Be told of each PlayListChange; replacing an item or marking it unplayable is not
   * reported. Called without the playlist's lock held.
   */
  void SetObserver(Observer observer);

private:
  struct Request
  {
    EntryId entry;
    bool inserted;
  };

  using Changes = std::vector<PlayListChange>;

  //! An entry owning a copy of the item, with no id yet.
  PlayListEntry MakeEntryLocked(const std::shared_ptr<CFileItem>& item) const;
  EntryId InsertLocked(const std::shared_ptr<CFileItem>& item, int position, Changes& changes);
  void RemoveLocked(int position, Changes& changes);
  void RemoveIfLocked(const std::function<bool(const PlayListEntry&)>& remove, Changes& changes);

  void ClearLocked(Changes& changes);
  void MoveCurrentLocked(EntryId entry, Changes& changes);
  void MoveLocked(int from, int to, Changes& changes);
  //! Put the play-next requests, in turn, straight after the current entry.
  void PlaceRequestsLocked(Changes& changes);
  //! Where an item asked to play next goes: its list position, and the entry it plays after.
  struct NextPlace
  {
    int position;
    EntryId after;
  };
  NextPlace NextPlaceLocked() const;
  //! Insert an item to play next at the place, and move the place on past it.
  EntryId QueueNextLocked(const std::shared_ptr<CFileItem>& item,
                          NextPlace& place,
                          Changes& changes);
  int FindLocked(EntryId entry) const;
  EntryId StepLocked(EntryId from, std::deque<Request>& requests, Advance advance) const;
  EntryId StepBackLocked(EntryId from) const;
  void DropStaleRequestsLocked(std::deque<Request>& requests) const;
  void Notify(const Changes& changes) const;

  mutable CCriticalSection m_critSection;
  std::vector<PlayListEntry> m_entries;
  EntryId m_lastId{NO_ENTRY};
  EntryId m_current{NO_ENTRY};
  Wrap m_wrap{Wrap::Off};
  bool m_repeatCurrent{false};
  std::string m_sourcePath;
  std::shared_ptr<IFeed> m_feed;
  std::deque<Request> m_requests;
  std::unique_ptr<IPlayListShuffle> m_shuffle;
  Observer m_observer;
  //! Where each entry was when last looked for; checked on use and rebuilt when out of date.
  mutable std::unordered_map<EntryId, int> m_positions;
};

} // namespace KODI::PLAYLIST
