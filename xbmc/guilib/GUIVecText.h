/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/ColorUtils.h"

#include <stdint.h>
#include <vector>

struct character_t
{
  character_t(char l, int s, UTILS::COLOR::Color c)
    : letter(static_cast<char32_t>(l)), style(s), color(c)
  {
  }
  character_t(char16_t l, int s, UTILS::COLOR::Color c)
    : letter(static_cast<char32_t>(l)), style(s), color(c)
  {
  }
  character_t(wchar_t l, int s, UTILS::COLOR::Color c)
    : letter(static_cast<char32_t>(l)), style(s), color(c)
  {
  }
  character_t(char32_t l, int s, UTILS::COLOR::Color c) : letter(l), style(s), color(c) {}

  bool operator==(const character_t& right) const
  {
    if (letter == right.letter && style == right.style && color == right.color)
      return true;
    return false;
  }

  bool operator!=(const character_t& right) const
  {
    if (letter != right.letter || style != right.style || color != right.color)
      return true;
    return false;
  }

  char32_t letter;
  uint32_t style;
  UTILS::COLOR::ColorIndex color;
};

using vecText = std::vector<character_t>;
