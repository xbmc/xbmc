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
#include "threads/Thread.h"
#include "utils/log.h"

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

      // Stage 1: All "fast" stuff we have already cached
      for (std::vector<CFileItemPtr>::const_iterator iter = m_vecItems.begin(); iter != m_vecItems.end(); ++iter)
      {
        const CFileItemPtr& pItem = *iter;

        // Ask the callback if we should abort
        if ((m_pProgressCallback && m_pProgressCallback->Abort()) || m_bStop)
          break;

        try
        {
          if (LoadItemCached(pItem.get()) && m_pObserver)
            m_pObserver->OnItemLoaded(pItem.get());
        }
        catch (...)
        {
          CLog::Log(LOGERROR,
                    "CBackgroundInfoLoader::LoadItemCached - Unhandled exception for item {}",
                    CURL::GetRedacted(pItem->GetPath()));
        }
      }

      // Stage 2: All "slow" stuff that we need to lookup
      for (std::vector<CFileItemPtr>::const_iterator iter = m_vecItems.begin(); iter != m_vecItems.end(); ++iter)
      {
        const CFileItemPtr& pItem = *iter;

        // Ask the callback if we should abort
        if ((m_pProgressCallback && m_pProgressCallback->Abort()) || m_bStop)
          break;

        try
        {
          if (LoadItemLookup(pItem.get()) && m_pObserver)
            m_pObserver->OnItemLoaded(pItem.get());
        }
        catch (...)
        {
          CLog::Log(LOGERROR,
                    "CBackgroundInfoLoader::LoadItemLookup - Unhandled exception for item {}",
                    CURL::GetRedacted(pItem->GetPath()));
        }
      }
    }

    OnLoaderFinish();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} - Unhandled exception", __FUNCTION__);
  }

  Reset();
}

void CBackgroundInfoLoader::Load(CFileItemList& items)
{
  StopThread();

  if (items.IsEmpty())
    return;

  std::unique_lock<CCriticalSection> lock(m_lock);

  for (int nItem=0; nItem < items.Size(); nItem++)
    m_vecItems.push_back(items[nItem]);

  m_pVecItems = &items;
  m_bStop = false;
  m_bIsLoading = true;

  m_thread = new CThread(this, "BackgroundLoader");
  m_thread->Create();
  m_thread->SetPriority(ThreadPriority::BELOW_NORMAL);
}

void CBackgroundInfoLoader::StopAsync()
{
  m_bStop = true;
}


void CBackgroundInfoLoader::StopThread()
{
  StopAsync();

  if (m_thread)
  {
    m_thread->StopThread();
    delete m_thread;
    m_thread = NULL;
  }
  Reset();
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

