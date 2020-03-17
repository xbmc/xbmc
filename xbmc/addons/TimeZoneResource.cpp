/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "TimeZoneResource.h"

#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <date/tz.h>

namespace ADDON
{

bool CTimeZoneResource::IsAllowed(const std::string& file) const
{
  return true;
}

bool CTimeZoneResource::IsInUse() const
{
  return true;
}

void CTimeZoneResource::OnPostInstall(bool update, bool modal)
{
  date::reload_tzdb();
}

} // namespace ADDON
