/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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

#include "BackgroundInfoLoader.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

using namespace std;

#define ITEMS_PER_THREAD 5

CBackgroundInfoLoader::CBackgroundInfoLoader(int nThreads)
{
  m_bStop = true;
  m_pObserver=NULL;
  m_pProgressCallback=NULL;
  m_pVecItems = NULL;
  m_nRequestedThreads = nThreads;
  m_bStartCalled = false;
  m_bFinishCalled = false;
  m_nActiveThreads = 0;
}

CBackgroundInfoLoader::~CBackgroundInfoLoader()
{
  StopThread();
}

void CBackgroundInfoLoader::SetNumOfWorkers(int nThreads)
{
  m_nRequestedThreads = nThreads;
}

void CBackgroundInfoLoader::Run()
{
  try
  {
    if (m_vecItems.size() > 0)
    {
      {
        CSingleLock lock(m_lock);
        if (!m_bStartCalled)
        {
          OnLoaderStart();
          m_bStartCalled = true;
        }
      }

      while (!m_bStop && !CThread::IsCurrentThreadStopping())
      {
        CSingleLock lock(m_lock);
        CFileItemPtr pItem;
        vector<CFileItemPtr>::iterator iter = m_vecItems.begin();
        if (iter != m_vecItems.end())
        {
          pItem = *iter;
          m_vecItems.erase(iter);
        }

        if (pItem == NULL)
          break;

        // Ask the callback if we should abort
        if ((m_pProgressCallback && m_pProgressCallback->Abort()) || m_bStop)
          break;

        lock.Leave();
        try
        {
          if (LoadItem(pItem.get()) && !CThread::IsCurrentThreadStopping() && m_pObserver)
            m_pObserver->OnItemLoaded(pItem.get());
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "%s::LoadItem - Unhandled exception for item %s", __FUNCTION__, pItem->GetPath().c_str());
        }
      }
    }

    CSingleLock lock(m_lock);
    if (!m_bFinishCalled && m_nActiveThreads == 1)
    {
      OnLoaderFinish();
      m_bFinishCalled = true;
    }
    m_nActiveThreads--;
  }
  catch (...)
  {
    m_nActiveThreads--;
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
}

void CBackgroundInfoLoader::Load(CFileItemList& items)
{
  CSingleLock lock(m_lock);

  StopAsync(true);

  if (items.Size() == 0)
    return;

  for (int nItem=0; nItem < items.Size(); nItem++)
    m_vecItems.push_back(items[nItem]);

  m_pVecItems = &items;
  m_bStop = false;
  m_bStartCalled = false;
  m_bFinishCalled = false;

  int nThreads = m_nRequestedThreads;
  if (nThreads == -1)
    nThreads = (m_vecItems.size() / (ITEMS_PER_THREAD+1)) + 1;

  if (nThreads > g_advancedSettings.m_bgInfoLoaderMaxThreads)
    nThreads = g_advancedSettings.m_bgInfoLoaderMaxThreads;

  m_nActiveThreads += nThreads;
  for (int i=0; i < nThreads; i++)
  {
    CThread *pThread = new CThread(this, "BackgroundLoader");
    pThread->Create();
#ifndef _LINUX
    pThread->SetPriority(THREAD_PRIORITY_BELOW_NORMAL);
#endif
    m_workers.push_back(pThread);
  }
  CLog::Log(LOGDEBUG, "%s - new workers: %d, old workers: %d/%d", __FUNCTION__, nThreads, m_nActiveThreads-nThreads, (int)m_workers.size() - nThreads);
}

void CBackgroundInfoLoader::StopAsync(bool invokeOnLoaderFinish /* = false */)
{
  m_bStop = true;

  if (!invokeOnLoaderFinish)
    return;

  CSingleLock lock(m_lock);
  if (!m_bFinishCalled && m_nActiveThreads > 0)
  {
    // save current work in inherit loaders.
    OnLoaderFinish();
    m_bFinishCalled = true;
  }
  m_vecItems.clear();

  // delete old workers if already stopped, leave running workers to
  // finish their work, this will fasten the ui nav switch.
  int nWorkerDeleted = 0;
  for (int i=0; i<(int)m_workers.size();)
  {
    if (!m_workers[i]->IsRunning())
    {
      delete m_workers[i];
      m_workers.erase(m_workers.begin() + i);
      ++nWorkerDeleted;
    }
    else
    {
      // notify worker to stop without wait.
      m_workers[i]->StopThread(false);
      ++i;
    }
  }
  if (!m_workers.empty() || nWorkerDeleted > 0)
    CLog::Log(LOGDEBUG, "%s - inactive old workers: %d/%d, deleted workers: %d", __FUNCTION__, m_nActiveThreads, (int)m_workers.size(), nWorkerDeleted);
  m_pVecItems = NULL;
}


void CBackgroundInfoLoader::StopThread()
{
  StopAsync();

  CSingleLock lock(m_lock);
  std::vector<CThread *> tempHolder;
  for (int i=0; i<(int)m_workers.size(); i++)
  {
    if (m_workers[i]->IsRunning())
      tempHolder.push_back(m_workers[i]);
    else
      delete m_workers[i];
  }
  m_workers.clear();
  m_vecItems.clear();
  lock.Leave();

  // we have set stop flag, wait workers finish their work.
  while (m_nActiveThreads > 0)
    Sleep(100);
  for (int i=0; i<(int)tempHolder.size(); i++)
  {
    tempHolder[i]->StopThread();
    delete tempHolder[i];
  }

  lock.Enter();
  m_pVecItems = NULL;
  m_nActiveThreads = 0;
}

bool CBackgroundInfoLoader::IsLoading()
{
  return m_nActiveThreads > 0;
}

void CBackgroundInfoLoader::SetObserver(IBackgroundLoaderObserver* pObserver)
{
  m_pObserver = pObserver;
}

void CBackgroundInfoLoader::SetProgressCallback(IProgressCallback* pCallback)
{
  m_pProgressCallback = pCallback;
}

