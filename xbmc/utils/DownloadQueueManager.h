#pragma once
#include "StdString.h"
#include "DownloadQueue.h"
#include <vector>

#define MAX_DOWNLOAD_QUEUES		3

class CDownloadQueueManager
{
public:
	CDownloadQueueManager();
	virtual ~CDownloadQueueManager(void);

	VOID    Initialize();
	TICKET	RequestContent(CStdString& aUrl, IDownloadQueueObserver* aObserver);
	TICKET	RequestFile(CStdString& aUrl, IDownloadQueueObserver* aObserver);
	TICKET	RequestFile(CStdString& aUrl, CStdString& aFilePath, IDownloadQueueObserver* aObserver);

protected:

	CDownloadQueue* GetNextDownloadQueue();

	typedef std::vector<CDownloadQueue*> QUEUEPOOL;
	QUEUEPOOL m_queues;

	CRITICAL_SECTION m_critical;
};

// Single global instance of class is in cpp file
extern CDownloadQueueManager g_DownloadManager;