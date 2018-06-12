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
\file GUIInfoBool.h
\brief
*/

#include "interfaces/info/InfoBool.h"

#include <string>

class CGUIListItem;

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{

class CGUIInfoBool
{
public:
  explicit CGUIInfoBool(bool value = false);
  ~CGUIInfoBool();

  operator bool() const { return m_value; };

  void Update(const CGUIListItem *item = NULL);
  void Parse(const std::string &expression, int context);
private:
  INFO::InfoPtr m_info;
  bool m_value;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
