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

class CDateTime;
class CFileItem;
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

  static std::unique_ptr<ISavestate> AllocateSavestate();

  bool AddSavestate(const std::string& savestatePath,
                    const std::string& gamePath,
                    const ISavestate& save);

  bool GetSavestate(const std::string& savestatePath, ISavestate& save);

  bool GetSavestatesNav(CFileItemList& items,
                        const std::string& gamePath,
                        const std::string& gameClient = "");

  static void GetSavestateItem(const ISavestate& savestate,
                               const std::string& savestatePath,
                               CFileItem& item);

  std::unique_ptr<ISavestate> RenameSavestate(const std::string& savestatePath,
                                              const std::string& label);

  bool DeleteSavestate(const std::string& savestatePath);

  bool ClearSavestatesOfGame(const std::string& gamePath, const std::string& gameClient = "");

  static std::string MakeSavestatePath(const std::string& gamePath, const CDateTime& creationTime);
  static std::string MakeThumbnailPath(const std::string& savestatePath);

private:
  static std::string MakePath(const std::string& gamePath);
  static bool CreateFolderIfNotExists(const std::string& path);
};
} // namespace RETRO
} // namespace KODI
