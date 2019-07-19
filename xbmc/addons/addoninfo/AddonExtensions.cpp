/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonExtensions.h"

using namespace ADDON;

const SExtValue CAddonExtensions::GetValue(const std::string& id) const
{
  for (auto values : m_values)
  {
    for (auto value : values.second)
    {
      if (value.first == id)
        return value.second;
    }
  }
  return SExtValue("");
}

const EXT_VALUES& CAddonExtensions::GetValues() const
{
  return m_values;
}

const CAddonExtensions* CAddonExtensions::GetElement(const std::string& id) const
{
  for (const auto& child : m_children)
  {
    if (child.first == id)
      return &child.second;
  }

  return nullptr;
}

const EXT_ELEMENTS CAddonExtensions::GetElements(const std::string& id) const
{
  if (id.empty())
    return m_children;

  EXT_ELEMENTS children;
  for (auto child : m_children)
  {
    if (child.first == id)
      children.push_back(std::make_pair(child.first, child.second));
  }
  return children;
}

void CAddonExtensions::Insert(const std::string& id, const std::string& value)
{
  EXT_VALUE extension;
  extension.push_back(std::make_pair(id, SExtValue(value)));
  m_values.push_back(std::make_pair(id, extension));
}
