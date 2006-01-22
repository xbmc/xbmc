#pragma once
#include "GUIMediaWindow.h"
#include "programdatabase.h"
#include "GUIDialogProgress.h"

class CGUIWindowPrograms :
      public CGUIMediaWindow
{
public:
  CGUIWindowPrograms(void);
  virtual ~CGUIWindowPrograms(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnPopupMenu(int iItem);
  virtual void OnWindowLoaded();
  CFileItem CurrentDirectory() const { return m_vecItems;};

protected:
  virtual void GoParentFolder();
  virtual bool OnClick(int iItem);
  virtual bool Update(const CStdString& strDirectory);

  void OnScan(CFileItemList& items, int& iTotalAppsFound) ;
  void LoadDirectory(const CStdString& strDirectory, int depth);
  void DeleteThumbs(CFileItemList& items);
  int GetRegion(int iItem, bool bReload=false);
  void PopulateTrainersList();
  CGUIDialogProgress* m_dlgProgress;

  CStdString m_shareDirectory;

  int m_iDepth;
  CStdString m_strBookmarkName;
  vector<CStdString> m_vecPaths, m_vecPaths1;
  CProgramDatabase m_database;
  CStdString m_strParentPath;
  bool m_isRoot;
  int m_iRegionSet; // for cd stuff
};
