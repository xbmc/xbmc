#pragma once
#include "GUIWindow.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "GUIWindowSlideShow.h"
#include "GUIViewControl.h"

using namespace DIRECTORY;

class CGUIWindowPictures : public CGUIWindow
{
public:
  CGUIWindowPictures(void);
  virtual ~CGUIWindowPictures(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();

  virtual void OnWindowLoaded();

protected:
  void GoParentFolder();
  void GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  void OnClick(int iItem);
  void OnPopupMenu(int iItem);
  void OnSort();
  void UpdateButtons();
  void ClearFileItems();
  void Update(const CStdString &strDirectory);
  void UpdateDir(const CStdString &strDirectory);
  void OnShowPicture(const CStdString& strPicture);
  void OnSlideShow(const CStdString& strPicture);
  void OnSlideShow();
  bool OnCreateThumbs();
  bool HaveDiscOrConnection( CStdString& strPath, int iDriveType );
  void OnSlideShowRecursive(const CStdString& strPicture);
  void OnSlideShowRecursive();
  int SortMethod();
  bool SortAscending();
  void SortItems(CFileItemList& items);
  void GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
  void SetHistoryForPath(const CStdString& strDirectory);
  bool DoCreateFolderThumbs(CStdString &strFolder, int *iTotalItems, int *iCurrentItem, bool bRecurse);
  void CreateFolderThumbs(bool bRecurse = false);
  void AddDir(CGUIWindowSlideShow *pSlideShow, const CStdString& strPath);
  CVirtualDirectory m_rootDir;
  CFileItemList m_vecItems;
  CFileItem m_Directory;
  CGUIDialogProgress* m_dlgProgress;
  CDirectoryHistory m_history;
  int m_iItemSelected;
  int m_iLastControl;
  CStdString m_strParentPath;

  int m_iViewAsIcons;
  int m_iViewAsIconsRoot;

  CGUIViewControl m_viewControl;
};
