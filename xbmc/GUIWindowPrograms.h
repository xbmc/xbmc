#pragma once
#include "guiwindow.h"
#include "FileItem.h"
#include "GUIDialogProgress.h"

class CGUIWindowPrograms :
  public CGUIWindow
{
public:
  CGUIWindowPrograms(void);
  virtual ~CGUIWindowPrograms(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    Render();
  virtual void    OnAction(const CAction &action);
protected:
  void            ShowThumbPanel();  
  bool            ViewByLargeIcon();
  bool            ViewByIcon();
	void						OnScan(VECFILEITEMS& items, int& iTotalAppsFound)  ;
  void            Update(const CStdString& strDirectory);
  void            LoadDirectory(const CStdString& strDirectory);
  void            OnClick(int iItem);
  void            OnSort();
  void            UpdateButtons();
  void            Clear();
	void						DeleteThumbs(VECFILEITEMS& items);
  int             GetSelectedItem();
	void						GoParentFolder();
	CGUIDialogProgress*	m_dlgProgress;  
  VECFILEITEMS				 m_vecItems;
  CStdString          m_strDirectory;
	int							m_iLastControl;
	int							m_iSelectedItem;
  
};
