#pragma once
#include "guiwindow.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/directoryhistory.h"
#include "MusicDatabase.h"
#include "FileItem.h"
#include "GUIDialogProgress.h"
#include "playlist.h"
#include "musicInfoTagLoaderFactory.h"

#include "stdstring.h"
#include <vector>
using namespace std;
using namespace DIRECTORY;
using namespace PLAYLIST;

#define VIEW_AS_LIST							0
#define VIEW_AS_ICONS							1
#define VIEW_AS_LARGEICONS				2


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
	virtual void							OnRetrieveMusicInfo(VECFILEITEMS& items);
					void							GoParentFolder();
					void							RetrieveMusicInfo();
					void							OnInfo(int iItem);
					void							ClearFileItems();
					void							Update(const CStdString &strDirectory);
					void							UpdateListControl();
					void							AddItemToPlayList(const CFileItem* pItem) ;
					int								GetSelectedItem();
					void							OnSearch();
					bool							DoSearch(const CStdString strDir,const CStdString& strSearch,VECFILEITEMS& items);
					bool							HaveDiscOrConnection( CStdString& strPath, int iDriveType );
					bool							GetKeyboard(CStdString& strInput);
	virtual	void							ShowThumbPanel();
					bool							ViewByIcon();
					bool							ViewByLargeIcon();

	CStdString								m_strDirectory;
	CVirtualDirectory					m_rootDir;
  VECFILEITEMS							m_vecItems;
	typedef vector <CFileItem*>::iterator ivecItems;
	CGUIDialogProgress*				m_dlgProgress;
	CDirectoryHistory					m_history;
	CMusicDatabase						m_database;
	int												m_iViewAsIcons;
	int												m_iViewAsIconsRoot;
	static int								m_nTempPlayListWindow;
	static CStdString					m_strTempPlayListDirectory;
	int												m_nSelectedItem;
	int												m_nFocusedControl;
};
