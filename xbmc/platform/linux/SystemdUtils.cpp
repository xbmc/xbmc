/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "SystemdUtils.h"

#include "DBusUtil.h"
#include "utils/log.h"

// systemd DBus interface specification:
// https://www.freedesktop.org/wiki/Software/systemd/dbus

static const std::string SYSTEMD_DEST = "org.freedesktop.systemd1";
static const std::string SYSTEMD_PATH = "/org/freedesktop/systemd1";
static const std::string SYSTEMD_IFACE = "org.freedesktop.systemd1.Manager";

bool CSystemdUtils::EnableUnit(const char **units, unsigned int length)
{
  CDBusMessage message(SYSTEMD_DEST, SYSTEMD_PATH, SYSTEMD_IFACE, "EnableUnitFiles");

  // It takes a list of unit files to enable (either just file names or full absolute paths)
  message.AppendArgument(units, length);

  // The first controls whether the unit shall be enabled for runtime only (true, /run), or persistently (false, /etc)
  message.AppendArgument(false);

  // The second one controls whether symlinks pointing to other units shall be replaced if necessary
  message.AppendArgument(false);
  return message.SendSystem() != NULL;
}

bool CSystemdUtils::DisableUnit(const char **units, unsigned int length)
{
  CDBusMessage message(SYSTEMD_DEST, SYSTEMD_PATH, SYSTEMD_IFACE, "DisableUnitFiles");

  // It takes a list of unit files to enable (either just file names or full absolute paths)
  message.AppendArgument(units, length);

  // The first controls whether the unit shall be enabled for runtime only (true, /run), or persistently (false, /etc)
  message.AppendArgument(false);

  return message.SendSystem() != NULL;
}

bool CSystemdUtils::StartUnit(const std::string& unit)
{
  CDBusMessage message(SYSTEMD_DEST, SYSTEMD_PATH, SYSTEMD_IFACE, "StartUnit");

  // Takes the unit to activate, plus a mode string
  message.AppendArgument(unit.c_str());
  message.AppendArgument("replace");

  return message.SendSystem() != NULL;
}

bool CSystemdUtils::StopUnit(const std::string& unit)
{
  CDBusMessage message(SYSTEMD_DEST, SYSTEMD_PATH, SYSTEMD_IFACE, "StopUnit");

  // Takes the unit to activate, plus a mode string
  message.AppendArgument(unit.c_str());
  message.AppendArgument("replace");

  return message.SendSystem() != NULL;
}
