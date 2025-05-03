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

namespace
{
constexpr auto KeyProjection = &std::pair<std::string, std::string>::first;
}

std::string CPVRStreamProperties::GetStreamURL() const
{
  const auto it = std::ranges::find(*this, PVR_STREAM_PROPERTY_STREAMURL, KeyProjection);
  return it != cend() ? (*it).second : std::string();
}

std::string CPVRStreamProperties::GetStreamMimeType() const
{
  const auto it = std::ranges::find(*this, PVR_STREAM_PROPERTY_MIMETYPE, KeyProjection);
  return it != cend() ? (*it).second : std::string();
}

bool CPVRStreamProperties::EPGPlaybackAsLive() const
{
  const auto it = std::ranges::find(*this, PVR_STREAM_PROPERTY_EPGPLAYBACKASLIVE, KeyProjection);
  return it != cend() ? StringUtils::EqualsNoCase((*it).second, "true") : false;
}

bool CPVRStreamProperties::LivePlaybackAsEPG() const
{
  const auto it = std::ranges::find(*this, PVR_STREAM_PROPERTY_LIVEPLAYBACKASEPG, KeyProjection);
  return it != cend() ? StringUtils::EqualsNoCase((*it).second, "true") : false;
}
