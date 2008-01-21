#pragma once
#include "GUIListExItem.h"
#include "utils/DownloadQueueManager.h"

class CKaiItem : public CGUIListExItem, public IDownloadQueueObserver
{
public:
  CKaiItem(CStdString& strLabel);
  virtual ~CKaiItem(void);
  virtual void AllocResources();
  virtual void FreeResources();

  void SetAvatar(CStdString& aUrl);

  bool IsAvatarCached();
  void UseCachedAvatar();

  virtual void OnFileComplete(TICKET aTicket, CStdString& aFilePath, INT aByteRxCount, Result aResult);

protected:

  void GetAvatarFilePath(CStdString& aFilePath);

public:

  CGUIImage* m_pAvatar;
};
