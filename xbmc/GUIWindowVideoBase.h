#pragma once
#include "guiwindow.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "FileItem.h"
#include "GUIDialogProgress.h"
#include "videodatabase.h"

#include "stdstring.h"
#include <vector>
using namespace std;
using namespace DIRECTORY;

class CGUIWindowVideoBase : 	public CGUIWindow
{
public:
	CGUIWindowVideoBase(void);
	virtual ~CGUIWindowVideoBase(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  virtual void    Render();

private:
  bool              IsCorrectDiskInDrive(const CStdString& strFileName, const CStdString& strDVDLabel);
protected:
	virtual void      SetIMDBThumbs(VECFILEITEMS& items) {};
  void							UpdateThumbPanel();
	// overrideable stuff for the different window classes
  virtual bool      ViewByLargeIcon()=0;
	virtual bool      ViewByIcon()=0;
	virtual void			SetViewMode(int iMode)=0;
	virtual int				SortMethod()=0;
	virtual bool			SortAscending()=0;
	virtual void			SortItems(VECFILEITEMS& items)=0;
	virtual void			FormatItemLabels()=0;
  virtual void			UpdateButtons();
	virtual void			OnPopupMenu(int iItem);

  void							ClearFileItems();
	void							OnSort();
	virtual void			Update(const CStdString &strDirectory) {};	// CONSOLIDATE??
	virtual void			OnClick(int iItem) {};		// CONSOLIDATE??
	virtual void			GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items) {}; //FIXME - this should be in all classes

	void							GoParentFolder();
  virtual void			OnInfo(int iItem);
	virtual void			OnScan() {};
	int								GetSelectedItem();
	void							DisplayEmptyDatabaseMessage(bool bDisplay);

  bool							HaveDiscOrConnection( CStdString& strPath, int iDriveType );
  void              ShowIMDB(const CStdString& strMovie, const CStdString& strFile, const CStdString& strFolder, bool bFolder);
  void              OnManualIMDB();
  bool              CheckMovie(const CStdString& strFileName);
  void							OnQueueItem(int iItem);
  void							AddItemToPlayList(const CFileItem* pItem);

	CVirtualDirectory		m_rootDir;
  VECFILEITEMS        m_vecItems;
	CStdString					m_strDirectory;
	CDirectoryHistory		m_history;
  int                 m_iItemSelected;
  CGUIDialogProgress*	m_dlgProgress;
  CVideoDatabase      m_database;
	int									m_iLastControl;
	bool								m_bDisplayEmptyDatabaseMessage;
	CStdString					m_strParentPath;	///< Parent path to handle going up a dir
};
