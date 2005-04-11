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
  virtual void SaveViewMode();
  virtual void LoadViewMode();
  virtual int SortMethod();
  virtual bool SortAscending();

  virtual void FormatItemLabels();
  virtual void SortItems(CFileItemList& items);
  virtual void Update(const CStdString &strDirectory);
  virtual void OnClick(int iItem);
  virtual void OnInfo(int iItem);
};
