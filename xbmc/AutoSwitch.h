#pragma once
#include "FileItem.h"

class CAutoSwitch
{
public:

	CAutoSwitch(void);
	virtual ~CAutoSwitch(void);

	static int GetView(CFileItemList& vecItems);

	static int ByFolders(bool bBigThumbs, CFileItemList& vecItems);
	static int ByFiles(bool bBigThumbs, bool bHideParentDirItems, CFileItemList& vecItems);
	static int ByThumbPercent(bool bBigThumbs, bool bHideParentDirItems, int iPercent, CFileItemList& vecItems);

protected:

};
