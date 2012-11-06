/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "BackgroundInfoLoader.h"
#include "FileItem.h"

#ifdef _XBOX
#define ITEMS_PER_THREAD 10
#define MAX_THREAD_COUNT 2
#else
#define ITEMS_PER_THREAD 5
#define MAX_THREAD_COUNT 5
#endif

// Static initializers.
CCriticalSection        CBackgroundRunner::g_lock;
set<CBackgroundRunner*> CBackgroundRunner::g_activeThreads;

CBackgroundInfoLoader::CBackgroundInfoLoader(int nThreads, int pauseBetweenLoadsInMS)
{
  m_bStop = true;
  m_pObserver = NULL;
  m_pProgressCallback = NULL;
  m_pVecItems = NULL;
  m_nRequestedThreads = nThreads;
  m_pauseBetweenLoadsInMS = pauseBetweenLoadsInMS;
  m_workerGroup = 0;
}

CBackgroundInfoLoader::~CBackgroundInfoLoader()
{
  if (m_workerGroup)
  {
    m_workerGroup->Stop();
    m_workerGroup->Detach();
    m_workerGroup = 0;
  }
}

void CBackgroundInfoLoader::SetNumOfWorkers(int nThreads)
{
  m_nRequestedThreads = nThreads;
}

void CBackgroundRunner::Process()
{
  try
  {
    while (m_bStop == false)
    {
      CFileItemPtr pItem = m_group.PopItem();
      if (pItem)
      {
        // Load the item.
        try { m_group.LoadItem(pItem); }
        catch (...) { CLog::Log(LOGERROR, "%s::LoadItem - Unhandled exception for item %s", __FUNCTION__, pItem->GetPath().c_str()); }
      }
      else
      {
        // We're done!
        break;
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }

  // We're done.
  m_group.WorkerDone(this);
}

void CBackgroundInfoLoader::OnLoaderFinished()
{
  CSingleLock lock(m_lock);
  m_workerGroup = 0;
  lock.Leave();

  OnLoaderFinish();
}

void CBackgroundInfoLoader::Load(CFileItemList& items)
{
  StopThread();

  if (items.Size() == 0)
    return;
  
  CSingleLock lock(m_lock);

  for (int nItem=0; nItem < items.Size(); nItem++)
    m_vecItems.push_back(items[nItem]);

  m_pVecItems = &items;

  // Compute how many threads to spawn.
  int nThreads = m_nRequestedThreads;
  if (nThreads == -1)
    nThreads = (m_vecItems.size() / (ITEMS_PER_THREAD+1)) + 1;

  if (nThreads > MAX_THREAD_COUNT)
    nThreads = MAX_THREAD_COUNT;

  if (items.IsPlexMediaServer())
    nThreads = 2;
  
  // Create a worker group.
  m_bStop = false;
  m_workerGroup = new CBackgroundRunnerGroup(this, items, nThreads, m_pauseBetweenLoadsInMS);
}

void CBackgroundInfoLoader::StopAsync()
{
  CSingleLock lock(m_lock);
  
  if (m_workerGroup)
  {
    // Tell it to stop and then forget about it.
    m_workerGroup->Stop();
    m_workerGroup->Detach();
    m_workerGroup = 0;
  }
 
  m_bStop = true;
  
}


void CBackgroundInfoLoader::StopThread()
{
  StopAsync();
}

bool CBackgroundInfoLoader::IsLoading()
{
  CSingleLock lock(m_lock);
  return m_workerGroup != 0 && m_workerGroup->GetNumActiveThreads() > 0;
}

void CBackgroundInfoLoader::SetObserver(IBackgroundLoaderObserver* pObserver)
{
  m_pObserver = pObserver;
}

void CBackgroundInfoLoader::SetProgressCallback(IProgressCallback* pCallback)
{
  m_pProgressCallback = pCallback;
}
