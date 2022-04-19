/*
 *  Copyright (C) 2012-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRStreamProperties.h"

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/pvr_general.h"
#include "utils/StringUtils.h"

#include <algorithm>

using namespace PVR;

std::string CPVRStreamProperties::GetStreamURL() const
{
  const auto it = std::find_if(cbegin(), cend(), [](const auto& prop) {
    return prop.first == PVR_STREAM_PROPERTY_STREAMURL;
  });
  return it != cend() ? (*it).second : std::string();
}

std::string CPVRStreamProperties::GetStreamMimeType() const
{
  const auto it = std::find_if(cbegin(), cend(), [](const auto& prop) {
    return prop.first == PVR_STREAM_PROPERTY_MIMETYPE;
  });
  return it != cend() ? (*it).second : std::string();
}

bool CPVRStreamProperties::EPGPlaybackAsLive() const
{
  const auto it = std::find_if(cbegin(), cend(), [](const auto& prop) {
    return prop.first == PVR_STREAM_PROPERTY_EPGPLAYBACKASLIVE;
  });
  return it != cend() ? StringUtils::EqualsNoCase((*it).second, "true") : false;
}
