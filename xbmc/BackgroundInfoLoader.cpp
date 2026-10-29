/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BackgroundInfoLoader.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "application/Application.h"
#include "threads/Thread.h"
#include "utils/log.h"

#include <algorithm>
#include <mutex>

CBackgroundInfoLoader::CBackgroundInfoLoader() = default;

CBackgroundInfoLoader::~CBackgroundInfoLoader()
{
  StopThread();
}

void CBackgroundInfoLoader::Reset()
{
  m_pVecItems = nullptr;
  m_vecItems.clear();
  m_bIsLoading = false;
}

void CBackgroundInfoLoader::Run()
{
  try
  {
    if (!m_vecItems.empty())
    {
      OnLoaderStart();
      RunStages();
    }

    OnLoaderFinish();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} - Unhandled exception", __FUNCTION__);
  }

  Reset();
}

void CBackgroundInfoLoader::RunStages()
{
  std::vector<CFileItemPtr> heldBeforeCached;
  std::vector<CFileItemPtr> heldBeforeLookup;
  const auto putAside{[](std::vector<CFileItemPtr>& items, const CFileItemPtr& item)
                      {
                        if (std::ranges::find(items, item) == items.end())
                          items.emplace_back(item);
                      }};

  // Stage 1: All "fast" stuff we have already cached
  for (const auto& item : m_vecItems)
  {
    const ItemState state{ProcessItem(Stage::CACHED, item.get(), false)};
    if (state == ItemState::STOPPED)
      return;
    if (state == ItemState::HELD)
      putAside(heldBeforeCached, item);
  }

  // their lookup follows their cached stuff, in LoadReleasedItems() or in stage 3
  const std::vector<CFileItemPtr> heldInStage1{heldBeforeCached};

  // Stage 2: All "slow" stuff that we need to lookup
  for (const auto& item : m_vecItems)
  {
    if (!LoadReleasedItems(heldBeforeCached, heldBeforeLookup))
      return;

    if (std::ranges::find(heldInStage1, item) != heldInStage1.end())
      continue;

    const ItemState state{ProcessItem(Stage::LOOKUP, item.get(), false)};
    if (state == ItemState::STOPPED)
      return;
    if (state == ItemState::HELD)
      putAside(heldBeforeLookup, item);
  }

  // Stage 3: The items still held, once they are released
  for (const auto& item : heldBeforeCached)
  {
    if (ProcessItem(Stage::CACHED, item.get(), true) == ItemState::STOPPED ||
        ProcessItem(Stage::LOOKUP, item.get(), true) == ItemState::STOPPED)
      return;
  }
  for (const auto& item : heldBeforeLookup)
  {
    if (ProcessItem(Stage::LOOKUP, item.get(), true) == ItemState::STOPPED)
      return;
  }
}

bool CBackgroundInfoLoader::LoadReleasedItems(std::vector<CFileItemPtr>& heldBeforeCached,
                                              std::vector<CFileItemPtr>& heldBeforeLookup)
{
  for (auto it = heldBeforeCached.begin(); it != heldBeforeCached.end();)
  {
    const ItemState state{ProcessItem(Stage::CACHED, it->get(), false)};
    if (state == ItemState::STOPPED)
      return false;
    if (state == ItemState::HELD)
    {
      ++it;
      continue;
    }

    const CFileItemPtr item{*it};
    it = heldBeforeCached.erase(it);

    const ItemState lookupState{ProcessItem(Stage::LOOKUP, item.get(), false)};
    if (lookupState == ItemState::STOPPED)
      return false;
    if (lookupState == ItemState::HELD)
      heldBeforeLookup.emplace_back(item);
  }

  for (auto it = heldBeforeLookup.begin(); it != heldBeforeLookup.end();)
  {
    const ItemState state{ProcessItem(Stage::LOOKUP, it->get(), false)};
    if (state == ItemState::STOPPED)
      return false;
    if (state == ItemState::HELD)
      ++it;
    else
      it = heldBeforeLookup.erase(it);
  }

  return true;
}

