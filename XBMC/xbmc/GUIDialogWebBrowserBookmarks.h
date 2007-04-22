#pragma once

#include "GUIDialog.h"

#ifdef WITH_LINKS_BROWSER
#include "LinksBoksManager.h"

class CGUIDialogWebBrowserBookmarks :
      public CGUIDialog
{
public:
  CGUIDialogWebBrowserBookmarks(void);
  virtual ~CGUIDialogWebBrowserBookmarks(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();

protected:
  void PopulateBookmarks();
  void FillListControl();
  void ContextMenu();

  void DebugPrintList();

  // bookmark management routines
  void NewBookmark(CFileItem *pItem);
  void NewFolder(CFileItem *pItem);
  void EditBookmark(CFileItem *pItem);
  void DeleteBookmark(CFileItem *pItem);
  void MoveBookmark(CFileItem *pItem);
  bool SaveBookmarks();

  VECFILEITEMS m_vecBookmarks;
  int m_iDepth;
  CFileItem *m_curRoot;
  VECFILEITEMS m_vecCompletePath;
  VECFILEITEMS m_vecCurrentLevelItems;
  CFileItem *m_curBookmark;
  CFileItem *m_movingBookmark;
};

#endif