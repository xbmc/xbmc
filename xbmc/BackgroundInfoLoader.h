/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IProgressCallback.h"
#include "threads/Condition.h"
#include "threads/CriticalSection.h"
#include "threads/IRunnable.h"

#include <memory>
#include <vector>

class CFileItem; typedef std::shared_ptr<CFileItem> CFileItemPtr;
class CFileItemList;
class CThread;

class IBackgroundLoaderObserver
{
public:
  virtual ~IBackgroundLoaderObserver() = default;
  virtual void OnItemLoaded(CFileItem* pItem) = 0;
};

class CBackgroundInfoLoader : public IRunnable
{
public:
  CBackgroundInfoLoader();
  ~CBackgroundInfoLoader() override;

  void Load(CFileItemList& items);
  bool IsLoading();
  void Run() override;
  void SetObserver(IBackgroundLoaderObserver* pObserver);
  void SetProgressCallback(IProgressCallback* pCallback);
  virtual bool LoadItem(CFileItem* pItem) { return false; }
  virtual bool LoadItemCached(CFileItem* pItem) { return false; }
  virtual bool LoadItemLookup(CFileItem* pItem) { return false; }

  void StopThread(); // will actually stop the loader thread.
  void StopAsync();  // will ask loader to stop as soon as possible, but not block

  /*!
   \brief Keeps the loader thread away from an item for as long as it lives, so that another thread
   can read or change the item meanwhile. Waits only while the loader thread is working on that
   very item. The loader thread puts a held item aside and goes on with the other items, and loads
   it between them once it is released, at the latest after them.
   \note The item has to outlive the hold. A null item is not held.
   \note While the loader thread waits for a held item after the others, only stopping the loader
   wakes it; an abort of the progress callback does not.
   */
  class CItemHold
  {
  public:
    CItemHold(CBackgroundInfoLoader& loader, const CFileItem* item);
    ~CItemHold();

    CItemHold(const CItemHold&) = delete;
    CItemHold& operator=(const CItemHold&) = delete;

  private:
    CBackgroundInfoLoader& m_loader;
    const CFileItem* m_item{nullptr};
  };

protected:
  virtual void OnLoaderStart() {}
  virtual void OnLoaderFinish() {}

  CFileItemList* m_pVecItems{nullptr};
  std::vector<CFileItemPtr> m_vecItems; // FileItemList would delete the items and we only want to keep a reference.
  CCriticalSection m_lock;

  volatile bool m_bIsLoading{false};
  volatile bool m_bStop{true};
  std::unique_ptr<CThread> m_thread{};

  IBackgroundLoaderObserver* m_pObserver{nullptr};
  IProgressCallback* m_pProgressCallback{nullptr};

private:
  enum class Stage
  {
    CACHED,
    LOOKUP
  };

  enum class ItemState
  {
    STARTED,
    HELD,
    STOPPED
  };

  void Reset();
  void RunStages();
  bool LoadReleasedItems(std::vector<CFileItemPtr>& heldBeforeCached,
                         std::vector<CFileItemPtr>& heldBeforeLookup);
  ItemState ProcessItem(Stage stage, CFileItem* item, bool waitWhileHeld);
  ItemState BeginItem(const CFileItem* item, bool waitWhileHeld);
  void EndItem();

  CCriticalSection m_itemLock;
  XbmcThreads::ConditionVariable m_itemChanged;
  const CFileItem* m_currentItem{nullptr}; //!< item the loader thread is working on
  std::vector<const CFileItem*> m_heldItems; //!< items the loader thread has to leave alone
};

