#pragma once
#include "GUIWindow.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "GUIWindowSlideShow.h"
#include "GUIViewControl.h"
#include "PictureThumbLoader.h"

using namespace DIRECTORY;

class CGUIWindowPictures : public CGUIWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPictures(void);
  virtual ~CGUIWindowPictures(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();

  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  CFileItem CurrentDirectory() const { return m_Directory;};

protected:
  void GoParentFolder();
  bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  void OnClick(int iItem);
  void OnPopupMenu(int iItem);
  void OnRegenerateThumbs();
  void OnSort();
  void UpdateButtons();
  void ClearFileItems();
  bool Update(const CStdString &strDirectory);
  bool UpdateDir(const CStdString &strDirectory);
  void OnShowPicture(const CStdString& strPicture);
  void OnShowPictureRecursive(const CStdString& strPicture);
  void OnSlideShow(const CStdString& strPicture);
  void OnSlideShow();
  bool HaveDiscOrConnection( CStdString& strPath, int iDriveType );
  void OnSlideShowRecursive(const CStdString& strPicture);
  void OnSlideShowRecursive();
  int SortMethod();
  bool SortAscending();
  void SortItems(CFileItemList& items);
  void GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
  void SetHistoryForPath(const CStdString& strDirectory);
  void AddDir(CGUIWindowSlideShow *pSlideShow, const CStdString& strPath);
  virtual void OnItemLoaded(CFileItem* pItem);
  void OnDeleteItem(int iItem);
  void ShowShareErrorMessage(CFileItem* pItem);

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
  CPictureThumbLoader m_thumbLoader;
};
