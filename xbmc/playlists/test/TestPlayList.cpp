/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "playlists/PlayList.h"
#include "playlists/PlayListShuffle.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace KODI;
using namespace KODI::PLAYLIST;

namespace
{
std::shared_ptr<CFileItem> Item(const std::string& name)
{
  return std::make_shared<CFileItem>("/media/" + name + ".mkv", false);
}

// Plays the list backwards, so that play order and list order are told apart.
class CReverseShuffle : public CPlayListNoShuffle
{
public:
  EntryId Following(EntryId entry) const override { return CPlayListNoShuffle::Preceding(entry); }
  EntryId Preceding(EntryId entry) const override { return CPlayListNoShuffle::Following(entry); }
  void PlayAfter(EntryId entry, EntryId after) override
  {
    std::erase(m_order, entry);
    m_order.insert(std::ranges::find(m_order, after), entry);
  }
  std::vector<EntryId> GetOrder() const override { return {m_order.rbegin(), m_order.rend()}; }
  int GetOrderPosition(EntryId entry) const override
  {
    const int position = CPlayListNoShuffle::GetOrderPosition(entry);
    return position < 0 ? -1 : static_cast<int>(m_order.size()) - 1 - position;
  }
  bool IsListOrder() const override { return false; }
};

class TestPlayList : public ::testing::Test
{
protected:
  // Four entries, a to d, with ids kept in list order.
  void SetUp() override
  {
    for (const char* name : {"a", "b", "c", "d"})
      m_ids.emplace_back(m_playList.Add(Item(name)));
  }

  // The same four entries, as though read from sourcePath, with their new ids.
  void AssignFrom(const std::string& sourcePath)
  {
    CFileItemList items;
    for (const char* name : {"a", "b", "c", "d"})
      items.Add(Item(name));
    m_playList.Assign(items, sourcePath);
    for (int i = 0; i < 4; ++i)
      m_ids[i] = m_playList.GetEntryId(i);
  }

  std::string NameOf(EntryId entry) const
  {
    const auto item = m_playList.GetItem(entry);
    return item ? item->GetPath().substr(7, 1) : "-";
  }

  // The whole play order from its start; the cursor is put back afterwards.
  std::string PlayOrder()
  {
    const EntryId current = m_playList.GetCurrent();
    m_playList.ClearCurrent();
    std::string order;
    for (int step = 1; step <= m_playList.Size(); ++step)
      order += NameOf(m_playList.PeekNext(Advance::Automatic, step));
    m_playList.SetCurrent(current);
    return order;
  }

  CPlayList m_playList;
  std::vector<EntryId> m_ids;
};
} // namespace

TEST_F(TestPlayList, EntryIdsAreNeverReused)
{
  const EntryId removed = m_ids[3];
  m_playList.Remove(3);
  const EntryId added = m_playList.Add(Item("e"));

  EXPECT_NE(removed, added);
  EXPECT_EQ(-1, m_playList.GetPosition(removed));

  m_playList.Clear();
  EXPECT_NE(added, m_playList.Add(Item("f")));
}

TEST_F(TestPlayList, AMoveShiftsTheEntriesBetweenAndKeepsTheirIds)
{
  const auto listOrder = [this]
  {
    std::string order;
    for (int position = 0; position < m_playList.Size(); ++position)
      order += NameOf(m_playList.GetEntryId(position));
    return order;
  };

  EXPECT_TRUE(m_playList.Move(0, 2));
  EXPECT_EQ("bcad", listOrder());
  EXPECT_EQ("bcad", PlayOrder());
  EXPECT_EQ(2, m_playList.GetPosition(m_ids[0]));

  EXPECT_TRUE(m_playList.Move(3, 0));
  EXPECT_EQ("dbca", listOrder());
  EXPECT_FALSE(m_playList.Move(0, 4));
}

TEST_F(TestPlayList, TheEntriesAreTakenInListOrderAndStayAsTaken)
{
  m_playList.SetShuffle(std::make_unique<CReverseShuffle>());
  const std::vector<PlayListEntry> entries = m_playList.GetEntries();
  m_playList.Remove(0);

  ASSERT_EQ(4u, entries.size());
  for (size_t i = 0; i < entries.size(); ++i)
    EXPECT_EQ(m_ids[i], entries[i].id);
}

