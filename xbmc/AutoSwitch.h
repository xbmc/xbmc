#pragma once

class CAutoSwitch
{
public:

  CAutoSwitch(void);
  virtual ~CAutoSwitch(void);

  static int GetView(CFileItemList& vecItems);

  static bool ByFolders(CFileItemList& vecItems);
  static bool ByFiles(bool bHideParentDirItems, CFileItemList& vecItems);
  static bool ByThumbPercent(bool bHideParentDirItems, int iPercent, CFileItemList& vecItems);
  static bool ByFileCount(CFileItemList& vecItems);

protected:

};
