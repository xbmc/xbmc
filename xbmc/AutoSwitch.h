#pragma once
#include "FileItem.h"

class CAutoSwitch
{
public:

	CAutoSwitch(void);
	virtual ~CAutoSwitch(void);

	static int GetView(VECFILEITEMS& vecItems);

	static int ByFolders(bool bBigThumbs, VECFILEITEMS& vecItems);
	static int ByFiles(bool bBigThumbs, bool bHideParentDirItems, VECFILEITEMS& vecItems);
	static int ByThumbPercent(bool bBigThumbs, bool bHideParentDirItems, int iPercent, VECFILEITEMS& vecItems);

protected:

};
