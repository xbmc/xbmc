#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoTitle : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoTitle(void);
  virtual ~CGUIWindowVideoTitle(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual bool Update(const CStdString &strDirectory);
  virtual void UpdateButtons();
  virtual void OnDeleteItem(int iItem);
  virtual void OnQueueItem(int iItem);
};
