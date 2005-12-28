#pragma once
#include "GUIViewState.h" // for VIEW_METHOD enum definition

class CAutoSwitch
{
public:

  CAutoSwitch(void);
  virtual ~CAutoSwitch(void);

  static VIEW_METHOD GetView(const CFileItemList& vecItems);

  static bool ByFolders(const CFileItemList& vecItems);
  static bool ByFiles(bool bHideParentDirItems, const CFileItemList& vecItems);
  static bool ByThumbPercent(bool bHideParentDirItems, int iPercent, const CFileItemList& vecItems);
  static bool ByFileCount(const CFileItemList& vecItems);

protected:

};
