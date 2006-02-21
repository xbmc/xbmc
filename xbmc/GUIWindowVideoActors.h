#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoActors : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoActors(void);
  virtual ~CGUIWindowVideoActors(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void OnPrepareFileItems(CFileItemList &items);
  virtual void UpdateButtons();
  virtual void OnInfo(int iItem);
  virtual void OnDeleteItem(int iItem) {return;};
};
