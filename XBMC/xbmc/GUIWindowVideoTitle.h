#pragma once
#include "GUIWindowVideoBase.h"

class CGUIWindowVideoTitle : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoTitle(void);
  virtual ~CGUIWindowVideoTitle(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

protected:
  virtual void SaveViewMode();
  virtual void LoadViewMode();
  virtual int SortMethod();
  virtual bool SortAscending();

  virtual void FormatItemLabels();
  virtual void SortItems(CFileItemList& items);
  virtual void Update(const CStdString &strDirectory);
  virtual void OnClick(int iItem);
};
