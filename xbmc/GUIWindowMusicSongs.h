#pragma once
#include "GUIWindowMusicBase.h"

class CGUIWindowMusicSongs : 	public CGUIWindowMusicBase
{
public:
	CGUIWindowMusicSongs(void);
	virtual	~CGUIWindowMusicSongs(void);

  virtual	bool				OnMessage(CGUIMessage& message);
	void						FilterItems(VECFILEITEMS &items);

protected:
	virtual void				GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items);
  virtual	void				UpdateButtons();
	virtual void				OnFileItemFormatLabel(CFileItem* pItem);
  virtual void				OnClick(int iItem);
 	virtual void				OnPopupMenu(int iItem);
	virtual void				DoSort(VECFILEITEMS& items);
	virtual	void				DoSearch(const CStdString& strSearch,VECFILEITEMS& items);
	virtual	void				OnSearchItemFound(const CFileItem* pItem);

					void				SetHistoryForPath(const CStdString& strDirectory);
	virtual void				GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
	virtual void				OnRetrieveMusicInfo(VECFILEITEMS& items);

	virtual void				OnScan();
					void				LoadPlayList(const CStdString& strPlayList);

	virtual void				Update(const CStdString &strDirectory);

					void				LoadDirectoryCache(const CStdString& strDirectory, MAPFILEITEMS& items);
					void				SaveDirectoryCache(const CStdString& strDirectory, VECFILEITEMS& items);
					void				DeleteDirectoryCache();

	CStdString					m_strPrevDir;
};
