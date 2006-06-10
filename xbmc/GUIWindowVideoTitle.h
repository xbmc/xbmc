#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoTitle : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoTitle(void);
  virtual ~CGUIWindowVideoTitle(void);

protected:
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void OnDeleteItem(int iItem);
  virtual void OnQueueItem(int iItem);
};
