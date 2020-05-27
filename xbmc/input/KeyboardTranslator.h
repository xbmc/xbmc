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

class TiXmlElement;

class CKeyboardTranslator
{
public:
  static uint32_t TranslateButton(const TiXmlElement* pButton);
  static uint32_t TranslateString(const std::string& szButton);
};