TEST_F(TestPlayList, ItsItemsAreAppendedInListOrder)
{
  CFileItemList items;
  items.Add(Item("z"));
  m_playList.GetItems(items);

  ASSERT_EQ(5, items.Size());
  EXPECT_EQ("/media/z.mkv", items[0]->GetPath());
  for (int i = 0; i < 4; ++i)
    EXPECT_EQ(m_playList.GetItem(m_ids[i]), items[i + 1]);
}

TEST_F(TestPlayList, TheSourcePathGoesWithTheEntries)
{
  AssignFrom("/media/list.m3u");
  EXPECT_EQ("/media/list.m3u", m_playList.GetSourcePath());

  m_playList.Clear();
  EXPECT_EQ("", m_playList.GetSourcePath());
}

TEST_F(TestPlayList, TwoCopiesOfOneItemAreTwoEntries)
{
  const auto item = Item("e");
  const EntryId first = m_playList.Add(item);
  const EntryId second = m_playList.Add(item);

  EXPECT_NE(first, second);
  EXPECT_EQ(5, m_playList.GetPosition(second));
}

TEST_F(TestPlayList, RepeatingTheCurrentEntryFollowsWhateverIsCurrent)
{
  m_playList.SetCurrent(m_ids[0]);
  m_playList.SetRepeatCurrent(true);
  EXPECT_EQ(m_ids[0], m_playList.Next(Advance::Automatic)) << "it plays again by itself";
  EXPECT_EQ(m_ids[1], m_playList.Next(Advance::User)) << "a skip moves on";
  EXPECT_TRUE(m_playList.IsRepeatCurrent());
  EXPECT_EQ(m_ids[1], m_playList.Next(Advance::Automatic)) << "and the new entry repeats";
}

TEST_F(TestPlayList, NextFromNoCurrentEntryGivesTheFirst)
{
  EXPECT_EQ(m_ids[0], m_playList.Next(Advance::Automatic));
  EXPECT_EQ(m_ids[0], m_playList.GetCurrent());
}

TEST_F(TestPlayList, NextFollowsListOrderAndStopsAtTheEnd)
{
  m_playList.SetCurrent(m_ids[2]);
  EXPECT_EQ(m_ids[3], m_playList.Next(Advance::Automatic));
  EXPECT_EQ(NO_ENTRY, m_playList.Next(Advance::Automatic));
  EXPECT_EQ(m_ids[3], m_playList.GetCurrent()) << "a failed Next() must leave the cursor alone";
}

TEST_F(TestPlayList, PeekingDoesNotMove)
{
  m_playList.SetCurrent(m_ids[1]);
  EXPECT_EQ(m_ids[2], m_playList.PeekNext(Advance::Automatic));
  EXPECT_EQ(m_ids[3], m_playList.PeekNext(Advance::Automatic, 2));
  EXPECT_EQ(m_ids[1], m_playList.GetCurrent());
}

TEST_F(TestPlayList, ARepeatingEntryThatWillNotPlayStops)
{
  m_playList.SetCurrent(m_ids[1]);
  m_playList.SetRepeatCurrent(true);
  m_playList.SetUnPlayable(m_ids[1]);

  EXPECT_EQ(NO_ENTRY, m_playList.PeekNext(Advance::Automatic));
  EXPECT_EQ(m_ids[2], m_playList.PeekNext(Advance::User));
}

TEST_F(TestPlayList, WrapToStartGoesBackToTheFirstEntry)
{
  m_playList.SetWrap(Wrap::ToStart);
  m_playList.SetCurrent(m_ids[3]);
  EXPECT_EQ(m_ids[0], m_playList.Next(Advance::Automatic));
}

TEST_F(TestPlayList, PreviousWrapsOnlyWhenTheListWraps)
{
  m_playList.SetCurrent(m_ids[0]);
  EXPECT_EQ(NO_ENTRY, m_playList.PeekPrevious());

  m_playList.SetWrap(Wrap::ToStart);
  EXPECT_EQ(m_ids[3], m_playList.PeekPrevious());
}

TEST_F(TestPlayList, APlayNextRequestPlaysAndTheListCarriesOnAfterIt)
{
  m_playList.SetCurrent(m_ids[0]);
  const EntryId e = m_playList.QueueNext(Item("e"));

  EXPECT_EQ(e, m_playList.Next(Advance::Automatic));
  EXPECT_EQ(m_ids[1], m_playList.Next(Advance::Automatic));
}

