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
  virtual void    OnKey(const CKey& key);
protected:
  
	void						OnScan(VECFILEITEMS& items, int& iTotalAppsFound)  ;
  void            Update(const CStdString& strDirectory);
  void            LoadDirectory(const CStdString& strDirectory);
  void            OnClick(int iItem);
  void            OnSort();
  void            UpdateButtons();
  void            Clear();
	CGUIDialogProgress*	m_dlgProgress;  
  VECFILEITEMS				 m_vecItems;
  CStdString          m_strDirectory;
  
};
