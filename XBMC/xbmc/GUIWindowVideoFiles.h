#pragma once
#include "GUIWindowVideoBase.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "FileItem.h"
#include "GUIDialogProgress.h"
#include "videodatabase.h"

#include "stdstring.h"
#include <vector>
using namespace std;
using namespace DIRECTORY;

class CGUIWindowVideoFiles : 	public CGUIWindowVideoBase
{
public:
	CGUIWindowVideoFiles(void);
	virtual ~CGUIWindowVideoFiles(void);
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  virtual void    Render();

private:
  virtual void      SetIMDBThumbs(VECFILEITEMS& items);
protected:
  virtual bool      ViewByLargeIcon();
  virtual bool      ViewByIcon();
	virtual void			SetViewMode(int iMode);
	virtual int				SortMethod();
	virtual bool			SortAscending();
  virtual void			AddFileToDatabase(const CStdString& strFile);
	virtual void			FormatItemLabels();
	virtual void			SortItems();
  virtual void			UpdateButtons();
	virtual void			Update(const CStdString &strDirectory);
  virtual void			OnClick(int iItem);

  virtual void			OnPopupMenu(int iItem);
  virtual void			OnInfo(int iItem);

  bool              OnScan(VECFILEITEMS& items);
  void              OnRetrieveVideoInfo(VECFILEITEMS& items);
  void              LoadPlayList(const CStdString& strFileName);
  virtual void			GetDirectory(const CStdString &strDirectory, VECFILEITEMS &items);
	virtual	void				GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
	void								SetHistoryForPath(const CStdString& strDirectory);
};
