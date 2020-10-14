/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CFileItemList;

namespace KODI
{
namespace GAME
{
class CDialogGameSaves;

class CGUIDialogSelectSavestate
{
public:
  static bool ShowAndGetSavestate(const std::string& gamePath, std::string& savestatePath);

private:
  static CDialogGameSaves* GetDialog(const std::string& title);
  static void LogSavestates(const CFileItemList& savestates);
};
} // namespace GAME
} // namespace KODI
