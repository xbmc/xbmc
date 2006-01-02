#pragma once
#include "GUIMediaWindow.h"
#include "filesystem/VirtualDirectory.h"
#include "filesystem/DirectoryHistory.h"
#include "GUIWindowSlideShow.h"
#include "PictureThumbLoader.h"
#include "DllImageLib.h"

using namespace DIRECTORY;

class CGUIWindowPictures : public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPictures(void);
  virtual ~CGUIWindowPictures(void);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void OnWindowLoaded();
  CFileItem CurrentDirectory() const { return m_vecItems;};

protected:
  virtual void GoParentFolder();
  virtual void OnClick(int iItem);
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory);

  bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  void OnPopupMenu(int iItem);
  void OnRegenerateThumbs();
  bool UpdateDir(const CStdString &strDirectory);
  void OnShowPicture(const CStdString& strPicture);
  void OnShowPictureRecursive(const CStdString& strPicture);
  void OnSlideShow(const CStdString& strPicture);
  void OnSlideShow();
  void OnSlideShowRecursive(const CStdString& strPicture);
  void OnSlideShowRecursive();
  void GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
  void SetHistoryForPath(const CStdString& strDirectory);
  void AddDir(CGUIWindowSlideShow *pSlideShow, const CStdString& strPath);
  virtual void OnItemLoaded(CFileItem* pItem);
  void OnDeleteItem(int iItem);
  virtual void OnRenameItem(int iItem);
  void CGUIWindowPictures::LoadPlayList(const CStdString& strPlayList);
  void ShowShareErrorMessage(CFileItem* pItem);

  CGUIDialogProgress* m_dlgProgress;
  vector<CStdString> m_vecPathHistory; ///< History of traversed directories
  DllImageLib m_ImageLib;

  CPictureThumbLoader m_thumbLoader;
};
