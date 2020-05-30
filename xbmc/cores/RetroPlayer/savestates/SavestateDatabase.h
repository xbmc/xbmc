/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

class CFileItemList;

namespace KODI
{
namespace RETRO
{
class ISavestate;

class CSavestateDatabase
{
public:
  CSavestateDatabase();
  virtual ~CSavestateDatabase() = default;

  std::unique_ptr<ISavestate> CreateSavestate();

  bool AddSavestate(const std::string& gamePath, const ISavestate& save);

  bool GetSavestate(const std::string& gamePath, ISavestate& save);

  bool GetSavestatesNav(CFileItemList& items,
                        const std::string& gamePath,
                        const std::string& gameClient = "");

  bool RenameSavestate(const std::string& path, const std::string& label);

  bool DeleteSavestate(const std::string& path);

  bool ClearSavestatesOfGame(const std::string& gamePath, const std::string& gameClient = "");
};
} // namespace RETRO
} // namespace KODI
