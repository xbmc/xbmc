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
  virtual void OnClick(int iItem);
  virtual bool Update(const CStdString& strDirectory);

  void OnScan(CFileItemList& items, int& iTotalAppsFound) ;
  void UpdateDir(const CStdString& strDirectory);
  void LoadDirectory(const CStdString& strDirectory, int depth);
  void DeleteThumbs(CFileItemList& items);
  int GetRegion(int iItem, bool bReload=false);

  CGUIDialogProgress* m_dlgProgress;

  CStdString m_shareDirectory;

  int m_iDepth;
  CStdString m_strBookmarkName;
//  set <CStdString> m_setPaths;
//  set <CStdString> m_setPaths1;
  vector<CStdString> m_vecPaths, m_vecPaths1;
  CProgramDatabase m_database;
  bool m_isRoot;
  int m_iRegionSet; // for cd stuff
};
