#pragma once
#include "GUIWindowVideo.h"
#include <map>
#include <set>

class CGUIWindowVideoPlaylist : public CGUIWindow
{
public:
	CGUIWindowVideoPlaylist(void);
	virtual	~CGUIWindowVideoPlaylist(void);

  virtual	bool				OnMessage(CGUIMessage& message);
  virtual	void				OnAction(const CAction &action);

protected:
  void        GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
  void        GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items);
  void        Update(const CStdString &strDirectory);
	void				ClearPlayList();
  void        ClearFileItems();
  void        UpdateListControl();
  void        RemovePlayListItem(int iItem);
  void        OnFileItemFormatLabel(CFileItem* pItem);
  void        DoSort(VECFILEITEMS& items);
  void        UpdateButtons();
  int         GetSelectedItem();
  void        ShowThumbPanel();
  bool        ViewByIcon();
  bool        ViewByLargeIcon();
  void        OnClick(int iItem);
  void        OnQueueItem(int iItem);
  int         m_iItemSelected;
  int         m_iLastControl;
  VECFILEITEMS							m_vecItems;
	CDirectoryHistory		m_history;
  CStdString m_strDirectory;
	typedef vector <CFileItem*>::iterator ivecItems;
};
