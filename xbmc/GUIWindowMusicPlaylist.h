#pragma once
#include "GUIWindowMusicBase.h"
#include "MusicInfoLoader.h"

class CGUIWindowMusicPlayList : 	public CGUIWindowMusicBase, public IMusicInfoLoaderObserver
{
public:
	CGUIWindowMusicPlayList(void);
	virtual	~CGUIWindowMusicPlayList(void);

  virtual	bool				OnMessage(CGUIMessage& message);
  virtual	void				OnAction(const CAction &action);

protected:
	virtual void				GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual	void				UpdateButtons();
  virtual void				OnClick(int iItem);
	virtual	void				OnQueueItem(int iItem);
	virtual	void				DoSort(CFileItemList& items) {};
	virtual void				OnFileItemFormatLabel(CFileItem* pItem) {};
	virtual void				OnRetrieveMusicInfo(CFileItemList& items) {};
	virtual void				OnItemLoaded(CFileItem* pItem);
	virtual void				ClearFileItems();
	virtual void				Update(const CStdString& strDirectory);

					void				SavePlayList();
					void				ClearPlayList();
					void				ShufflePlayList();
					void				RemovePlayListItem(int iItem);
          void				MoveCurrentPlayListItem(int iAction); // up or down

					MAPSONGS		m_songsMap;
					CStdString	m_strPrevPath;
					CMusicInfoLoader m_tagloader;
};
