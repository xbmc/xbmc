#pragma once
#include "GUIWindowVideoBase.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "videodatabase.h"

using namespace DIRECTORY;

class CGUIWindowVideoYear : public CGUIWindowVideoBase
{
public:
  CGUIWindowVideoYear(void);
  virtual ~CGUIWindowVideoYear(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void FormatItemLabels();
  virtual bool Update(const CStdString &strDirectory);
  virtual void OnClick(int iItem);
  virtual void OnInfo(int iItem);
  virtual void OnDeleteItem(int iItem) {return;};
};
