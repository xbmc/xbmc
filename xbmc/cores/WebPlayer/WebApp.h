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

//! \brief Whether a web app's manifest leaves the use of encrypted media to the user
inline bool WebAppEncryptedMediaOptional(const ADDON::CAddonType& type)
{
  return type.GetValue("encryptedmedia@optional").asString() == "true";
}

//! \brief Whether a web app's manifest names the query parameters of its red button
inline bool WebAppHasRedButton(const ADDON::CAddonType& type)
{
  return !type.GetValue("redbutton@on").empty() || !type.GetValue("redbutton@off").empty();
}

//! \brief \p url with the query parameter a web app's manifest names for whether the red
//! button hides the app
inline std::string WebAppRedButtonUrl(const ADDON::CAddonType& type,
                                      const std::string& url,
                                      bool hides)
{
  const std::string parameter = type.GetValue(hides ? "redbutton@on" : "redbutton@off").asString();
  if (parameter.empty())
    return url;

  const size_t fragment = url.find('#');
  std::string withParameter = url.substr(0, fragment);
  withParameter += withParameter.find('?') == std::string::npos ? '?' : '&';
  withParameter += parameter;
  if (fragment != std::string::npos)
    withParameter += url.substr(fragment);
  return withParameter;
}

} // namespace WEB
} // namespace KODI