TEST_F(TestPlayList, APlayNextRequestComesBeforeRepeatingTheCurrentEntry)
{
  m_playList.SetCurrent(m_ids[0]);
  m_playList.SetRepeatCurrent(true);
  const EntryId e = m_playList.QueueNext(Item("e"));

  EXPECT_EQ(e, m_playList.Next(Advance::Automatic));
}

TEST_F(TestPlayList, ARequestForARemovedEntryIsDropped)
{
  m_playList.SetCurrent(m_ids[0]);
  const EntryId e = m_playList.QueueNext(Item("e"));
  m_playList.Remove(m_playList.GetPosition(e));

  EXPECT_EQ(m_ids[1], m_playList.Next(Advance::Automatic));
}

TEST_F(TestPlayList, StartingTheRequestedEntryConsumesTheRequest)
{
  m_playList.SetCurrent(m_ids[0]);
  const EntryId e = m_playList.QueueNext(Item("e"));

  // What a player queueing its gapless successor does: look, then move once it has started.
  const EntryId next = m_playList.PeekNext(Advance::Automatic);
  ASSERT_EQ(e, next);
  m_playList.SetCurrent(next);

  EXPECT_EQ(m_ids[1], m_playList.PeekNext(Advance::Automatic)) << "e is not served again";
}

TEST_F(TestPlayList, NewItemsToPlayNextGoAfterTheCurrentEntryInOrder)
{
  m_playList.SetCurrent(m_ids[1]);
  const EntryId e = m_playList.QueueNext(Item("e"));
  const EntryId f = m_playList.QueueNext(Item("f"));

  EXPECT_EQ(2, m_playList.GetPosition(e));
  EXPECT_EQ(3, m_playList.GetPosition(f)) << "a later insert sits after the earlier one";
  EXPECT_EQ(e, m_playList.Next(Advance::Automatic));
  EXPECT_EQ(f, m_playList.Next(Advance::Automatic));
  EXPECT_EQ(m_ids[2], m_playList.Next(Advance::Automatic));
}

TEST_F(TestPlayList, NewItemsToPlayNextComeFirstUnderShuffle)
{
  m_playList.SetCurrent(m_ids[1]);
  m_playList.SetShuffle(std::make_unique<CReverseShuffle>());
  CFileItemList items;
  items.Add(Item("e"));
  m_playList.QueueNext(items);

  EXPECT_EQ("e", NameOf(m_playList.Next(Advance::Automatic)));
}

TEST_F(TestPlayList, ShufflingNeverReordersTheList)
{
  m_playList.SetShuffle(std::make_unique<CReverseShuffle>());

  for (int i = 0; i < 4; i++)
    EXPECT_EQ(m_ids[i], m_playList.GetEntryId(i));
  EXPECT_TRUE(m_playList.IsShuffled());
  EXPECT_EQ("dcba", PlayOrder());
}

TEST_F(TestPlayList, NextFollowsTheShuffle)
{
  m_playList.SetShuffle(std::make_unique<CReverseShuffle>());
  m_playList.SetCurrent(m_ids[2]);
  EXPECT_EQ(m_ids[1], m_playList.Next(Advance::Automatic));
}

TEST_F(TestPlayList, TurningShuffleOffLeavesTheCursorWhereItWas)
{
  m_playList.SetShuffled(true);
  m_playList.SetCurrent(m_ids[2]);
  m_playList.SetShuffled(false);

  EXPECT_FALSE(m_playList.IsShuffled());
  EXPECT_EQ(m_ids[2], m_playList.GetCurrent());
  EXPECT_EQ("abcd", PlayOrder());
}

TEST_F(TestPlayList, RemovingTheCurrentEntryKeepsWhatFollowedIt)
{
  m_playList.SetCurrent(m_ids[1]);
  m_playList.Remove(1);
  EXPECT_EQ(m_ids[2], m_playList.Next(Advance::Automatic));
}

TEST_F(TestPlayList, ClearingLeavesNoCurrentEntry)
{
  m_playList.SetCurrent(m_ids[1]);
  m_playList.QueueNext(Item("e"));
  m_playList.Clear();

  EXPECT_EQ(NO_ENTRY, m_playList.GetCurrent());
  const EntryId added = m_playList.Add(Item("e"));
  EXPECT_EQ(added, m_playList.Next(Advance::Automatic)) << "the old request went with the list";
}

