#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoYear : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoYear(void);
  virtual ~CGUIWindowVideoYear(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual bool Update(const CStdString &strDirectory);
  virtual void OnClick(int iItem);
  virtual void OnInfo(int iItem);
  virtual void OnDeleteItem(int iItem) {return;};
};
