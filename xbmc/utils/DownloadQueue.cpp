
#include "../stdafx.h"
#include "DownloadQueue.h"
#include "Http.h"
#include "../Util.h"

WORD CDownloadQueue::m_wNextQueueId = 0;

CDownloadQueue::CDownloadQueue(void) : CThread()
{
	InitializeCriticalSection(&m_critical);
	m_bStop = false;
	m_wQueueId = m_wNextQueueId++;
	m_dwNextItemId = 0;
	CThread::Create(false);
}

void CDownloadQueue::OnStartup()
{
	SetPriority( THREAD_PRIORITY_LOWEST );
}

CDownloadQueue::~CDownloadQueue(void)
{
	DeleteCriticalSection(&m_critical);
}


TICKET CDownloadQueue::RequestContent(CStdString& aUrl, IDownloadQueueObserver* aObserver)
{
	EnterCriticalSection(&m_critical);

	TICKET ticket(m_wQueueId,m_dwNextItemId++);

	Command request = {ticket,aUrl,"",aObserver};
	m_queue.push(request);

	LeaveCriticalSection(&m_critical);
	return request.ticket;
}

TICKET CDownloadQueue::RequestFile(CStdString& aUrl, CStdString& aFilePath, IDownloadQueueObserver* aObserver)
{
	EnterCriticalSection(&m_critical);

	TICKET ticket(m_wQueueId,m_dwNextItemId++);

	Command request = {ticket,aUrl,aFilePath,aObserver};
	m_queue.push(request);

	LeaveCriticalSection(&m_critical);
	return request.ticket;
}

TICKET CDownloadQueue::RequestFile(CStdString& aUrl, IDownloadQueueObserver* aObserver)
{
	EnterCriticalSection(&m_critical);

	// create a temporary destination
	CStdString strExtension;
	CUtil::GetExtension(aUrl,strExtension);
	
	TICKET ticket(m_wQueueId,m_dwNextItemId++);

	CStdString strFilePath;
	strFilePath.Format("Z:\\q%d-item%u%s", ticket.wQueueId, ticket.dwItemId, strExtension.c_str());

	Command request = {ticket,aUrl,strFilePath,aObserver};
	m_queue.push(request);

	LeaveCriticalSection(&m_critical);
	return request.ticket;
}

VOID CDownloadQueue::Flush()
{
	EnterCriticalSection(&m_critical);

	m_queue.empty();

	LeaveCriticalSection(&m_critical);
}

void CDownloadQueue::Process() 
{
	CLog::Log(LOGNOTICE, "DownloadQueue ready.");

	CHTTP http;
	bool bSuccess;

	while ( !m_bStop ) 
	{	
		while( CDownloadQueue::Size()>0 )
		{
			EnterCriticalSection(&m_critical);

			Command request = m_queue.front();
			m_queue.pop();

			LeaveCriticalSection(&m_critical);

			bool bFileRequest = request.content.length()>0;
			DWORD dwSize = 0;

			if (bFileRequest)
			{
				::DeleteFile(request.content.c_str());
				bSuccess = http.Download(request.location, request.content, &dwSize);
			}
			else
			{
				bSuccess = http.Get(request.location, request.content);
			}

			assert(request.observer!=NULL);

			g_graphicsContext.Lock();
			
			try
			{
				if (bFileRequest)
				{
					request.observer->OnFileComplete(request.ticket, request.content, dwSize,
						bSuccess ? IDownloadQueueObserver::Succeeded : IDownloadQueueObserver::Failed );
				}
				else
				{
					request.observer->OnContentComplete(request.ticket, request.content,
						bSuccess ? IDownloadQueueObserver::Succeeded : IDownloadQueueObserver::Failed );
				}
			}
			catch(...)
			{
				CLog::Log(LOGERROR, "exception while updating download observer.");
				
				if (bFileRequest)
				{
					::DeleteFile(request.content.c_str());
				}
			}

			g_graphicsContext.Unlock();
		}

		Sleep(500);
	}

	CLog::Log(LOGNOTICE, "DownloadQueue terminated.");	
}

INT CDownloadQueue::Size()
{
	EnterCriticalSection(&m_critical);

	int sizeOfQueue = m_queue.size();

	LeaveCriticalSection(&m_critical);

	return sizeOfQueue;
}

