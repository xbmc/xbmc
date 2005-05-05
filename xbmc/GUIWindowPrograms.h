#pragma once
#include "guiwindow.h"
#include "programdatabase.h"
#include "GUIViewControl.h"

class CGUIWindowPrograms :
      public CGUIWindow
{
public:
  CGUIWindowPrograms(void);
  virtual ~CGUIWindowPrograms(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnPopupMenu(int iItem);
  virtual void Render();
  virtual void OnAction(const CAction &action);
  virtual void OnWindowLoaded();
protected:
  void OnScan(CFileItemList& items, int& iTotalAppsFound) ;
  void Update(const CStdString& strDirectory);
  void UpdateDir(const CStdString& strDirectory);
  void LoadDirectory(const CStdString& strDirectory, int depth);
  void OnClick(int iItem);
  void OnSort();
  void UpdateButtons();
  void ClearFileItems();
  void DeleteThumbs(CFileItemList& items);
  void GoParentFolder();
  CGUIDialogProgress* m_dlgProgress;
  CFileItemList m_vecItems;
  CFileItem m_Directory;
  CStdString m_shareDirectory;
  int m_iLastControl;
  int m_iSelectedItem;
  int m_iDepth;
  CStdString m_strBookmarkName;
  CProgramDatabase m_database;
  CStdString m_strParentPath;
  int m_iViewAsIcons;
  CGUIViewControl m_viewControl;
};