TEST_F(TestPlayList, SwapMovesEntriesNotTheCursor)
{
  m_playList.SetCurrent(m_ids[0]);
  ASSERT_TRUE(m_playList.Swap(0, 3));

  EXPECT_EQ(m_ids[0], m_playList.GetCurrent());
  EXPECT_EQ(3, m_playList.GetCurrentPosition());
  EXPECT_EQ("dbca", PlayOrder());
}

TEST_F(TestPlayList, PeekOffsetLooksBothWays)
{
  m_playList.SetCurrent(m_ids[2]);
  EXPECT_EQ(m_ids[2], m_playList.PeekOffset(0));
  EXPECT_EQ(m_ids[3], m_playList.PeekOffset(1));
  EXPECT_EQ(m_ids[0], m_playList.PeekOffset(-2));
  EXPECT_EQ(NO_ENTRY, m_playList.PeekOffset(2));
}

TEST_F(TestPlayList, GetPlayableCountsEntries)
{
  EXPECT_EQ(4, m_playList.GetPlayable());
  m_playList.SetUnPlayable(m_ids[0]);
  EXPECT_EQ(3, m_playList.GetPlayable());
}

TEST_F(TestPlayList, TheObserverHearsEachEditAfterTheLockIsReleased)
{
  std::vector<PlayListChange> seen;
  m_playList.SetObserver(
      [this, &seen](const std::vector<PlayListChange>& changes)
      {
        // Reading the playlist from the observer must not deadlock or see a half-done edit.
        EXPECT_GE(m_playList.Size(), 0);
        seen.insert(seen.end(), changes.begin(), changes.end());
      });

  const EntryId added = m_playList.Add(Item("e"));
  m_playList.Remove(0);
  m_playList.Clear();

  ASSERT_EQ(3u, seen.size());
  EXPECT_EQ(PlayListChange::Type::Added, seen[0].type);
  EXPECT_EQ(added, seen[0].entry);
  EXPECT_EQ(4, seen[0].position);
  EXPECT_EQ(PlayListChange::Type::Removed, seen[1].type);
  EXPECT_EQ(m_ids[0], seen[1].entry);
  EXPECT_EQ(PlayListChange::Type::Cleared, seen[2].type);
}

TEST_F(TestPlayList, ShuffleRepeatWrapAndCursorChangesAreReported)
{
  std::vector<PlayListChange::Type> seen;
  m_playList.SetObserver(
      [&seen](const std::vector<PlayListChange>& changes)
      {
        for (const auto& change : changes)
          seen.push_back(change.type);
      });

  m_playList.SetCurrent(m_ids[1]);
  m_playList.SetCurrent(m_ids[1]);
  m_playList.SetRepeatCurrent(true);
  m_playList.SetRepeatCurrent(true);
  m_playList.SetWrap(Wrap::ToStart);
  m_playList.SetWrap(Wrap::ToStart);
  m_playList.SetShuffled(true);
  m_playList.Next(Advance::User);
  m_playList.SetRepeatCurrent(false);

  using enum PlayListChange::Type;
  const std::vector<PlayListChange::Type> expected{Current,  Repeat,  Wrap,
                                                   Shuffled, Current, Repeat};
  EXPECT_EQ(expected, seen) << "setting what is already set reports nothing";
}

TEST_F(TestPlayList, UpdatingAnItemReplacesItRatherThanChangingTheSharedOne)
{
  const std::shared_ptr<CFileItem> shared = m_playList.GetItem(m_ids[0]);
  CFileItem changed(*shared);
  changed.SetLabel("changed");
  m_playList.UpdateItem(changed);

  EXPECT_NE("changed", shared->GetLabel()) << "a reader's item is not changed under it";
  EXPECT_EQ("changed", m_playList.GetItem(m_ids[0])->GetLabel());
  EXPECT_EQ(shared->GetPath(), m_playList.GetItem(m_ids[0])->GetPath());
}

