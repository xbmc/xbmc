#pragma once
#include "guiwindow.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/directoryhistory.h"
#include "MusicDatabase.h"
#include "FileItem.h"
#include "GUIDialogProgress.h"
#include "playlist.h"

#include "stdstring.h"
#include <vector>
using namespace std;
using namespace DIRECTORY;
using namespace PLAYLIST;

class CGUIWindowMusicBase : 	public CGUIWindow
{
public:
	CGUIWindowMusicBase(void);
	virtual ~CGUIWindowMusicBase(void);
  virtual bool							OnMessage(CGUIMessage& message);
protected:
	virtual void							GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items)=0;
  virtual void							OnClick(int iItem)=0;
	virtual void							OnFileItemFormatLabel(CFileItem* pItem)=0;
	virtual	void							DoSort(VECFILEITEMS& items)=0;
  virtual	void							UpdateButtons()=0;
	virtual	void							OnQueueItem(int iItem);
					void							GoParentFolder();
					void							RetrieveMusicInfo();
					void							OnInfo(int iItem);
					void							ClearFileItems();
					void							Update(const CStdString &strDirectory);
					void							OnRetrieveMusicInfo(VECFILEITEMS& items, bool bScan=false);
					void							UpdateListControl();
					void							AddItemToPlayList(const CFileItem* pItem) ;
					int								GetSelectedItem();
					void							OnSearch();
					bool							DoSearch(const CStdString strDir,const CStdString& strSearch,VECFILEITEMS& items);
					bool							HaveDiscOrConnection( CStdString& strPath, int iDriveType );

	CStdString								m_strDirectory;
	CVirtualDirectory					m_rootDir;
  VECFILEITEMS							m_vecItems;
	typedef vector <CFileItem*>::iterator ivecItems;
	CGUIDialogProgress*				m_dlgProgress;
	CDirectoryHistory					m_history;
	CMusicDatabase						m_database;
	bool											m_bViewAsIcons;
	bool											m_bViewAsIconsRoot;
	static int								m_nTempPlayListWindow;
	static CStdString					m_strTempPlayListDirectory;
	int												m_nSelectedItem;
};
