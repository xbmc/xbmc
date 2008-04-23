#pragma once
#include "Database.h"

#define VIEW_DATABASE_NAME "ViewModes.db"

class CViewState;

class CViewDatabase : public CDatabase
{
public:
  CViewDatabase(void);
  virtual ~CViewDatabase(void);

  bool GetViewState(const CStdString &path, int windowID, CViewState &state);
  bool SetViewState(const CStdString &path, int windowID, const CViewState &state);
  bool ClearViewStates(int windowID);

protected:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);
};
