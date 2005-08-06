#pragma once
#include "GUIDialog.h"
#include "GUIViewControl.h"

class CGUIDialogVideoBookmarks : public CGUIDialog
{
public:
  CGUIDialogVideoBookmarks(void);
  virtual ~CGUIDialogVideoBookmarks(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();

protected:
  void GotoBookmark(int iItem);
  void ClearBookmarks();
  void AddBookmark();
  void Clear();
  void Update();

  CFileItemList m_vecItems;
  CGUIViewControl m_viewControl;
};
