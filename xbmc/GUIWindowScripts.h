#pragma once
#include "guiwindow.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "FileItem.h"

#include "stdstring.h"
#include <vector>
using namespace std;
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
  VECFILEITEMS				m_vecItems;
	CStdString					m_strDirectory;
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
