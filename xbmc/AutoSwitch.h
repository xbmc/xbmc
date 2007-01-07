#pragma once

class CAutoSwitch
{
public:

  CAutoSwitch(void);
  virtual ~CAutoSwitch(void);

  static int GetView(const CFileItemList& vecItems);

  static bool ByFolders(const CFileItemList& vecItems);
  static bool ByFiles(bool bHideParentDirItems, const CFileItemList& vecItems);
  static bool ByThumbPercent(bool bHideParentDirItems, int iPercent, const CFileItemList& vecItems);
  static bool ByFileCount(const CFileItemList& vecItems);
  static bool ByFolderThumbPercentage(bool hideParentDirItems, int percent, const CFileItemList &vecItems);

protected:

};
