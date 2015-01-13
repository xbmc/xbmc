#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "dbwrappers/Database.h"
#include <string>

class CViewState;

class CViewDatabase : public CDatabase
{
public:
  CViewDatabase();
  virtual ~CViewDatabase();
  virtual bool Open();

  bool GetViewState(const std::string &path, int windowID, CViewState &state, const std::string &skin);
  bool SetViewState(const std::string &path, int windowID, const CViewState &state, const std::string &skin);
  bool ClearViewStates(int windowID);

protected:
  virtual void CreateTables();
  virtual void CreateAnalytics();
  virtual void UpdateTables(int version);
  virtual int GetSchemaVersion() const { return 6; }
  const char *GetBaseDBName() const { return "ViewModes"; }
};
