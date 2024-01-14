/*
 *  Copyright (C) 2016-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/XBMC_vkeys.h"
#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"

#include <vector>

namespace KODI
{
namespace KEYBOARD
{
/*!
 * \ingroup keyboard
 *
 * \brief Hush!!!
 */
class CKeyboardEasterEgg : public IKeyboardDriverHandler
{
public:
  ~CKeyboardEasterEgg() override = default;

  // implementation of IKeyboardDriverHandler
  bool OnKeyPress(const CKey& key) override;
  void OnKeyRelease(const CKey& key) override {}

private:
  static std::vector<XBMCVKey> m_sequence;

  unsigned int m_state = 0;
};
} // namespace KEYBOARD
} // namespace KODI
