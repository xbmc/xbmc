/*
 *  Copyright (C) 2020-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dialogs/GUIDialogSelect.h"

class CGUIMessage;

namespace KODI
{
namespace GAME
{
class CDialogGameSaves : public CGUIDialogSelect
{
public:
  CDialogGameSaves();
  ~CDialogGameSaves() override = default;
  bool OnMessage(CGUIMessage& message) override;
  std::string GetSelectedItemPath();

private:
  void OnPopupMenu(int itemIndex);
};
} // namespace GAME
} // namespace KODI
