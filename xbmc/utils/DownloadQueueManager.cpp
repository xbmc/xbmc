
#include "../stdafx.h"
#include "DownloadQueueManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Global instance of the download manager
CDownloadQueueManager g_DownloadManager;

CDownloadQueueManager::CDownloadQueueManager()
{}

CDownloadQueueManager::~CDownloadQueueManager(void)
{
  DeleteCriticalSection(&m_critical);
}

VOID CDownloadQueueManager::Initialize()
{
  InitializeCriticalSection(&m_critical);
}


TICKET CDownloadQueueManager::RequestContent(CStdString& aUrl, IDownloadQueueObserver* aObserver)
{
  EnterCriticalSection(&m_critical);
  TICKET ticket = GetNextDownloadQueue()->RequestContent(aUrl, aObserver);
  LeaveCriticalSection(&m_critical);
  return ticket;
}

TICKET CDownloadQueueManager::RequestFile(CStdString& aUrl, CStdString& aFilePath, IDownloadQueueObserver* aObserver)
{
  EnterCriticalSection(&m_critical);
  TICKET ticket = GetNextDownloadQueue()->RequestFile(aUrl, aFilePath, aObserver);
  LeaveCriticalSection(&m_critical);
  return ticket;
}

TICKET CDownloadQueueManager::RequestFile(CStdString& aUrl, IDownloadQueueObserver* aObserver)
{
  EnterCriticalSection(&m_critical);
  TICKET ticket = GetNextDownloadQueue()->RequestFile(aUrl, aObserver);
  LeaveCriticalSection(&m_critical);
  return ticket;
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
  if (pQueueAvailable->Size() > 0 && m_queues.size() < MAX_DOWNLOAD_QUEUES)
  {
    // spawn a new queue
    pQueueAvailable = new CDownloadQueue();
    m_queues.push_back(pQueueAvailable);
  }

  assert(pQueueAvailable != NULL);

  return pQueueAvailable;
}

