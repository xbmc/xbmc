
#include "stdafx.h"
#include "DownloadQueueManager.h"
#include "Log.h"

// Global instance of the download manager
CDownloadQueueManager g_DownloadManager;

CDownloadQueueManager::CDownloadQueueManager()
{
}

CDownloadQueueManager::~CDownloadQueueManager(void)
{
}

VOID CDownloadQueueManager::Initialize()
{
	CLog::Log(LOGNOTICE, "DownloadQueueManager: Instantiating...");

	for(int nQueue = 0; nQueue < DOWNLOAD_QUEUES; nQueue++)
	{
		m_queues.push_back( new CDownloadQueue() );
	}

	CLog::Log(LOGNOTICE, "DownloadQueueManager: Queues created.");
}


TICKET CDownloadQueueManager::RequestContent(CStdString& aUrl, IDownloadQueueObserver* aObserver)
{
	return GetNextDownloadQueue()->RequestContent(aUrl,aObserver);
}

TICKET CDownloadQueueManager::RequestFile(CStdString& aUrl, CStdString& aFilePath, IDownloadQueueObserver* aObserver)
{
	return GetNextDownloadQueue()->RequestFile(aUrl,aFilePath,aObserver);
}

TICKET CDownloadQueueManager::RequestFile(CStdString& aUrl, IDownloadQueueObserver* aObserver)
{
	return GetNextDownloadQueue()->RequestFile(aUrl,aObserver);
}

CDownloadQueue* CDownloadQueueManager::GetNextDownloadQueue()
{
	CDownloadQueue* pQueueAvailable = NULL;

	// return the queue with the least number of items pending
	for(QUEUEPOOL::iterator it = m_queues.begin(); it != m_queues.end(); ++it)
	{
		if (!pQueueAvailable)
		{
			pQueueAvailable = *it;
		}
		else
		{
			if ( pQueueAvailable->Size() > (*it)->Size() )
			{
				pQueueAvailable = *it;
			}
		}
	}

	assert(pQueueAvailable!=NULL);

	return pQueueAvailable;
}

