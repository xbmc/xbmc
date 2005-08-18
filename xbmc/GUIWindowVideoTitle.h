#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoTitle : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoTitle(void);
  virtual ~CGUIWindowVideoTitle(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void SaveViewMode();
  virtual void LoadViewMode();
  virtual int SortMethod();
  virtual bool SortAscending();

  virtual void FormatItemLabels();
  virtual void SortItems(CFileItemList& items);
  virtual bool Update(const CStdString &strDirectory);
  virtual void OnClick(int iItem);
  virtual void OnDeleteItem(int iItem);
  virtual void OnQueueItem(int iItem);
};
