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

using namespace PVR;

std::string CPVRStreamProperties::GetStreamURL() const
{
  for (const auto& prop : *this)
  {
    if (prop.first == PVR_STREAM_PROPERTY_STREAMURL)
      return prop.second;
  }
  return {};
}

std::string CPVRStreamProperties::GetStreamMimeType() const
{
  for (const auto& prop : *this)
  {
    if (prop.first == PVR_STREAM_PROPERTY_MIMETYPE)
      return prop.second;
  }
  return {};
}

bool CPVRStreamProperties::EPGPlaybackAsLive() const
{
  for (const auto& prop : *this)
  {
    if (prop.first == PVR_STREAM_PROPERTY_EPGPLAYBACKASLIVE)
      return StringUtils::EqualsNoCase(prop.second, "true");
  }
  return false;
}
