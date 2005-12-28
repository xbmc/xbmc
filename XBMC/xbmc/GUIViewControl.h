#pragma once
#include "GUIControl.h"
#include "GUIViewState.h"

typedef map<VIEW_METHOD, CGUIControl *>::const_iterator map_iter;

class CGUIViewControl
{
public:
  CGUIViewControl(void);
  virtual ~CGUIViewControl(void);

  void Reset();
  void SetParentWindow(int window);
  void AddView(VIEW_METHOD type, const CGUIControl *control);
  void SetViewControlID(int control);

  void SetCurrentView(VIEW_METHOD viewMode);

  void SetItems(CFileItemList &items);

  void SetSelectedItem(int item);
  void SetSelectedItem(const CStdString &itemPath);

  int GetSelectedItem() const;
  void SetFocused();

  bool HasControl(int controlID);
  bool HasViewMode(VIEW_METHOD viewMode);
  int GetCurrentControl();

  void Clear();

protected:
  int GetSelectedItem(const CGUIControl *control) const;
  void UpdateContents(const CGUIControl *control);
  void UpdateView();
  void UpdateViewAsControl();

  VIEW_METHOD       m_currentView;
  map<VIEW_METHOD, CGUIControl *>     m_vecViews;
  CFileItemList*        m_fileItems;
  int                   m_viewAsControl;
  int                   m_parentWindow;
};
