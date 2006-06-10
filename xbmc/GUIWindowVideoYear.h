#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoYear : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoYear(void);
  virtual ~CGUIWindowVideoYear(void);

protected:
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual void OnInfo(int iItem);
  virtual void OnDeleteItem(int iItem) {return;};
};
