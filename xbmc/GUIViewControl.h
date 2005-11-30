#pragma once
#include "GUIControl.h"

typedef map<int, CGUIControl *>::const_iterator map_iter;

#define VIEW_AS_LIST        0
#define VIEW_AS_ICONS       1
#define VIEW_AS_LARGE_ICONS 2
#define VIEW_AS_LARGE_LIST  3

class CGUIViewControl
{
public:
  CGUIViewControl(void);
  virtual ~CGUIViewControl(void);

  void Reset();
  void SetParentWindow(int window);
  void AddView(int type, const CGUIControl *control);
  void SetViewControlID(int control);

  void SetCurrentView(int viewMode);
  int GetCurrentView();
  void SetItems(CFileItemList &items);

  void SetSelectedItem(int item);
  void SetSelectedItem(const CStdString &itemPath);

  int GetSelectedItem();
  void SetFocused();

  bool HasControl(int viewMode);
  int GetCurrentControl();

  void Clear();

protected:
  int GetSelectedItem(const CGUIControl *control);
  void UpdateContents(const CGUIControl *control);
  void UpdateView();
  void UpdateViewAsControl();

  unsigned int          m_currentView;
  map<int, CGUIControl *>     m_vecViews;
  CFileItemList*        m_fileItems;
  int                   m_viewAsControl;
  int                   m_parentWindow;
};
