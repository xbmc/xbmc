/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace GAME
{
class CDialogGameOSD;

class CDialogGameOSDHelp
{
public:
  CDialogGameOSDHelp(CDialogGameOSD& dialog);

  // Initialize help controls
  void OnInitWindow();

  // Check if any help controls are visible
  bool IsVisible();

private:
  // Utility functions
  bool IsVisible(int windowId);

  // Construction parameters
  CDialogGameOSD& m_dialog;

  // Help control IDs
  static const int CONTROL_ID_HELP_TEXT;
};
} // namespace GAME
} // namespace KODI
