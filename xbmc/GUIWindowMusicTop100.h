#pragma once
#include "GUIWindowMusicBase.h"

class CGUIWindowMusicTop100 : 	public CGUIWindowMusicBase
{
public:
	CGUIWindowMusicTop100(void);
	virtual	~CGUIWindowMusicTop100(void);

  virtual	bool				OnMessage(CGUIMessage& message);
  virtual	void				OnAction(const CAction &action);

protected:
	virtual void				GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items);
  virtual	void				UpdateButtons();
	virtual void				OnFileItemFormatLabel(CFileItem* pItem);
  virtual void				OnClick(int iItem);
	virtual	void				DoSort(VECFILEITEMS& items);

};
