/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonExtensions.h"

#include "utils/StringUtils.h"

using namespace ADDON;

bool SExtValue::asBoolean() const
{
  return StringUtils::EqualsNoCase(str, "true");
}

SExtValue CAddonExtensions::GetValue(std::string_view id) const
{
  for (const auto& [_, values] : m_values)
  {
    for (const auto& [valueId, valueValue] : values)
    {
      if (valueId == id)
        return valueValue;
    }
  }
  return {};
}

const EXT_VALUES& CAddonExtensions::GetValues() const
{
  return m_values;
}

const CAddonExtensions* CAddonExtensions::GetElement(std::string_view id) const
{
  for (const auto& [childId, childExts] : m_children)
  {
    if (childId == id)
      return &childExts;
  }

  return nullptr;
}

EXT_ELEMENTS CAddonExtensions::GetElements(std::string_view id /*= ""*/) const
{
  if (id.empty())
    return m_children;

  EXT_ELEMENTS children;
  for (const auto& [childId, childExts] : m_children)
  {
    if (childId == id)
      children.emplace_back(childId, childExts);
  }
  return children;
}

void CAddonExtensions::Insert(const std::string& id, const std::string& value)
{
  EXT_VALUE extension;
  extension.emplace_back(id, SExtValue(value));
  m_values.emplace_back(id, extension);
}
