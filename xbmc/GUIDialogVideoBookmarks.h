#pragma once
#include "GUIDialog.h"
#include "GUIViewControl.h"
#include "VideoDatabase.h"

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
  CGUIControl *GetFirstFocusableControl(int id);

  CFileItemList m_vecItems;
  CGUIViewControl m_viewControl;
  VECBOOKMARKS m_bookmarks;
};
