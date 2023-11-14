/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DialogGameVideoSelect.h"
#include "cores/GameSettings.h"

#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CDialogGameStretchMode : public CDialogGameVideoSelect
{
public:
  CDialogGameStretchMode();
  ~CDialogGameStretchMode() override = default;

protected:
  // implementation of CDialogGameVideoSelect
  std::string GetHeading() override;
  void PreInit() override;
  void GetItems(CFileItemList& items) override;
  void OnItemFocus(unsigned int index) override;
  unsigned int GetFocusedItem() const override;
  void PostExit() override;
  bool OnClickAction() override;

private:
  struct StretchModeProperties
  {
    int stringIndex;
    RETRO::STRETCHMODE stretchMode;
  };

  std::vector<StretchModeProperties> m_stretchModes;

  /*!
   * \brief The list of all the stretch modes along with their properties
   */
  static const std::vector<StretchModeProperties> m_allStretchModes;
};
} // namespace GAME
} // namespace KODI
