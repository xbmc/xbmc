/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DialogGameVideoSelect.h"

#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CDialogGameVideoRotation : public CDialogGameVideoSelect
{
public:
  CDialogGameVideoRotation();
  ~CDialogGameVideoRotation() override = default;

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
  // Helper functions
  static std::string GetRotationLabel(unsigned int rotationDegCCW);

  // Dialog parameters
  std::vector<unsigned int> m_rotations; // Degrees counter-clockwise
};
} // namespace GAME
} // namespace KODI
