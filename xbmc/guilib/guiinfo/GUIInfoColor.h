/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIInfoColor.h
\brief
*/

#include "guilib/GUIListItem.h"
#include "utils/ColorUtils.h"

#include <string>

class CGUIListItem;

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfoColor
{
public:
  constexpr CGUIInfoColor(UTILS::COLOR::Color color = 0) : m_color(color) {}

  constexpr operator UTILS::COLOR::Color() const { return m_color; }

  bool Update(const CGUIListItem* item = nullptr);
  void Parse(const std::string &label, int context);

private:
  int m_info = 0;
  UTILS::COLOR::Color m_color;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
