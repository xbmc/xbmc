/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

/*!
\file GUIInfoColor.h
\brief
*/

#include "utils/Color.h"

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
  constexpr CGUIInfoColor(UTILS::Color color = 0):m_color(color) {}

  constexpr operator UTILS::Color() const { return m_color; };

  bool Update();
  void Parse(const std::string &label, int context);

private:
  int m_info = 0;
  UTILS::Color m_color;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
