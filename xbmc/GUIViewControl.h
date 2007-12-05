#pragma once
#include "GUIViewState.h"

#include "GUIBaseContainer.h"

class CGUIViewControl
{
public:
  CGUIViewControl(void);
  virtual ~CGUIViewControl(void);

  void Reset();
  void SetParentWindow(int window);
  void AddView(const CGUIControl *control);
  void SetViewControlID(int control);

  void SetCurrentView(int viewMode);

  void SetItems(CFileItemList &items);

  void SetSelectedItem(int item);
  void SetSelectedItem(const CStdString &itemPath);

  int GetSelectedItem() const;
  void SetFocused();

  bool HasControl(int controlID) const;
  int GetNextViewMode() const;
  int GetViewModeNumber(int number) const;

  int GetCurrentControl() const;

  void Clear();

protected:
  int GetSelectedItem(const CGUIControl *control) const;
  void UpdateContents(const CGUIControl *control, int currentItem);
  void UpdateView();
  void UpdateViewAsControl(const CStdString &viewLabel);
  int GetView(VIEW_TYPE type, int id) const;

  vector<CGUIControl *> m_vecViews;
  typedef vector<CGUIControl *>::const_iterator ciViews;

  CFileItemList*        m_fileItems;
  int                   m_viewAsControl;
  int                   m_parentWindow;
  int                   m_currentView;
};
