/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace GAME
{
class CDialogGameDiscManager;
class CDiscManagerActions;

/*!
 * \ingroup games
 */
class CDiscManagerButtons
{
public:
  explicit CDiscManagerButtons(CDialogGameDiscManager& discManager,
                               CDiscManagerActions& discActions);
  ~CDiscManagerButtons() = default;

  // Dialog interface
  bool OnClick(int controlId);
  void UpdateButtons(bool ejected, const std::string& selectedDisc);
  void SetFocus(unsigned int menuItemIndex);

private:
  // Construction parameters
  CDialogGameDiscManager& m_discManager;
  CDiscManagerActions& m_discActions;
};
} // namespace GAME
} // namespace KODI