TEST(TestPlayListRandomShuffle, TheCurrentEntryLeadsAndEveryEntryIsPlayedOnce)
{
  CPlayListRandomShuffle shuffle;
  const std::vector<EntryId> listOrder{1, 2, 3, 4, 5, 6, 7, 8};
  shuffle.Reset(listOrder, 5);

  std::vector<EntryId> order;
  for (EntryId entry = shuffle.Following(NO_ENTRY); entry != NO_ENTRY;
       entry = shuffle.Following(entry))
    order.emplace_back(entry);

  ASSERT_EQ(listOrder.size(), order.size());
  EXPECT_EQ(5u, order.front());
  std::ranges::sort(order);
  EXPECT_EQ(listOrder, order);
}

TEST(TestPlayListRandomShuffle, AnAddedEntryComesAfterTheCurrentOne)
{
  for (int attempt = 0; attempt < 20; attempt++)
  {
    CPlayListRandomShuffle shuffle;
    shuffle.Reset({1, 2, 3, 4}, 3);
    shuffle.OnAdded(9, 4, 3);

    bool seenCurrent = false;
    for (EntryId entry = shuffle.Following(NO_ENTRY); entry != NO_ENTRY;
         entry = shuffle.Following(entry))
    {
      if (entry == 3)
        seenCurrent = true;
      if (entry == 9)
      {
        EXPECT_TRUE(seenCurrent) << "the new entry was placed among those already played";
      }
    }
  }
}

TEST_F(TestPlayList, PositionsFollowEveryEdit)
{
  m_playList.Remove(1);
  const EntryId e = m_playList.Insert(Item("e"), 0);
  EXPECT_TRUE(m_playList.Swap(1, 3));

  for (int position = 0; position < m_playList.Size(); ++position)
  {
    EXPECT_EQ(position, m_playList.GetPosition(m_playList.GetEntryId(position)));
    EXPECT_EQ(position, m_playList.GetPlayOrderPosition(m_playList.GetEntryId(position)));
  }
  EXPECT_EQ(0, m_playList.GetPosition(e));
  EXPECT_EQ(-1, m_playList.GetPosition(m_ids[1]));
  EXPECT_EQ(-1, m_playList.GetPlayOrderPosition(m_ids[1]));
}

TEST_F(TestPlayList, ThePlayOrderIsListOrderUnlessShuffled)
{
  EXPECT_EQ(m_ids, m_playList.GetPlayOrder());
  EXPECT_EQ(2, m_playList.GetPlayOrderPosition(m_ids[2]));

  m_playList.SetShuffle(std::make_unique<CReverseShuffle>());
  EXPECT_EQ(std::vector<EntryId>(m_ids.rbegin(), m_ids.rend()), m_playList.GetPlayOrder());
  EXPECT_EQ(0, m_playList.GetPlayOrderPosition(m_ids[3]));
  EXPECT_EQ(-1, m_playList.GetPlayOrderPosition(NO_ENTRY));
}

TEST(TestPlayListRandomShuffle, PlayingAnEntryNextSkipsNothing)
{
  for (int attempt = 0; attempt < 20; attempt++)
  {
    CPlayList playList;
    for (const char* name : {"a", "b", "c", "d"})
      playList.Add(Item(name));
    playList.SetCurrent(playList.GetEntryId(0));
    playList.SetShuffled(true);
    const EntryId queued = playList.QueueNext(Item("e"));
    EXPECT_EQ(1, playList.GetPlayOrderPosition(queued)) << "the play order shows it next";

    std::vector<EntryId> played{playList.GetCurrent()};
    for (EntryId entry = playList.Next(Advance::Automatic); entry != NO_ENTRY;
         entry = playList.Next(Advance::Automatic))
      played.emplace_back(entry);

    ASSERT_EQ(5u, played.size()) << "every entry plays once";
    EXPECT_EQ(queued, played[1]);
    std::ranges::sort(played);
    EXPECT_EQ(played.end(), std::ranges::adjacent_find(played));
  }
}

TEST(TestPlayListTypes, APlaylistIsNamedAsVideoOrAsAudioOrMusic)
{
  EXPECT_EQ(PLAYLIST::Video, PLAYLIST::TypeFromName("video"));
  EXPECT_EQ(PLAYLIST::Audio, PLAYLIST::TypeFromName("Audio"));
  EXPECT_EQ(PLAYLIST::Audio, PLAYLIST::TypeFromName("MUSIC"));
  EXPECT_EQ(std::nullopt, PLAYLIST::TypeFromName("picture"));
  EXPECT_EQ(std::nullopt, PLAYLIST::TypeFromName(""));
}

