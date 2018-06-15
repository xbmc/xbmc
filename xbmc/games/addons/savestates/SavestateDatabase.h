/*
 *  Copyright (C) 2012-2017 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
