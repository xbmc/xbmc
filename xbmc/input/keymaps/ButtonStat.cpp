/*
 *  Copyright (C) 2020-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "ButtonStat.h"

#include "input/keyboard/KeyboardTypes.h"

using namespace KODI;
using namespace KEYMAP;

CButtonStat::CButtonStat() = default;

CKey CButtonStat::TranslateKey(const CKey& key) const
{
  uint32_t buttonCode = key.GetButtonCode();
  if (key.GetHeld() > KEYBOARD::KEY_HOLD_TRESHOLD)
    buttonCode |= CKey::MODIFIER_LONG;

  CKey translatedKey(buttonCode, key.GetHeld());
  return translatedKey;
}