CBackgroundInfoLoader::ItemState CBackgroundInfoLoader::ProcessItem(Stage stage,
                                                                    CFileItem* item,
                                                                    bool waitWhileHeld)
{
  // Ask the callback if we should abort
  if ((m_pProgressCallback && m_pProgressCallback->Abort()) || m_bStop)
    return ItemState::STOPPED;

  const ItemState state{BeginItem(item, waitWhileHeld)};
  if (state != ItemState::STARTED)
    return state;

  // released after the observer, which may change the item as well
  class CItemEnd
  {
  public:
    explicit CItemEnd(CBackgroundInfoLoader& loader) : m_loader(loader) {}
    ~CItemEnd() { m_loader.EndItem(); }

    CItemEnd(const CItemEnd&) = delete;
    CItemEnd& operator=(const CItemEnd&) = delete;

  private:
    CBackgroundInfoLoader& m_loader;
  };
  const CItemEnd itemEnd{*this};

  const char* name{stage == Stage::CACHED ? "LoadItemCached" : "LoadItemLookup"};
  try
  {
    const bool loaded{stage == Stage::CACHED ? LoadItemCached(item) : LoadItemLookup(item)};
    if (loaded && m_pObserver)
      m_pObserver->OnItemLoaded(item);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CBackgroundInfoLoader::{} - Unhandled exception for item {}", name,
              CURL::GetRedacted(item->GetPath()));
  }

  return ItemState::STARTED;
}

void CBackgroundInfoLoader::Load(CFileItemList& items)
{
  // Never start a background load while the application is shutting down.
  if (g_application.IsStopping())
    return;

  StopThread();

  if (items.IsEmpty())
    return;

  std::unique_lock lock(m_lock);

  std::ranges::copy(items, std::back_inserter(m_vecItems));

  m_pVecItems = &items;
  m_bStop = false;
  m_bIsLoading = true;

  m_thread = std::make_unique<CThread>(this, "BackgroundLoader");
  m_thread->Create();
  m_thread->SetPriority(ThreadPriority::BELOW_NORMAL);
}

void CBackgroundInfoLoader::StopAsync()
{
  std::unique_lock lock(m_itemLock);
  m_bStop = true;
  // a loader thread waiting for a held item has to stop as well
  m_itemChanged.notifyAll();
}

void CBackgroundInfoLoader::StopThread()
{
  StopAsync();

  if (m_thread)
  {
    m_thread->StopThread();
    m_thread.reset();
  }
  Reset();
}

CBackgroundInfoLoader::ItemState CBackgroundInfoLoader::BeginItem(const CFileItem* item,
                                                                  bool waitWhileHeld)
{
  std::unique_lock lock(m_itemLock);
  const auto isHeld{[this, item]
                    { return std::ranges::find(m_heldItems, item) != m_heldItems.end(); }};
  if (waitWhileHeld)
    m_itemChanged.wait(lock, [this, &isHeld] { return m_bStop || !isHeld(); });

  if (m_bStop)
    return ItemState::STOPPED;
  if (isHeld())
    return ItemState::HELD;

  m_currentItem = item;
  return ItemState::STARTED;
}

void CBackgroundInfoLoader::EndItem()
{
  std::unique_lock lock(m_itemLock);
  m_currentItem = nullptr;
  m_itemChanged.notifyAll();
}

CBackgroundInfoLoader::CItemHold::CItemHold(CBackgroundInfoLoader& loader, const CFileItem* item)
  : m_loader(loader),
    m_item(item)
{
  if (!m_item)
    return;

  std::unique_lock lock(m_loader.m_itemLock);
  m_loader.m_heldItems.emplace_back(m_item);
  // the loader thread may be in the middle of this item
  m_loader.m_itemChanged.wait(lock, [this] { return m_loader.m_currentItem != m_item; });
}

CBackgroundInfoLoader::CItemHold::~CItemHold()
{
  if (!m_item)
    return;

  std::unique_lock lock(m_loader.m_itemLock);
  // the same item can be held more than once, release this hold only
  if (const auto it{std::ranges::find(m_loader.m_heldItems, m_item)};
      it != m_loader.m_heldItems.end())
    m_loader.m_heldItems.erase(it);
  m_loader.m_itemChanged.notifyAll();
}

bool CBackgroundInfoLoader::IsLoading()
{
  return m_bIsLoading;
}

void CBackgroundInfoLoader::SetObserver(IBackgroundLoaderObserver* pObserver)
{
  m_pObserver = pObserver;
}

void CBackgroundInfoLoader::SetProgressCallback(IProgressCallback* pCallback)
{
  m_pProgressCallback = pCallback;
}
