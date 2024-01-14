/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace KODI
{
namespace ACTION
{
/*!
 * \ingroup action
 */
class CActionTranslator
{
public:
  static void GetActions(std::vector<std::string>& actionList);
  static bool IsAnalog(unsigned int actionId);
  static bool TranslateString(std::string strAction, unsigned int& actionId);
};
} // namespace ACTION
} // namespace KODI
