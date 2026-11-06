/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RosNameUtils.h"

using namespace KODI;
using namespace SMART_HOME;

std::string KODI::SMART_HOME::NormalizeRosNamespace(std::string_view rosNamespace)
{
  std::string normalized;
  for (const char character : rosNamespace)
  {
    if (character == '/')
    {
      if (!normalized.empty() && normalized.back() != '/')
        normalized.push_back(character);
    }
    else
      normalized.push_back(character);
  }

  while (!normalized.empty() && normalized.back() == '/')
    normalized.pop_back();

  return normalized.empty() ? "/" : "/" + normalized;
}

std::string KODI::SMART_HOME::BuildRosName(std::string_view rosNamespace,
                                           const std::vector<std::string_view>& pathComponents)
{
  std::string name = NormalizeRosNamespace(rosNamespace);
  for (std::string_view component : pathComponents)
  {
    while (!component.empty() && component.front() == '/')
      component.remove_prefix(1);
    while (!component.empty() && component.back() == '/')
      component.remove_suffix(1);

    if (component.empty())
      continue;

    if (name.back() != '/')
      name.push_back('/');
    name.append(component);
  }
  return name;
}
