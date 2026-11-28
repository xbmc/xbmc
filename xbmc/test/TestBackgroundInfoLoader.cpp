/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BackgroundInfoLoader.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "test/MtTestUtils.h"
#include "threads/Event.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

namespace
{
class CRecordingLoader : public CBackgroundInfoLoader
{
public:
  ~CRecordingLoader() override { StopThread(); }

  bool LoadItemCached(CFileItem* item) override { return Record("cached:", *item); }
  bool LoadItemLookup(CFileItem* item) override
  {
    if (item == m_blockLookupOf)
    {
      m_lookupStarted.Set();
      m_continueLookup.Wait();
    }
    return Record("lookup:", *item);
  }

  std::vector<std::string> Calls()
  {
    std::unique_lock lock(m_callsLock);
    return m_calls;
  }

  const CFileItem* m_blockLookupOf{nullptr};
  CEvent m_lookupStarted;
  CEvent m_continueLookup;

private:
  bool Record(const std::string& stage, const CFileItem& item)
  {
    std::unique_lock lock(m_callsLock);
    m_calls.emplace_back(stage + item.GetPath());
    return false;
  }

  CCriticalSection m_callsLock;
  std::vector<std::string> m_calls;
};

void AddItems(CFileItemList& items)
{
  items.Add(std::make_shared<CFileItem>("a", false));
  items.Add(std::make_shared<CFileItem>("b", false));
}
} // namespace

TEST(TestBackgroundInfoLoader, HeldItemIsLoadedAfterRelease)
{
  CFileItemList items;
  AddItems(items);
  CRecordingLoader loader;
  {
    const CBackgroundInfoLoader::CItemHold hold{loader, items[0].get()};
    loader.Load(items);
    ASSERT_TRUE(ConditionPoll::poll([&loader] { return loader.Calls().size() >= 2; }));
    EXPECT_EQ(loader.Calls(), (std::vector<std::string>{"cached:b", "lookup:b"}));
  }
  ASSERT_TRUE(ConditionPoll::poll([&loader] { return !loader.IsLoading(); }));
  EXPECT_EQ(loader.Calls(),
            (std::vector<std::string>{"cached:b", "lookup:b", "cached:a", "lookup:a"}));
}

TEST(TestBackgroundInfoLoader, HoldWaitsOnlyForItemInProgress)
{
  CFileItemList items;
  AddItems(items);
  CRecordingLoader loader;
  loader.m_blockLookupOf = items[0].get();
  loader.Load(items);
  ASSERT_TRUE(loader.m_lookupStarted.Wait(5s));

  {
    // not the item in progress, so no wait
    const CBackgroundInfoLoader::CItemHold hold{loader, items[1].get()};
  }

  std::atomic<bool> held{false};
  std::thread holder(
      [&]
      {
        const CBackgroundInfoLoader::CItemHold hold{loader, items[0].get()};
        held = true;
      });
  std::this_thread::sleep_for(200ms);
  EXPECT_FALSE(held);
  loader.m_continueLookup.Set();
  holder.join();
  EXPECT_TRUE(held);
}

TEST(TestBackgroundInfoLoader, StopWakesWaitForHeldItem)
{
  CFileItemList items;
  AddItems(items);
  CRecordingLoader loader;
  const CBackgroundInfoLoader::CItemHold hold{loader, items[0].get()};
  loader.Load(items);
  ASSERT_TRUE(ConditionPoll::poll([&loader] { return loader.Calls().size() >= 2; }));

  // the loader thread waits for the held item now and must return without it
  loader.StopThread();
  EXPECT_FALSE(loader.IsLoading());
  EXPECT_EQ(loader.Calls(), (std::vector<std::string>{"cached:b", "lookup:b"}));
}

TEST(TestBackgroundInfoLoader, ItemHeldTwiceIsLoadedAfterBothHolds)
{
  CFileItemList items;
  AddItems(items);
  CRecordingLoader loader;
  {
    const CBackgroundInfoLoader::CItemHold outer{loader, items[0].get()};
    {
      const CBackgroundInfoLoader::CItemHold inner{loader, items[0].get()};
      loader.Load(items);
      ASSERT_TRUE(ConditionPoll::poll([&loader] { return loader.Calls().size() >= 2; }));
    }
    std::this_thread::sleep_for(200ms);
    EXPECT_EQ(loader.Calls(), (std::vector<std::string>{"cached:b", "lookup:b"}));
  }
  ASSERT_TRUE(ConditionPoll::poll([&loader] { return !loader.IsLoading(); }));
  EXPECT_EQ(loader.Calls(),
            (std::vector<std::string>{"cached:b", "lookup:b", "cached:a", "lookup:a"}));
}

TEST(TestBackgroundInfoLoader, CallWhenDoneWaitsForStoppedThread)
{
  CFileItemList items;
  AddItems(items);
  CRecordingLoader loader;
  EXPECT_FALSE(loader.CallWhenDone([] {}));

  loader.m_blockLookupOf = items[0].get();
  loader.Load(items);
  ASSERT_TRUE(loader.m_lookupStarted.Wait(5s));
  loader.StopAsync();

  CEvent done;
  std::atomic<bool> onLoaderThread{false};
  std::atomic<bool> loadingWhenCalled{true};
  const std::thread::id caller{std::this_thread::get_id()};
  ASSERT_TRUE(loader.CallWhenDone(
      [&]
      {
        onLoaderThread = std::this_thread::get_id() != caller;
        loadingWhenCalled = loader.IsLoading();
        done.Set();
      }));
  EXPECT_FALSE(done.Wait(200ms));

  loader.m_continueLookup.Set();
  ASSERT_TRUE(done.Wait(5s));
  EXPECT_TRUE(onLoaderThread);
  // a restart from the function must not be deferred again
  EXPECT_FALSE(loadingWhenCalled);
}

TEST(TestBackgroundInfoLoader, StopThreadDropsCallWhenDone)
{
  CFileItemList items;
  AddItems(items);
  CRecordingLoader loader;
  loader.m_blockLookupOf = items[0].get();
  loader.Load(items);
  ASSERT_TRUE(loader.m_lookupStarted.Wait(5s));

  std::atomic<bool> called{false};
  ASSERT_TRUE(loader.CallWhenDone([&called] { called = true; }));
  // StopThread() joins the blocked thread
  std::thread releaser(
      [&loader]
      {
        std::this_thread::sleep_for(200ms);
        loader.m_continueLookup.Set();
      });
  loader.StopThread();
  releaser.join();
  EXPECT_FALSE(called);
}
