#pragma once
#include "GUIWindowMusicBase.h"

class CGUIWindowMusicNav : 	public CGUIWindowMusicBase
{
public:

	CGUIWindowMusicNav(void);
	virtual	~CGUIWindowMusicNav(void);

	virtual	bool				OnMessage(CGUIMessage& message);

protected:
	// override base class methods
	virtual void				GoParentFolder();
	virtual void				GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items);
	virtual void				UpdateButtons();
	virtual void				OnFileItemFormatLabel(CFileItem* pItem);
	virtual void				OnClick(int iItem);
	virtual void				DoSort(VECFILEITEMS& items);
	virtual void				DoSearch(const CStdString& strSearch,VECFILEITEMS& items);
	virtual void				OnSearchItemFound(const CFileItem* pItem);
	virtual void				GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
	virtual void				AddItemToPlayList(const CFileItem* pItem);

	// new methods
	void						PlayItem(int iItem);
	void						AddItemToTempPlayList(const CFileItem* pItem);

	// state of the window
	int							m_iState;
	int							m_iPath;
	CStdString					m_strGenre;
	CStdString					m_strArtist;
	CStdString					m_strAlbum;
};
