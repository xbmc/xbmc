#pragma once
#include "GUIDialog.h"
#include "PictureInfoTag.h"

class CGUIDialogPictureInfo :
      public CGUIDialog
{
public:
  CGUIDialogPictureInfo(void);
  virtual ~CGUIDialogPictureInfo(void);
  void SetPicture(const CStdString &picture);
  virtual void Render();

protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  void UpdatePictureInfo();

  CFileItemList m_pictureInfo;
  CStdString    m_currentPicture;
};
