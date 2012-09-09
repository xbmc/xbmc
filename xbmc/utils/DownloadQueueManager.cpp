/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "DownloadQueueManager.h"
#include "threads/SingleLock.h"
#include <assert.h>

CDownloadQueueManager::CDownloadQueueManager()
{
}

CDownloadQueueManager::~CDownloadQueueManager(void)
{
}

VOID CDownloadQueueManager::Initialize()
{
}


TICKET CDownloadQueueManager::RequestContent(const CStdString& aUrl, IDownloadQueueObserver* aObserver)
{
  CSingleLock lock(m_critical);
  return GetNextDownloadQueue()->RequestContent(aUrl, aObserver);
}

TICKET CDownloadQueueManager::RequestFile(const CStdString& aUrl, const CStdString& aFilePath, IDownloadQueueObserver* aObserver)
{
  CSingleLock lock(m_critical);
  return GetNextDownloadQueue()->RequestFile(aUrl, aFilePath, aObserver);
}

TICKET CDownloadQueueManager::RequestFile(const CStdString& aUrl, IDownloadQueueObserver* aObserver)
{
  CSingleLock lock(m_critical);
  return GetNextDownloadQueue()->RequestFile(aUrl, aObserver);
}

void CDownloadQueueManager::CancelRequests(IDownloadQueueObserver *aObserver)
{
  CSingleLock lock(m_critical);
  // run through all our queues and remove all requests from this observer
  for (QUEUEPOOL::iterator it = m_queues.begin(); it != m_queues.end(); ++it)
  {
    CDownloadQueue* downloadQueue = *it;
    downloadQueue->CancelRequests(aObserver);
  }
}

CDownloadQueue* CDownloadQueueManager::GetNextDownloadQueue()
{
  CDownloadQueue* pQueueAvailable = NULL;

  // if we haven't added any queues to the pool, add one.
  if (m_queues.size() < 1)
  {
    m_queues.push_back( new CDownloadQueue() );
  }

  // return the queue with the least number of items pending
  for (QUEUEPOOL::iterator it = m_queues.begin(); it != m_queues.end(); ++it)
  {
    // always choose the first queue if we haven't selected one yet
    if (!pQueueAvailable)
    {
      pQueueAvailable = *it;
    }
    else
    {
      // pick this queue if it has less items pending than our previous selection
      if ( pQueueAvailable->Size() > (*it)->Size() )
      {
        pQueueAvailable = *it;
      }
    }
  }

  // if we picked a queue with pending items and we haven't reached out max pool limit
  if (pQueueAvailable && pQueueAvailable->Size() > 0 && m_queues.size() < MAX_DOWNLOAD_QUEUES)
  {
    // spawn a new queue
    pQueueAvailable = new CDownloadQueue();
    m_queues.push_back(pQueueAvailable);
  }

  assert(pQueueAvailable != NULL);

  return pQueueAvailable;
}

