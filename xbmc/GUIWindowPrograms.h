#pragma once
#include "guiwindow.h"
#include "FileItem.h"
#include "GUIDialogProgress.h"
#include "programdatabase.h"

class CGUIWindowPrograms :
	public CGUIWindow
{
public:
	CGUIWindowPrograms(void);
	virtual ~CGUIWindowPrograms(void);
	virtual bool		OnMessage(CGUIMessage& message);
	virtual void		Render();
	virtual void		OnAction(const CAction &action);
protected:
	void				ShowThumbPanel();  
	bool				ViewByLargeIcon();
	bool				ViewByIcon();
	void				OnScan(VECFILEITEMS& items, int& iTotalAppsFound)  ;
	void				Update(const CStdString& strDirectory);
	void				UpdateDir(const CStdString& strDirectory);
	void				LoadDirectory(const CStdString& strDirectory, int depth);
	void				OnClick(int iItem);
	void				OnSort();
	void				UpdateButtons();
	void				Clear();
	void				DeleteThumbs(VECFILEITEMS& items);
	int					GetSelectedItem();
	void				GoParentFolder();
	CGUIDialogProgress*	m_dlgProgress;  
	VECFILEITEMS		m_vecItems;
	CStdString			m_strDirectory;
	CStdString			m_shareDirectory;
	int					m_iLastControl;
	int					m_iSelectedItem;
	int					m_iDepth;	
	CStdString			m_strBookmarkName;
	CProgramDatabase	m_database;
	CStdString			m_strParentPath;
	int					m_iViewAsIcons;
};
