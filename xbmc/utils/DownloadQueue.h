#pragma once

struct TICKET
{
	TICKET(WORD aQueueId, DWORD aItemId)
	{
		wQueueId = aQueueId;
		dwItemId = aItemId;
	};

	WORD wQueueId;
	DWORD dwItemId;
};

class IDownloadQueueObserver
{
	public:
		enum Result {Succeeded,Failed};
		virtual void OnContentComplete(TICKET aTicket, CStdString& aContentString, Result aResult){};
		virtual void OnFileComplete(TICKET aTicket, CStdString& aFilePath, INT aByteRxCount, Result aResult){};
};


class CDownloadQueue: CThread
{
public:
	CDownloadQueue();
	virtual ~CDownloadQueue(void);

	TICKET	RequestContent(CStdString& aUrl, IDownloadQueueObserver* aObserver);
	TICKET	RequestFile(CStdString& aUrl, IDownloadQueueObserver* aObserver);
	TICKET	RequestFile(CStdString& aUrl, CStdString& aFilePath, IDownloadQueueObserver* aObserver);
	VOID	Flush();
	INT		Size();

protected:
  
	void OnStartup();
	void Process();

	struct Command
	{
		TICKET					ticket;
		CStdString				location;
		CStdString				content;
		IDownloadQueueObserver*	observer;
	};

	typedef std::queue<Command> COMMANDQUEUE;
	COMMANDQUEUE m_queue;
	CRITICAL_SECTION m_critical;

	WORD m_wQueueId;
	DWORD m_dwNextItemId;

	static WORD m_wNextQueueId;
};