TEST_F(TestPlayList, AnAddedItemIsTheListsOwnCopy)
{
  const auto item = Item("e");
  const EntryId entry = m_playList.Add(item);

  item->SetLabel("changed by its window");

  EXPECT_NE(item, m_playList.GetItem(entry));
  EXPECT_EQ("", m_playList.GetItem(entry)->GetLabel());
}

TEST_F(TestPlayList, RemovingAnEntryRemovesThatEntryWhereverItHasMoved)
{
  m_playList.Move(3, 0);

  EXPECT_TRUE(m_playList.RemoveEntry(m_ids[3]));
  EXPECT_EQ(3, m_playList.Size());
  EXPECT_EQ(-1, m_playList.GetPosition(m_ids[3]));
  EXPECT_EQ(0, m_playList.GetPosition(m_ids[0]));
}

TEST_F(TestPlayList, AnEntryToKeepIsNotRemoved)
{
  EXPECT_FALSE(m_playList.RemoveEntry(m_ids[1], {m_ids[1]}));
  EXPECT_FALSE(m_playList.RemoveEntry(NO_ENTRY));
  EXPECT_EQ(4, m_playList.Size());
}

TEST_F(TestPlayList, RemovingIfTakesEveryEntryItSaysTo)
{
  m_playList.RemoveIf(
      [](const CFileItem& item)
      { return item.GetPath() == "/media/b.mkv" || item.GetPath() == "/media/d.mkv"; });

  EXPECT_EQ(2, m_playList.Size());
  EXPECT_EQ(0, m_playList.GetPosition(m_ids[0]));
  EXPECT_EQ(1, m_playList.GetPosition(m_ids[2]));
}

TEST_F(TestPlayList, AReplacedEntryHandsItsPlaceAndTheCursorToTheFirstItem)
{
  m_playList.SetCurrent(m_ids[1]);
  const auto held = m_playList.GetItem(m_ids[1]);

  const EntryId first = m_playList.ReplaceEntry(m_ids[1], held, {Item("x"), Item("y")});

  EXPECT_EQ("axycd", NameOf(m_playList.GetEntryId(0)) + NameOf(m_playList.GetEntryId(1)) +
                         NameOf(m_playList.GetEntryId(2)) + NameOf(m_playList.GetEntryId(3)) +
                         NameOf(m_playList.GetEntryId(4)));
  EXPECT_EQ(first, m_playList.GetCurrent());
  EXPECT_EQ(1, m_playList.GetPosition(first));
}

TEST_F(TestPlayList, AnEntryHoldingAnotherItemIsNotReplaced)
{
  m_playList.ReplaceItem(m_ids[1], *Item("z"));

  EXPECT_EQ(NO_ENTRY, m_playList.ReplaceEntry(m_ids[1], Item("b"), {Item("x")}));
  EXPECT_EQ(4, m_playList.Size());
}

TEST_F(TestPlayList, ReplacingAnItemTouchesOnlyItsEntry)
{
  // two tracks of one cue sheet: the same file at different offsets
  auto first = Item("album");
  first->SetStartOffset(0);
  auto second = Item("album");
  second->SetStartOffset(60000);
  const EntryId one = m_playList.Add(first);
  const EntryId two = m_playList.Add(second);

  CFileItem replacement(*m_playList.GetItem(two));
  replacement.SetStartOffset(0);
  m_playList.ReplaceItem(two, replacement);

  EXPECT_EQ(0, m_playList.GetItem(two)->GetStartOffset());
  EXPECT_EQ(0, m_playList.GetItem(one)->GetStartOffset());
  EXPECT_EQ(60000, second->GetStartOffset()) << "the caller's item is not the entry";
}

TEST_F(TestPlayList, RearrangingTheSameItemsKeepsTheEntries)
{
  AssignFrom("/media/");
  m_playList.SetCurrent(m_ids[1]);

  CFileItemList sorted;
  for (const char* name : {"d", "c", "b", "a"})
    sorted.Add(Item(name));
  m_playList.Replace(sorted);

  EXPECT_EQ(m_ids[3], m_playList.GetEntryId(0));
  EXPECT_EQ(m_ids[0], m_playList.GetEntryId(3));
  EXPECT_EQ(m_ids[1], m_playList.GetCurrent());
  EXPECT_EQ("/media/", m_playList.GetSourcePath());
}

