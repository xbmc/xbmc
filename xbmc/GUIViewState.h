#pragma once

typedef enum {
  VIEW_METHOD_NONE=-1,
  VIEW_METHOD_LIST,
  VIEW_METHOD_ICONS,
  VIEW_METHOD_LARGE_ICONS,
  VIEW_METHOD_LARGE_LIST,
  VIEW_METHOD_MAX
} VIEW_METHOD;

class CGUIViewState
{
public:
  virtual ~CGUIViewState();
  static CGUIViewState* GetViewState(int windowId, const CFileItemList& items);

  VIEW_METHOD SetNextViewAsControl();
  VIEW_METHOD GetViewAsControl() const;
  int GetViewAsControlButtonLabel() const;

  SORT_METHOD SetNextSortMethod();
  SORT_METHOD GetSortMethod() const;
  int GetSortMethodLabel() const;

  SORT_ORDER SetNextSortOrder();
  SORT_ORDER GetSortOrder() const { return m_sortOrder; }

protected:
  CGUIViewState(const CFileItemList& items);  // no direct object creation, use GetViewState()
  virtual void SaveViewState()=0;

  void AddViewAsControl(VIEW_METHOD viewAsControl, int buttonLabel);
  void SetViewAsControl(VIEW_METHOD viewAsControl);
  void AddSortMethod(SORT_METHOD sortMethod, int buttonLabel);
  void SetSortMethod(SORT_METHOD sortMethod);
  void SetSortOrder(SORT_ORDER sortOrder) { m_sortOrder=sortOrder; }
  const CFileItemList& m_items;

private:
  typedef struct _VIEW
  {
    VIEW_METHOD m_viewAsControl;
    int m_buttonLabel;
  } VIEW;
  vector<VIEW> m_viewAsControls;
  int m_currentViewAsControl;

  typedef struct _SORT
  {
    SORT_METHOD m_sortMethod;
    int m_buttonLabel;
  } SORT;
  vector<SORT> m_sortMethods;
  int m_currentSortMethod;

  SORT_ORDER m_sortOrder;
};

class CGUIViewStateGeneral : public CGUIViewState
{
public:
  CGUIViewStateGeneral(const CFileItemList& items);

protected:
  virtual void SaveViewState() {};
};
