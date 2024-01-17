/*
 *  Copyright (C) 2020-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keyboard/Key.h"

#include <string>

namespace KODI
{
namespace KEYMAP
{

/*!
 * \ingroup keymap
 */
class CButtonStat
{
public:
  CButtonStat();
  ~CButtonStat() = default;

  CKey TranslateKey(const CKey& key) const;
};
} // namespace KEYMAP
} // namespace KODI
