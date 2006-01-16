#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoGenre : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoGenre(void);
  virtual ~CGUIWindowVideoGenre(void);
  virtual bool OnMessage(CGUIMessage& message);  

protected:
  virtual bool Update(const CStdString &strDirectory);
  virtual void OnInfo(int iItem);
  virtual void OnDeleteItem(int iItem) {return;};
};
