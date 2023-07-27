/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>

namespace tinyxml2
{
class XMLElement;
}

class CKeyboardTranslator
{
public:
  static uint32_t TranslateButton(const tinyxml2::XMLElement* pButton);
  static uint32_t TranslateString(const std::string& szButton);
};
