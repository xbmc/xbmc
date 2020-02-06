/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OSScreenSaverFreedesktop.h"

#include "CompileInfo.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"

#include "platform/linux/DBusMessage.h"
#include "platform/linux/DBusUtil.h"

using namespace KODI::WINDOWING::LINUX;

namespace
{
const std::string SCREENSAVER_OBJECT = "/org/freedesktop/ScreenSaver";
const std::string SCREENSAVER_INTERFACE = "org.freedesktop.ScreenSaver";
}

bool COSScreenSaverFreedesktop::IsAvailable()
{
  // Test by making a call to a function without side-effects
  CDBusMessage dummyMessage(SCREENSAVER_INTERFACE, SCREENSAVER_OBJECT, SCREENSAVER_INTERFACE, "GetActive");
  CDBusError error;
  dummyMessage.SendSession(error);
  // We do not care whether GetActive() is actually supported, we're just checking for the name to be there
  // (GNOME for example does not support GetActive)
  return !error || error.Name() == DBUS_ERROR_NOT_SUPPORTED;
}

void COSScreenSaverFreedesktop::Inhibit()
{
  if (m_inhibited)
  {
    return;
  }

  CDBusMessage inhibitMessage(SCREENSAVER_INTERFACE, SCREENSAVER_OBJECT, SCREENSAVER_INTERFACE, "Inhibit");
  inhibitMessage.AppendArguments(std::string(CCompileInfo::GetAppName()),
                                 g_localizeStrings.Get(14086));
  if (!inhibitMessage.SendSession())
  {
    // DBus call failed
    CLog::Log(LOGERROR, "Inhibiting freedesktop screen saver failed");
    return;
  }

  if (!inhibitMessage.GetReplyArguments(&m_cookie))
  {
    CLog::Log(LOGERROR, "Could not retrieve cookie from org.freedesktop.ScreenSaver Inhibit response");
    return;
  }

  m_inhibited = true;
}

void COSScreenSaverFreedesktop::Uninhibit()
{
  if (!m_inhibited)
  {
    return;
  }

  CDBusMessage uninhibitMessage(SCREENSAVER_INTERFACE, SCREENSAVER_OBJECT, SCREENSAVER_INTERFACE, "UnInhibit");
  uninhibitMessage.AppendArgument(m_cookie);
  if (!uninhibitMessage.SendSession())
  {
    CLog::Log(LOGERROR, "Uninhibiting freedesktop screen saver failed");
  }

  m_inhibited = false;
}
