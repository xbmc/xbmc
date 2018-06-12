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

#include <string>

#define SAVESTATES_DATABASE_NAME  "Savestates"

class CFileItem;
class CFileItemList;
class CVariant;

namespace KODI
{
namespace GAME
{
  class CSavestate;

  class CSavestateDatabase
  {
  public:
    CSavestateDatabase();
    virtual ~CSavestateDatabase() = default;

    bool AddSavestate(const CSavestate& save);

    bool GetSavestate(const std::string& path, CSavestate& save);

    bool GetSavestatesNav(CFileItemList& items, const std::string& gamePath, const std::string& gameClient = "");

    bool RenameSavestate(const std::string& path, const std::string& label);

    bool DeleteSavestate(const std::string& path);

    bool ClearSavestatesOfGame(const std::string& gamePath, const std::string& gameClient = "");

  private:
    CFileItem* CreateFileItem(const CVariant& object) const;
  };
}
}
