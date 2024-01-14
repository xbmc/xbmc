/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keymaps/interfaces/IKeyboardActionMap.h"

namespace KODI
{
namespace KEYMAP
{
/*!
 * \ingroup keymap
 */
class CKeyboardActionMap : public IKeyboardActionMap
{
public:
  CKeyboardActionMap() = default;

  ~CKeyboardActionMap() override = default;

  // implementation of IActionMap
  unsigned int GetActionID(const CKey& key) override;
};
} // namespace KEYMAP
} // namespace KODI
