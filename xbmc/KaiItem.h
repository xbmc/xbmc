#pragma once
#include "GUIListExItem.h"
#include "Utils/DownloadQueueManager.h"

class CKaiItem : public CGUIListExItem, public IDownloadQueueObserver
{
public:
	CKaiItem(CStdString& strLabel);
	virtual ~CKaiItem(void);

	void SetAvatar(CStdString& aUrl);

	bool IsAvatarCached();
	void UseCachedAvatar();

	virtual void OnFileComplete(TICKET aTicket, CStdString& aFilePath, INT aByteRxCount, Result aResult);

protected:
	
	void GetAvatarFilePath(CStdString& aFilePath);

public:

	CGUIImage*	 m_pAvatar;
};