/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/addoninfo/AddonType.h"
#include "utils/Variant.h"

#include <string>

namespace KODI
{
namespace WEB
{

//! \brief The text of the first \p element of a web app's manifest whose id is \p id
inline std::string WebAppElement(const ADDON::CAddonType& type,
                                 const std::string& element,
                                 const std::string& id)
{
  for (const auto& [name, values] : type.GetValues())
  {
    if (name == element && values.GetValue(element + "@id").asString() == id)
      return values.GetValue(element).asString();
  }
  return {};
}

} // namespace WEB
} // namespace KODI
