/*
*  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlatformWebOS.h"

#include "ServiceBroker.h"
#include "filesystem/SpecialProtocol.h"
#include "powermanagement/LunaPowerManagement.h"
#include "utils/log.h"

#include <sys/resource.h>

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformWebOS();
}

bool CPlatformWebOS::InitStageOne()
{
  // WebOS ipks run in a chroot like environment. $HOME is set to the ipk dir and $LD_LIBRARY_PATH is lib
  const auto HOME = std::string(getenv("HOME"));
  setenv("XDG_RUNTIME_DIR", "/tmp/xdg", 1);
  setenv("XKB_CONFIG_ROOT", "/usr/share/X11/xkb", 1);
  setenv("WAYLAND_DISPLAY", "wayland-0", 1);
  setenv("PYTHONHOME", (HOME + "/lib/python3").c_str(), 1);
  setenv("PYTHONPATH", (HOME + "/lib/python3").c_str(), 1);
  setenv("PYTHONIOENCODING", "UTF-8", 1);
  setenv("KODI_HOME", HOME.c_str(), 1);
  setenv("SSL_CERT_FILE",
         CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem").c_str(), 1);

  return CPlatformLinux::InitStageOne();
}

bool CPlatformWebOS::InitStageTwo()
{
  if (CServiceBroker::GetLogging().GetLogLevel() > LOG_LEVEL_DEBUG)
  {
    constexpr rlimit limit{0, 0};
    if (setrlimit(RLIMIT_CORE, &limit) != 0)
      CLog::Log(LOGERROR, "Failed to disable core dumps");
  }

  return CPlatformLinux::InitStageTwo();
}

void CPlatformWebOS::RegisterPowerManagement()
{
  CLunaPowerManagement::Register();
}
