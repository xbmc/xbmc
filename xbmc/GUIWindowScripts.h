#pragma once
#include "GUIWindow.h"
#include "FileSystem/VirtualDirectory.h"
#include "FileSystem/DirectoryHistory.h"
#include "FileItem.h"

using namespace DIRECTORY;

class CGUIWindowScripts : 	public CGUIWindow
{
public:
	CGUIWindowScripts(void);
	virtual ~CGUIWindowScripts(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  virtual void    Render();

protected:
	void							GoParentFolder();
  void							OnClick(int iItem);
  void							OnSort();
	void							OnInfo();
  void							UpdateButtons();
  void							ClearFileItems();
	void							Update(const CStdString &strDirectory);
	int								GetSelectedItem();
  bool							HaveDiscOrConnection( CStdString& strPath, int iDriveType );

	CVirtualDirectory		m_rootDir;
  CFileItemList				m_vecItems;
	CFileItem					m_Directory;
	CStdString					m_strParentPath;
	CDirectoryHistory		m_history;
	bool								m_bDVDDiscChanged;
	bool								m_bDVDDiscEjected;
	bool								m_bViewOutput;
	int									scriptSize;
	int									m_iLastControl;
	int									m_iSelectedItem;
	VECSHARES shares;
};
