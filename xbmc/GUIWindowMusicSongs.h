#pragma once
#include "GUIWindowMusicBase.h"

class CGUIWindowMusicSongs : 	public CGUIWindowMusicBase
{
public:
	CGUIWindowMusicSongs(void);
	virtual	~CGUIWindowMusicSongs(void);

  virtual	bool				OnMessage(CGUIMessage& message);

protected:
	virtual void				GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items);
  virtual	void				UpdateButtons();
	virtual void				OnFileItemFormatLabel(CFileItem* pItem);
  virtual void				OnClick(int iItem);
	virtual void				DoSort(VECFILEITEMS& items);
	virtual	void				DoSearch(const CStdString& strSearch,VECFILEITEMS& items);
	virtual	void				OnSearchItemFound(const CFileItem* pItem);
					void				AutoSwitchControlThumbList();

	virtual void				GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
	virtual void				OnRetrieveMusicInfo(VECFILEITEMS& items);

					void				OnScan();
					bool				DoScan(VECFILEITEMS& items);
					void				LoadPlayList(const CStdString& strPlayList);

	virtual void				Update(const CStdString &strDirectory);

	CStdString					m_strPrevDir;
	bool								m_bScan;
};
