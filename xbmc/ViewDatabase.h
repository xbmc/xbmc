#pragma once
#include "Database.h"

#define VIEW_DATABASE_NAME "ViewModes.db"

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

class CViewDatabase : public CDatabase
{
public:
  CViewDatabase(void);
  virtual ~CViewDatabase(void);

  bool GetViewState(const CStdString &path, int windowID, CViewState &state);
  bool SetViewState(const CStdString &path, int windowID, const CViewState &state);

protected:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);
};
