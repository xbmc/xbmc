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
  virtual void    OnKey(const CKey& key);
  virtual void    Render();

protected:
	void							GoParentFolder();
  void							OnClick(int iItem);
  void							OnSort();
  void							UpdateButtons();
  void							Clear();
	void							Update(const CStdString &strDirectory);
	int								GetSelectedItem();

  bool							HaveDiscOrConnection( CStdString& strPath, int iDriveType );
	CVirtualDirectory		m_rootDir;
  vector <CFileItem*> m_vecItems;
	CStdString					m_strDirectory;
	CDirectoryHistory		m_history;
	bool								m_bDVDDiscChanged;
	bool								m_bDVDDiscEjected;
	int									scriptSize;
	VECSHARES shares;
};
