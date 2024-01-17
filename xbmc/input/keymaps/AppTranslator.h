/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>

namespace KODI
{
namespace KEYMAP
{
/*!
 * \ingroup keymap
 */
class CAppTranslator
{
public:
  static uint32_t TranslateAppCommand(const std::string& szButton);
};
} // namespace KEYMAP
} // namespace KODI
