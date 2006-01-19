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
  virtual bool OnClick(int iItem);
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory);

  void OnPopupMenu(int iItem);
  void OnRegenerateThumbs();
  void OnPlayMedia(int iItem);
  void OnShowPictureRecursive(const CStdString& strPicture);
  void OnSlideShow(const CStdString& strPicture);
  void OnSlideShow();
  void OnSlideShowRecursive(const CStdString& strPicture);
  void OnSlideShowRecursive();
  void AddDir(CGUIWindowSlideShow *pSlideShow, const CStdString& strPath);
  virtual void OnItemLoaded(CFileItem* pItem);
  void OnDeleteItem(int iItem);
  virtual void OnRenameItem(int iItem);
  virtual void LoadPlayList(const CStdString& strPlayList);

  CGUIDialogProgress* m_dlgProgress;
  DllImageLib m_ImageLib;

  CPictureThumbLoader m_thumbLoader;
};
