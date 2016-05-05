/*
 *      Copyright (C) 2012-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "dbwrappers/DenormalizedDatabase.h"
#include "dbwrappers/DatabaseQuery.h"

#include <string>

#define SAVESTATES_DATABASE_NAME  "Savestates"

class CFileItemList;
class CVariant;

namespace KODI
{
namespace GAME
{
  class CSavestate;

  class CSavestateDatabase : public CDenormalizedDatabase,
                             public IDatabaseQueryRuleFactory
  {
  public:
    CSavestateDatabase();
    virtual ~CSavestateDatabase() = default;

    // implementation of CDatabase
    virtual bool Open() override;

    bool AddSavestate(const CSavestate& save);

    bool GetSavestate(const std::string& path, CSavestate& save);

    bool GetSavestatesNav(CFileItemList& items, const std::string& gamePath, const std::string& gameClient = "");

    bool RenameSavestate(const std::string& path, const std::string& label);

    bool DeleteSavestate(const std::string& path);

    bool ClearSavestatesOfGame(const std::string& gamePath, const std::string& gameClient = "");

    // implementation of IDatabaseQueryRuleFactory
    virtual CDatabaseQueryRule *CreateRule() const override { return nullptr; } // TODO
    virtual CDatabaseQueryRuleCombination *CreateCombination() const override { return nullptr; } // TODO

  protected:
    // implementation of CDatabase
    virtual void UpdateTables(int version) override;
    virtual int GetSchemaVersion() const override { return 1; }
    virtual const char *GetBaseDBName() const override { return SAVESTATES_DATABASE_NAME; }

    // implementation of CDenormalizedDatabase
    virtual bool Exists(const CVariant& object, int& idObject) override;
    virtual bool IsValid(const CVariant& object) const override;
    virtual CFileItem* CreateFileItem(const CVariant& object) const override;
  };
}
}
