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
protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  CFileItemList m_pictureInfo;
};
