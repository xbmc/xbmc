#pragma once

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

#include "threads/Thread.h"
#include "IProgressCallback.h"
#include "threads/CriticalSection.h"

#include <vector>
#include <memory>

class CFileItem; typedef std::shared_ptr<CFileItem> CFileItemPtr;
class CFileItemList;

class IBackgroundLoaderObserver
{
public:
  virtual ~IBackgroundLoaderObserver() {}
  virtual void OnItemLoaded(CFileItem* pItem) = 0;
};

class CBackgroundInfoLoader : public IRunnable
{
public:
  CBackgroundInfoLoader();
  virtual ~CBackgroundInfoLoader();

  void Load(CFileItemList& items);
  bool IsLoading();
  virtual void Run();
  void SetObserver(IBackgroundLoaderObserver* pObserver);
  void SetProgressCallback(IProgressCallback* pCallback);
  virtual bool LoadItem(CFileItem* pItem) { return false; };
  virtual bool LoadItemCached(CFileItem* pItem) { return false; };
  virtual bool LoadItemLookup(CFileItem* pItem) { return false; };

  void StopThread(); // will actually stop the loader thread.
  void StopAsync();  // will ask loader to stop as soon as possible, but not block

protected:
  virtual void OnLoaderStart() {};
  virtual void OnLoaderFinish() {};

  CFileItemList *m_pVecItems;
  std::vector<CFileItemPtr> m_vecItems; // FileItemList would delete the items and we only want to keep a reference.
  CCriticalSection m_lock;

  volatile bool m_bIsLoading;
  volatile bool m_bStop;
  CThread *m_thread;

  IBackgroundLoaderObserver* m_pObserver;
  IProgressCallback* m_pProgressCallback;
};