TEST_F(TestPlayList, ReplacingWithOtherItemsKeepsTheCurrentFileCurrent)
{
  AssignFrom("/media/");
  m_playList.SetCurrent(m_ids[1]);

  CFileItemList other;
  other.Add(Item("e"));
  other.Add(Item("b"));
  m_playList.Replace(other);

  ASSERT_EQ(2, m_playList.Size());
  EXPECT_EQ("b", NameOf(m_playList.GetCurrent()));
  EXPECT_EQ("/media/", m_playList.GetSourcePath());
}

TEST_F(TestPlayList, ReplacingWithOtherItemsKeepsTheEntriesStillListed)
{
  m_playList.SetCurrent(m_ids[1]);

  CFileItemList other;
  for (const char* name : {"c", "e", "b"})
    other.Add(Item(name));
  m_playList.Replace(other);

  EXPECT_EQ(m_ids[2], m_playList.GetEntryId(0)) << "what refers to an entry still finds it";
  EXPECT_EQ(m_ids[1], m_playList.GetEntryId(2));
  EXPECT_EQ(m_ids[1], m_playList.GetCurrent());
  EXPECT_EQ(-1, m_playList.GetPosition(m_ids[0]));
}

TEST_F(TestPlayList, ReplacingAShuffledListDealsAgainFromTheCurrentEntry)
{
  m_playList.SetShuffle(std::make_unique<CPlayListRandomShuffle>());
  m_playList.SetCurrent(m_ids[2]);

  CFileItemList other;
  for (const char* name : {"a", "b", "c", "e", "f", "g", "h"})
    other.Add(Item(name));
  m_playList.Replace(other);

  ASSERT_EQ(m_ids[2], m_playList.GetCurrent());
  EXPECT_EQ(m_ids[2], m_playList.GetPlayOrder().front());
}

TEST_F(TestPlayList, AssigningEmptiesAndRefillsInOneStep)
{
  m_playList.SetCurrent(m_ids[1]);

  CFileItemList items;
  items.Add(Item("e"));
  m_playList.Assign(items, "/elsewhere/");

  ASSERT_EQ(1, m_playList.Size());
  EXPECT_EQ(NO_ENTRY, m_playList.GetCurrent());
  EXPECT_EQ("/elsewhere/", m_playList.GetSourcePath());
}

TEST_F(TestPlayList, PlayNextRequestsDoNotOutliveTheCursor)
{
  m_playList.SetCurrent(m_ids[1]);
  m_playList.QueueNext(Item("e"));

  m_playList.ClearCurrent();

  EXPECT_EQ(m_ids[0], m_playList.PeekNext(Advance::User)) << "playing from the start";
}

TEST_F(TestPlayList, AfterAJumpThePlayNextRequestsFollowTheNewEntry)
{
  m_playList.SetCurrent(m_ids[0]);
  const EntryId e = m_playList.QueueNext(Item("e"));

  m_playList.SetCurrent(m_ids[2]);

  EXPECT_EQ(e, m_playList.Next(Advance::Automatic));
  EXPECT_EQ(m_ids[3], m_playList.Next(Advance::Automatic)) << "the skipped entries stay skipped";
  EXPECT_EQ(3, m_playList.GetPosition(e)) << "the list shows the order it plays in";
}

TEST_F(TestPlayList, AfterAJumpUnderShuffleThePlayNextRequestsFollowTheNewEntry)
{
  m_playList.SetShuffle(std::make_unique<CReverseShuffle>());
  m_playList.SetCurrent(m_ids[3]);
  const EntryId e = m_playList.QueueNext(Item("e"));

  m_playList.SetCurrent(m_ids[1]);

  EXPECT_EQ(e, m_playList.Next(Advance::Automatic));
  EXPECT_EQ(m_ids[0], m_playList.Next(Advance::Automatic)) << "the skipped entries stay skipped";
}

TEST_F(TestPlayList, JumpingToALaterRequestLeavesTheEarlierOnesToPlayNext)
{
  m_playList.SetCurrent(m_ids[0]);
  const EntryId e = m_playList.QueueNext(Item("e"));
  const EntryId f = m_playList.QueueNext(Item("f"));

  m_playList.SetCurrent(f);

  EXPECT_EQ(e, m_playList.Next(Advance::Automatic));
  EXPECT_EQ(m_ids[1], m_playList.Next(Advance::Automatic));
}
