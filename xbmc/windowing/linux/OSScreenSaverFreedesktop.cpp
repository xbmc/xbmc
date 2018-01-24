/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "OSScreenSaverFreedesktop.h"

#include "guilib/LocalizeStrings.h"
#include "platform/linux/DBusMessage.h"
#include "platform/linux/DBusUtil.h"
#include "platform/linux/PlatformConstants.h"
#include "utils/log.h"

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
  inhibitMessage.AppendArguments(KODI::LINUX::DESKTOP_FILE_NAME, g_localizeStrings.Get(14086));
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