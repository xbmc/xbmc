#pragma once
#include "guiwindow.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "FileItem.h"
#include "GUIDialogProgress.h"
#include "GUIWindowSlideShow.h"
#include "stdstring.h"
#include <vector>
using namespace std;
using namespace DIRECTORY;

class CGUIWindowPictures : 	public CGUIWindow
{
public:
	CGUIWindowPictures(void);
	virtual ~CGUIWindowPictures(void);
  virtual bool			OnMessage(CGUIMessage& message);
  virtual void			OnAction(const CAction &action);
  virtual void			Render();

protected:
	void							GoParentFolder();
	void							GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items);
  void							OnClick(int iItem);
	void							OnPopupMenu(int iItem);
  void							OnSort();
  void							UpdateButtons();
  void							Clear();
	void							Update(const CStdString &strDirectory);
	void							UpdateDir(const CStdString &strDirectory);
	void							OnShowPicture(const CStdString& strPicture);
	void							OnSlideShow(const CStdString& strPicture);
	void							OnSlideShow();
	bool							OnCreateThumbs();
	int								GetSelectedItem();
  bool							HaveDiscOrConnection( CStdString& strPath, int iDriveType );  
  void              OnSlideShowRecursive(const CStdString& strPicture);
  void              OnSlideShowRecursive();
  bool              ViewByIcon();
  bool              ViewByLargeIcon();
  void							SetViewMode(int iMode);
	int								SortMethod();
	bool							SortAscending();
	void							SortItems(VECFILEITEMS& items);
  void              UpdateThumbPanel();
	void              GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
	void							SetHistoryForPath(const CStdString& strDirectory);
	bool							DoCreateFolderThumbs(CStdString &strFolder, int *iTotalItems, int *iCurrentItem, bool bRecurse);
	void							CreateFolderThumbs(bool bRecurse = false);
  void              AddDir(CGUIWindowSlideShow *pSlideShow,const CStdString& strPath);
	CVirtualDirectory		m_rootDir;
  VECFILEITEMS				m_vecItems;
	CStdString					m_strDirectory;
	CGUIDialogProgress*	m_dlgProgress;
	CDirectoryHistory		m_history;
  int                 m_iItemSelected;
	int									m_iLastControl;
	CStdString					m_strParentPath;

	int									m_iViewAsIcons;
	int									m_iViewAsIconsRoot;
};
