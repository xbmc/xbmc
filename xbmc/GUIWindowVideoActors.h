#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoActors : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoActors(void);
  virtual ~CGUIWindowVideoActors(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void FormatItemLabels();
  virtual bool Update(const CStdString &strDirectory);
  virtual void OnClick(int iItem);
  virtual void OnInfo(int iItem);
  virtual void OnDeleteItem(int iItem) {return;};
};
