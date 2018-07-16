/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/interfaces/IActionMap.h"

namespace KODI
{
namespace KEYBOARD
{
  class CKeymapActionMap : public IActionMap
  {
  public:
    CKeymapActionMap(void) = default;

    ~CKeymapActionMap(void) override = default;

    // implementation of IActionMap
    unsigned int GetActionID(const CKey& key) override;
  };
}
}
