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
	virtual void				OnRetrieveMusicInfo(VECFILEITEMS& items);

					void				OnScan();
					bool				DoScan(VECFILEITEMS& items);
					void				LoadPlayList(const CStdString& strPlayList);

	CStdString					m_strPrevDir;
	bool								m_bScan;
};
