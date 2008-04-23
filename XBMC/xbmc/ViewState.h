#pragma once

#include "SortFileItem.h"

#define DEFAULT_VIEW_AUTO (VIEW_TYPE_AUTO << 16)
#define DEFAULT_VIEW_LIST (VIEW_TYPE_LIST << 16)
#define DEFAULT_VIEW_ICONS (VIEW_TYPE_ICON << 16)
#define DEFAULT_VIEW_BIG_ICONS (VIEW_TYPE_BIG_ICON << 16)
#define DEFAULT_VIEW_MAX (((VIEW_TYPE_MAX - 1) << 16) | 60)

class CViewState
{
public:
  CViewState(int viewMode, SORT_METHOD sortMethod, SORT_ORDER sortOrder)
  {
    m_viewMode = viewMode;
    m_sortMethod = sortMethod;
    m_sortOrder = sortOrder;
  };
  CViewState()
  {
    m_viewMode = 0;
    m_sortMethod = SORT_METHOD_LABEL;
    m_sortOrder = SORT_ORDER_ASC;
  };

  int m_viewMode;
  SORT_METHOD m_sortMethod;
  SORT_ORDER m_sortOrder;
};
