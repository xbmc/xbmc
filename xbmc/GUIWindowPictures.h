#pragma once
#include "GUIMediaWindow.h"
#include "FileSystem/VirtualDirectory.h"
#include "FileSystem/DirectoryHistory.h"
#include "GUIWindowSlideShow.h"
#include "PictureThumbLoader.h"
#include "DllImageLib.h"

class CGUIWindowPictures : public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPictures(void);
  virtual ~CGUIWindowPictures(void);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void OnInfo(int item);
  virtual bool OnClick(int iItem);
  virtual void UpdateButtons();
  virtual void OnPrepareFileItems(CFileItemList& items);
  virtual bool Update(const CStdString &strDirectory);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);

  void OnRegenerateThumbs();
  virtual bool OnPlayMedia(int iItem);
  void OnShowPictureRecursive(const CStdString& strPicture, CFileItemList* pVecItems=NULL);
  void OnSlideShow(const CStdString& strPicture);
  void OnSlideShow();
  void OnSlideShowRecursive(const CStdString& strPicture);
  void OnSlideShowRecursive();
  void AddDir(CGUIWindowSlideShow *pSlideShow, const CStdString& strPath);
  virtual void OnItemLoaded(CFileItem* pItem);
  virtual void LoadPlayList(const CStdString& strPlayList);

  CGUIDialogProgress* m_dlgProgress;
  DllImageLib m_ImageLib;

  CPictureThumbLoader m_thumbLoader;
};
