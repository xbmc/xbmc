#pragma once
#include "GUIWindowMusicBase.h"
#include <map>
#include <set>

class CGUIWindowMusicPlayList : 	public CGUIWindowMusicBase
{
public:
	CGUIWindowMusicPlayList(void);
	virtual	~CGUIWindowMusicPlayList(void);

  virtual	bool				OnMessage(CGUIMessage& message);
  virtual	void				OnAction(const CAction &action);

protected:
	virtual void				GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items);
  virtual	void				UpdateButtons();
  virtual void				OnClick(int iItem);
	virtual	void				OnQueueItem(int iItem);
	virtual void				OnFileItemFormatLabel(CFileItem* pItem);
	virtual	void				DoSort(VECFILEITEMS& items);
	virtual void				OnRetrieveMusicInfo(VECFILEITEMS& items);

					void				SavePlayList();
					void				ClearPlayList();
					void				ShufflePlayList();
					void				RemovePlayListItem(int iItem);

					SETPATHES m_Pathes;
};
