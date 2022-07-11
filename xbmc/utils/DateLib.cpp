/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DateLib.h"

#include "filesystem/Directory.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <cstdlib>
#include <stdexcept>

const std::string KODI::TIME::ExtractTzName(const char* tzname)
{
  if (tzname[0] != '/')
    return tzname;

  std::string result = tzname;
  const char zoneinfo[] = "zoneinfo";
  size_t pos = result.rfind(zoneinfo);
  if (pos == result.npos)
  {
    return "";
  }

  pos = result.find('/', pos);
  result.erase(0, pos + 1);
  return result;
}

const date::time_zone* KODI::TIME::GetTimeZone()
{
  const date::time_zone* tz = nullptr;

#if defined(TARGET_POSIX) && !defined(TARGET_DARWIN)
  const char* tzname = std::getenv("TZ");

  if (tzname && tzname[0] != '\0')
  {
    const std::string tzn = ExtractTzName(tzname);
    if (!tzn.empty())
      tz = date::locate_zone(tzn);
  }

  if (tz)
    return tz;
#endif

  // On FreeBSD, there is a bug preventing date::current_zone
  // from finding the currently installed timezone
  // (https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=210197)
  // Let's fall back to UTC in such cases
  try
  {
    tz = date::current_zone();
  }
  catch (const std::exception& err)
  {
    // Log exception and fall back to UTC
    CLog::LogF(LOGERROR, "exception from current_zone: {}", err.what());
    CLog::LogF(LOGERROR, "falling back to UTC");
    tz = date::locate_zone("UTC");
  }

  return tz;
}
