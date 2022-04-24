/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

  operator bool() const { return m_value; }

  void Update(int contextWindow, const CGUIListItem* item = nullptr);
  void Parse(const std::string &expression, int context);
private:
  INFO::InfoPtr m_info;
  bool m_value;
};

} // namespace GUIINFO
} // namespace GUILIB
} // namespace KODI
